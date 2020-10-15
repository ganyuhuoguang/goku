#include <gflags/gflags.h>
#include <string>
#include "common/pipeline.h"

// yolo model parameters
DEFINE_string(yolo_model_file,
    "../models/yolo/yolo-coco_ReLU.prototxt",
    "yolo detector model file's path");
DEFINE_string(yolo_trained_file,
    "../models/yolo/yolo-coco_ReLU.caffemodel",
    "yolo detector trained model's path");
DEFINE_string(yolo_label_file,
    "../models/yolo/coco.names",
    "yolo detector label text's path");

#ifdef USE_CAFFE
DEFINE_string(model_file, "", "model file's path");
DEFINE_string(trained_file, "", "trained model's path");
DEFINE_string(mean_file, "", "mean file's path");
DEFINE_string(label_file, "", "class labels text file");
DEFINE_double(stdvar, 1.0, "input standard variance");
#endif // USE_CAFFE
#ifdef USE_TENSORRT
DEFINE_string(engine_file, "../models/resnet50engine.rt", "model file's path");
DEFINE_string(output_blob_file, "", "model output blob names");
DEFINE_string(mean_file, "", "mean file's path");
DEFINE_string(label_file, "", "class labels text file");
DEFINE_double(stdvar, 1.0, "input standard variance");
#endif // USE_TENSORRT
#ifdef USE_NOVURT
DEFINE_string(novu_model_file, "", "novu model file's path");
DEFINE_string(caffe_model_file, "", "caffe model file's path");
DEFINE_string(trained_file, "", "trained model's path");
DEFINE_string(mean_file, "", "mean file's path");
DEFINE_string(label_file, "", "class labels text file");
DEFINE_double(stdvar, 1.0, "input standard variance");
#endif // USE_NOVURT

namespace novumind {
namespace common {
#ifdef USE_CAFFE
InferPipe::InferPipe() {
  vector<int> interests;
  vector<double> thresholds;
  classifiers_.emplace_back(new Classifier(
    FLAGS_model_file, FLAGS_trained_file, 
    FLAGS_mean_file, FLAGS_label_file,
    thresholds, interests, FLAGS_stdvar));
  batch_size_ = classifiers_[0]->GetBatchSize();
}

void InferPipe::Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) {
  vector<cv::Mat> imgs;
  vector<uint64_t> frame_nums;
  for (auto& obj : objs) {
    imgs.emplace_back(obj.mat);
    frame_nums.emplace_back(obj.frame_num);
  }
  vector<vector<Detail>> results;
  for (int i = 0; i < classifiers_.size(); i++) {
    results.emplace_back(classifiers_[i]->Classify(imgs));
  }
  for (int i = 0; i < frame_nums.size(); i++) {
    for (int j = 0; j < results.size(); j++) {
      FrameMetadata metadata;
      metadata.set_frame_num(frame_nums[i]);
      metadata.set_model_id(std::to_string(j));
      if (results[j][i].class__size() > 0) {
        *metadata.add_details() = results[j][i];
      }
      metadatas.emplace_back(metadata);
    }
  }
}

YoloPipe::YoloPipe() {
  detector_.reset(new YoloDetector(FLAGS_yolo_model_file, FLAGS_yolo_trained_file, FLAGS_yolo_label_file));
}

void YoloPipe::Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) {
  vector<cv::Mat> imgs;
  vector<uint64_t> frame_nums;
  for (auto& obj: objs) {
    imgs.emplace_back(obj.mat);
    frame_nums.emplace_back(obj.frame_num);
  }
  vector<vector<Detail>> results = detector_->Detect(imgs);
  for (int i = 0; i < frame_nums.size(); i++) {
    FrameMetadata metadata;
    metadata.set_frame_num(frame_nums[i]);
    metadata.set_model_id("2");
    for (auto& detail : results[i]) {
      *metadata.add_details() = detail;
    }
    metadatas.emplace_back(metadata);
  }
}

MixPipe::MixPipe() {
  detector_.reset(new YoloDetector(
    FLAGS_yolo_model_file, FLAGS_yolo_trained_file, FLAGS_yolo_label_file));
  vector<int> interests;
  vector<double> thresholds;
  classifiers_.reset(new Classifier(
    FLAGS_model_file, FLAGS_trained_file, FLAGS_mean_file, FLAGS_label_file, 
    thresholds, interests, FLAGS_stdvar));
  batch_size_ = classifiers_->GetBatchSize();
}

void MixPipe::Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) {
  vector<cv::Mat> imgs;
  vector<uint64_t> frame_nums;
  for (auto& obj: objs) {
    imgs.emplace_back(obj.mat);
    frame_nums.emplace_back(obj.frame_num);
  }
  vector<vector<Detail>> detect_results = detector_->Detect(imgs);
  vector<Detail> classify_results = classifiers_->Classify(imgs);
  for (int i = 0; i < frame_nums.size(); i++) {
    FrameMetadata metadata;
    metadata.set_frame_num(frame_nums[i]);
    metadata.set_model_id("2");
    for (auto& detail : detect_results[i]) {
      *metadata.add_details() = detail;
    }
    metadatas.emplace_back(metadata);
    FrameMetadata classify_meta;
    classify_meta.set_frame_num(frame_nums[i]);
    classify_meta.set_model_id("0");
    if (classify_results[i].class__size() > 0) {
      *classify_meta.add_details() = classify_results[i];
    }
    metadatas.emplace_back(classify_meta);
  }
}
#endif // USE_CAFFE
#ifdef USE_TENSORRT
InferPipe::InferPipe() {
  vector<int> interests;
  vector<double> thresholds;
  classifiers_.emplace_back(new Classifier(
    FLAGS_engine_file, FLAGS_output_blob_file, 
    FLAGS_mean_file, FLAGS_label_file,
    thresholds, interests, FLAGS_stdvar));
  batch_size_ = classifiers_[0]->GetBatchSize();
}

void InferPipe::Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) {
  vector<cv::Mat> imgs;
  vector<uint64_t> frame_nums;
  for (auto& obj : objs) {
    imgs.emplace_back(obj.mat);
    frame_nums.emplace_back(obj.frame_num);
  }
  vector<vector<Detail>> results;
  for (int i = 0; i < classifiers_.size(); i++) {
    results.emplace_back(classifiers_[i]->Classify(imgs));
  }
  for (int i = 0; i < frame_nums.size(); i++) {
    for (int j = 0; j < results.size(); j++) {
      FrameMetadata metadata;
      metadata.set_frame_num(frame_nums[i]);
      metadata.set_model_id(std::to_string(j));
      if (results[j][i].class__size() > 0) {
        *metadata.add_details() = results[j][i];
      }
      metadatas.emplace_back(metadata);
    }
  }
}
#endif // USE_TENSORRT
#ifdef USE_NOVURT
InferPipe::InferPipe() {
  vector<int> interests;
  vector<double> thresholds;
  classifiers_.emplace_back(new Classifier(
    FLAGS_novu_model_file, FLAGS_caffe_model_file, FLAGS_trained_file, 
    FLAGS_mean_file, FLAGS_label_file,
    thresholds, interests, FLAGS_stdvar));
  batch_size_ = classifiers_[0]->GetBatchSize();
}

void InferPipe::Forward(const vector<FrameObj>& objs, vector<FrameMetadata>& metadatas) {
  vector<cv::Mat> imgs;
  vector<uint64_t> frame_nums;
  for (auto& obj : objs) {
    imgs.emplace_back(obj.mat);
    frame_nums.emplace_back(obj.frame_num);
  }
  vector<vector<Detail>> results;
  for (int i = 0; i < classifiers_.size(); i++) {
    results.emplace_back(classifiers_[i]->Classify(imgs));
  }
  for (int i = 0; i < frame_nums.size(); i++) {
    for (int j = 0; j < results.size(); j++) {
      FrameMetadata metadata;
      metadata.set_frame_num(frame_nums[i]);
      metadata.set_model_id(std::to_string(j));
      if (results[j][i].class__size() > 0) {
        *metadata.add_details() = results[j][i];
      }
      metadatas.emplace_back(metadata);
    }
  }
}
#endif // USE_NOVURT
}  // namespace common
}  // namespace novumind
