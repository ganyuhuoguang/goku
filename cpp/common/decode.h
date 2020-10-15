#ifndef COMMON_DECODE_H_
#define COMMON_DECODE_H_

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#ifdef USE_TENSORRT
#include <cuda.h>
#endif //USE_TENSORRT
#include <memory>
#include <thread>

#include "common/concurrent.h"
#include "common/handler.h"
#include "common.pb.h"

using novumind::common::MultiThreadFlag;
using novumind::goku::proto::common::VideoMetadata;

namespace novumind {
namespace common {
// Abstract class for decoder.
class Decoder {
 public:
  // Open a video by address
  // This func should be called before calling other func.
  virtual bool Open(std::string address) = 0;

  // Get video metadata.
  virtual VideoMetadata GetVideoMetadata() = 0;

  // Start a thread to decode video stream and return immediately.
  virtual void Run(Handler* handler) = 0;
  // Wait until the termination of the decode thread.
  // Return false if it's aborted by exception.
  virtual bool Join() = 0;
};

class CPUDecoder : public Decoder {
 public:
  explicit CPUDecoder() : aborted_flag_(false) {}
  // Use decoder to open a video, return true if success.
  bool Open(std::string address) override {
    address_ = address;
    cv::VideoCapture capture;
    capture.open(address_);
    capture.open(address_);
    if (!capture.isOpened()) {
      return false;
    }
    width_ = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    height_ = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    fps_ = capture.get(CV_CAP_PROP_FPS);
    frame_count_ = capture.get(CV_CAP_PROP_FRAME_COUNT);
    return true;
  }

  VideoMetadata GetVideoMetadata() override {
    VideoMetadata meta;
    meta.set_addr(address_);
    meta.set_width(width_);
    meta.set_height(height_);
    meta.set_fps(fps_);
    meta.set_frame_count((uint64_t)frame_count_);
    return meta;
  }

  void Run(Handler* handler) override {
    std::thread decode_thread(
        &CPUDecoder::Decode, this, handler);
    decode_thread_.swap(decode_thread);
  }

  // Return false if it's aborted by exception.
  bool Join() override {
    decode_thread_.join();
    return !aborted_flag_.value();
  }

 private:
  void Decode(Handler* handler) {
    uint64_t frame_num = 0;
    cv::VideoCapture capture;
    capture.open(address_);
    if (!capture.isOpened()) {
      return;
    }
    cv::Mat frame;
    try {
      while (true) {
        capture >> frame;
        if (!frame.data) {
          break;
        }
        FrameObj frame_obj{frame_num, frame};
        handler->AddFrame(frame_obj);
        frame_num++;
      }
    } catch (cv::Exception e) {
      // Aborted by exception
      aborted_flag_.set_value(true);
    }
    handler->CloseInput();
  }
  std::string address_;
  std::unique_ptr<cv::VideoCapture> capture;
  int width_, height_, frame_count_;
  float fps_;
  MultiThreadFlag aborted_flag_;
  std::thread decode_thread_;
  bool realtime_stream_;
};

/*class GPUDecoder : public Decoder {
 public:
  explicit GPUDecoder() : aborted_flag_(false) {}

  // Use decoder to open a video, return true if success.
  bool Open(std::string address) override {
    try {
      address_ = address;
      // Use video capture to read width/height/fps
      // since cudacodec doesn't support fps property.
      cv::VideoCapture capture;
      capture.open(address);
      if (!capture.isOpened()) {
        return false;
      }
      width_ = capture.get(CV_CAP_PROP_FRAME_WIDTH);
      height_ = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
      fps_ = capture.get(CV_CAP_PROP_FPS);
      frame_count_ = capture.get(CV_CAP_PROP_FRAME_COUNT);
      capture.release();
      reader_ = cv::cudacodec::createVideoReader(address);
      return true;
    } catch (cv::Exception e) {
      return false;
    }
  }

  VideoMetadata GetVideoMetadata() override {
    VideoMetadata meta;
    meta.set_addr(address_);
    meta.set_width(width_);
    meta.set_height(height_);
    meta.set_fps(fps_);
    meta.set_frame_count((uint64_t)frame_count_);
    return meta;
  }

  void Run(Handler* handler) override {
    std::thread decode_thread(
        &GPUDecoder::Decode, this, handler);
    decode_thread_.swap(decode_thread);
  }

  // Return false if it's aborted by exception.
  bool Join() override {
    decode_thread_.join();
    return !aborted_flag_.value();
  }

 private:
  void Decode(Handler* handler) {
    cv::gpu::GpuMat frame;
    cv::gpu::Stream stream;
    uint64_t frame_num = 0;
    try {
      while (true) {
        if(!reader_->nextFrame(frame)) {
          break;
        }
        // The type of the frame is 8UC4, convert to 8UC3
        cv::gpu::cvtColor(frame, frame, CV_BGRA2BGR, 3, stream);
        stream.waitForCompletion();
        GPUFrameObj frame_obj{frame_num, frame};
        handler->AddFrame(frame_obj);
        frame_num++;
      }
    } catch (cv::Exception e) {
      // Aborted by exception
      aborted_flag_.set_value(true);
    }
  }
  cv::Ptr<cv::cudacodec::VideoReader> reader_;
  std::string address_;
  // frame_count_ is total frame count
  int width_, height_, frame_count_;
  float fps_;
  MultiThreadFlag aborted_flag_;
  std::thread decode_thread_;
  bool realtime_stream_;
};*/

}  // namespace common
}  // namespace novumind
#endif  // COMMON_DECODE_H_
