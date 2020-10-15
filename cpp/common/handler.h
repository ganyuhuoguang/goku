#ifndef COMMON_HANDLER_H_
#define COMMON_HANDLER_H_

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#ifdef USE_TENSORRT 
#include <cuda.h>
#endif // USE_TENSORRT
#include <memory>
#include <thread>
#include <mutex>

#include "common/channel.h"
#include "common/pipeline.h"
#include "worker.pb.h"
#include "common.pb.h"

using novumind::goku::proto::worker::AnnotateVideoResponse;
using novumind::goku::proto::worker::VideoSummaryResponse;
using novumind::goku::proto::common::VideoMetadata;

namespace novumind {
namespace common {

// Class for handling decoded image.
class Handler {
 public:
  explicit Handler(int max_chan_capacity, Pipeline* pipeline);
  void AddFrame(const FrameObj& frame_obj) {
    frame_chan_.Put(frame_obj);
  }
  bool GetOutput(AnnotateVideoResponse *resp);
  void GetOutputSummary(VideoSummaryResponse *resp);
  void SetVideoMeta(const VideoMetadata &meta) {
    total_frame_num_ = meta.frame_count();
    width_ = meta.width();
    height_ = meta.height();
    fps_ = meta.fps();
    total_batch_num_ = total_frame_num_ / batch_size_;
    if ((total_frame_num_ % batch_size_) != 0) {
      total_batch_num_++;
    }
  }
  void CloseInput() {
    frame_chan_.Close();
  }
 private:
  int batch_size_, batch_num_, total_batch_num_;
  Pipeline* pipeline_;
  int width_, height_; 
  uint64_t total_frame_num_, processed_num_;
  float fps_;
  Channel<FrameObj> frame_chan_;
};


}  // namespace common
}  // namespace novumind
#endif  // COMMON_HANDLER_H_
