#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/security/server_credentials.h>
#include <gflags/gflags.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <thread>
#include <mutex>
#include <chrono>
#include "common/decode.h"
#include "common/handler.h"
#include "common/pipeline.h"

#include "worker.pb.h"
#include "worker.grpc.pb.h"

#include <condition_variable> //add by xl

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using grpc::StatusCode;

using novumind::goku::proto::worker::Worker;
using novumind::goku::proto::worker::AnnotateVideoRequest;
using novumind::goku::proto::worker::AnnotateVideoResponse;
using novumind::goku::proto::worker::CheckInferenceCoreRequest;
using novumind::goku::proto::worker::CheckInferenceCoreResponse;
using novumind::goku::proto::worker::VideoSummaryResponse;
using novumind::common::Pipeline;
using novumind::common::InferPipe;
#ifdef USE_CAFFE
using novumind::common::YoloPipe;
using novumind::common::MixPipe;
#endif // USE_CAFFE
using novumind::common::Handler;
using novumind::common::Decoder;
//using novumind::common::GPUDecoder;
using novumind::common::CPUDecoder;

DEFINE_int32(port, 8089, "Server listening port.");
// DEFINE_bool(decode_gpu, false, "Whether to use gpu for video decoding");

class InferCoreImpl final : public Worker::Service {
 public:
  explicit InferCoreImpl() : available_(false) {
    pipeline_.reset(new InferPipe());
    //pipeline_.reset(new YoloPipe());
    //pipeline_.reset(new MixPipe());
    SetAvailablity(true);      
  }

  ~InferCoreImpl() {
  }

  Status AnnotateVideo(
      ServerContext* context, const AnnotateVideoRequest* request,
      ServerWriter<AnnotateVideoResponse>* writer) override {
    if (!GetAvailablity()) {
      return Status(StatusCode::INTERNAL, "Inference core is unavailable.");
    }
    const std::string &video_addr = request->video_path();
    std::cout << "====== Handle request: ======\n"
      << request->video_path() << std::endl;
    if (video_addr.empty()) {
      return Status(StatusCode::INVALID_ARGUMENT, "empty video address.");
    }
    // set decoder for video
    //if (FLAGS_decode_gpu) {
    //  decoder_.reset(new GPUDecoder());
    //} else {
    //  decoder_.reset(new CPUDecoder());
    //}
    decoder_.reset(new CPUDecoder());
    if (!decoder_->Open(video_addr)) {
      return Status(StatusCode::INVALID_ARGUMENT, "invalid video address.");
    }
    // The pipeline only do forwarding on one video at one time,
    // so set the availablity to false to avoid multiple video annotations.
    SetAvailablity(false);
    // initialize a handler, which will be passed to decoder & process the video
    handler_.reset(new Handler(100, pipeline_.get()));
    handler_->SetVideoMeta(decoder_->GetVideoMetadata());
    // Run asyn decode thread.
    decoder_->Run(handler_.get());
    while (true) {
      AnnotateVideoResponse resp;
      if (!handler_->GetOutput(&resp)) {
        break;
      }
      writer->Write(resp);
    }
    handler_->CloseInput();
    // if the decode encounter with exception then exit annotating video
    if (!decoder_->Join()) {
      SetAvailablity(true);
      return Status(StatusCode::ABORTED, "Read video file aborted. ");
    }
    // if the video is done processing, then set availablity to true
    std::cout << "Done processing :" << video_addr << std::endl;
    SetAvailablity(true);
    return Status::OK;
  }
  
  Status AnnotateVideoSummary(
      ServerContext* context, 
      ServerReaderWriter<VideoSummaryResponse, AnnotateVideoRequest>* stream) override {
    if (!GetAvailablity()) {
      return Status(StatusCode::INTERNAL, "Inference core is unavailable.");
    }
    AnnotateVideoRequest request;
    while (stream->Read(&request)) {
      SetAvailablity(false);
      const std::string &video_addr = request.video_path();
      std::cout << "====== Handle request: ======\n"
        << request.video_path() << std::endl;
      if (video_addr.empty()) {
        return Status(StatusCode::INVALID_ARGUMENT, "empty video address.");
      }
      decoder_.reset(new CPUDecoder());
      if (!decoder_->Open(video_addr)) {
        return Status(StatusCode::INVALID_ARGUMENT, "invalid video address.");
      }
      // The pipeline only do forwarding on one video at one time,
      // so set the availablity to false to avoid multiple video annotations.
      // initialize a handler, which will be passed to decoder & process the video
      handler_.reset(new Handler(100, pipeline_.get()));
      handler_->SetVideoMeta(decoder_->GetVideoMetadata());
      // Run asyn decode thread.
      decoder_->Run(handler_.get());
      VideoSummaryResponse resp;
      handler_->GetOutputSummary(&resp);
      resp.set_request_id(request.request_id());
      handler_->CloseInput();
      if (!stream->Write(resp)) {
        SetAvailablity(true);
        return Status(StatusCode::UNKNOWN, "Server cannot write response. ");
      }
      // if the decode encounter with exception then exit annotating video
      if (!decoder_->Join()) {
        SetAvailablity(true);
        return Status(StatusCode::ABORTED, "Read video file aborted. ");
      }
    }
    // if the video is done processing, then set availablity to true
    std::cout << "Done processing video list" << std::endl;
    SetAvailablity(true);
    return Status::OK;
  }

  Status CheckInferenceCore(
      ServerContext* context, const CheckInferenceCoreRequest* request,
      CheckInferenceCoreResponse* response) override {
    response->set_is_able(GetAvailablity());
    return Status::OK;
  }
  void SetAvailablity(bool is_available) {
    std::unique_lock<std::mutex> mlock(mutex_);
    available_ = is_available;
  }
  bool GetAvailablity() {
    std::unique_lock<std::mutex> mlock(mutex_);
    return available_;
  }
 private:
  std::unique_ptr<Pipeline> pipeline_;
  std::unique_ptr<Decoder> decoder_;
  std::unique_ptr<Handler> handler_;
  std::mutex mutex_;
  bool available_;
};

void RunServer() {
  std::string server_address("0.0.0.0:");
  std::cout << "server_address  "  << std::endl;
  server_address = server_address + std::to_string(FLAGS_port);
  
  InferCoreImpl infer_server;
  std::cout << "infer_server  "  << std::endl;
  ServerBuilder builder;
  std::cout << "builder  "  << std::endl;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.SetMaxReceiveMessageSize(400*1024*1024);
  builder.SetMaxSendMessageSize(400*1024*1024);
  std::cout << "RegisterService  "  << std::endl;
  builder.RegisterService(&infer_server);
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server  "  << std::endl;
  std::cout << "Server listening on " << server_address << std::endl;
  server->Wait();
}

int main(int argc, char** argv) {
  int i=0;
  std::cout << "gflags before "  << std::endl;
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  std::cout << "gflags  "  << std::endl;
  RunServer();
  return 0;
}
