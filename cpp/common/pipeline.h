#ifndef COMMON_PIPELINE_H_
#define COMMON_PIPELINE_H_

#include <vector>
#ifdef USE_TENSORRT
#include <cuda.h>
#endif //USE_TENSORRT
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>
#include <memory>
#include "storage.pb.h"
#include "common/classifier.h"
#include "common/yolo_detector.h"

using std::vector;
using std::unique_ptr;
using novumind::goku::proto::storage::FrameMetadata;

namespace novumind {
namespace common {

// definition of frame object
struct FrameObj {
  uint64_t frame_num;
  cv::Mat mat;
};


// Abstract class for inference pipeline
class Pipeline {
 public:
  // Fit-in cv mats and get the outputs
  // @Input:    mats
  // @Output:   metadatas
  virtual int GetBatchSize() = 0;
  virtual void Forward(const vector<FrameObj>& mats, vector<FrameMetadata>& metadatas) = 0;
};

// Implement a instance of pipeline
class InferPipe :public Pipeline {
 public:
  InferPipe();
  int GetBatchSize() override {
    return batch_size_;
  }
  void Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) override;
 private:
  int batch_size_;
  vector<unique_ptr<Classifier>> classifiers_;
};

#ifdef USE_CAFFE
class YoloPipe :public Pipeline {
 public:
  YoloPipe();
  int GetBatchSize() override {
    return batch_size_;
  }
  void Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) override;
 private:
  int batch_size_;
  unique_ptr<YoloDetector> detector_;
};

class MixPipe :public Pipeline {
 public:
  MixPipe();
  int GetBatchSize() override {
    return batch_size_;
  }
  void Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) override;
 private:
  int batch_size_;
  unique_ptr<YoloDetector> detector_;
  unique_ptr<Classifier> classifiers_;
};
#endif // USE_CAFFE
} //namespace common
} //namespace novumind
#endif // COMMON_PIPELINE_H_
