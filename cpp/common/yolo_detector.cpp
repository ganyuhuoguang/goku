#include <gflags/gflags.h>
#include <map>
#include "common/yolo_detector.h"
#include "common/math_util.h"
#include <iostream>


namespace novumind {
namespace common {

#ifdef USE_CAFFE
YoloDetector::YoloDetector(
    const string& model_file, 
    const string& trained_file,
    const string& label_file,
    float threshold,
    bool is_bgr):
    threshold_(threshold), is_bgr_(is_bgr) {
#ifdef USE_GPU
  Caffe::set_mode(Caffe::GPU);
#else
  Caffe::set_mode(Caffe::CPU);
#endif // USE_GPU

  /* Load the network. */
  net_.reset(new Net<float>(model_file, TEST));
  net_->CopyTrainedLayersFrom(trained_file);
  Blob<float>* input_layer = net_->input_blobs()[0];
  num_channels_ = input_layer->channels();
  CHECK_EQ(num_channels_, 3) << "Input layer should have 3 channels.";
  width_ = input_layer->width();
  height_ = input_layer->height();
  input_geometry_ = cv::Size(width_, height_);
  /* Reshape the net for batch process*/  
  input_layer->Reshape(FLAGS_batch_size, num_channels_, height_, width_);
  net_->Reshape();
  /* Load labels. */
  std::ifstream labels(label_file.c_str());
  CHECK(labels) << "Unable to open labels file " << label_file;
  string line;
  while (std::getline(labels, line)) {
    if (!line.empty()) {
      labels_.push_back(string(line));
    }
  }
  Blob<float>* output_layer = net_->output_blobs()[0];
  output_width_ = output_layer->width();
  output_height_ = output_layer->height();
  output_channels_ = output_layer->channels();
  label_num_ = labels_.size();
  CHECK_EQ(output_channels_, (label_num_ + 5) * num_anchor_)
    << "Number of labels is different from the output layer dimension.";
}

vector<vector<Detail>> YoloDetector::Detect(const vector<cv::Mat>& imgs) { 
#ifdef USE_GPU 
  Caffe::set_mode(Caffe::GPU);
#else
  Caffe::set_mode(Caffe::CPU);
#endif //USE_GPU
  vector<vector<cv::Mat>> input_batch;
  WrapInputLayer(&input_batch);
  Preprocess(imgs, &input_batch);
  net_->Forward();
  
  /* Copy the output layer to a std::vector */
  Blob<float>* output_layer = net_->output_blobs()[0];
  vector<vector<float>> preds;
  const float* begin = output_layer->cpu_data();
  for (int i = 0; i < imgs.size(); ++i) {
    const float* local_begin = begin + i * output_channels_ * output_width_ * output_height_;
    const float* local_end = local_begin + output_channels_ * output_width_ * output_height_;
    preds.push_back(vector<float>(local_begin, local_end));
  }
  vector<vector<Detail>> result;
  for (int i = 0; i < imgs.size(); ++i) {
    vector<Detail> box;
    RegionLayer(box, preds[i]);
    vector<Detail> output;
    ApplyNMS(output, box, threshold_);
    result.emplace_back(output);
  }
  return result;
}

void YoloDetector::RegionLayer(vector<Detail>& details, vector<float>& pred) {
  vector<vector<vector<float>>> feature_map( output_height_ * output_width_, 
      vector<vector<float>>(num_anchor_, vector<float>(label_num_+5)));
  for (int h = 0; h < output_height_; h++) {
    for (int w = 0; w < output_width_; w++) {
      for (int c = 0; c < output_channels_; c++) {
        int idx = (c * output_height_ + h) * output_width_ + w;
        feature_map[h * output_width_ + w][c / (label_num_ + 5)][c % (label_num_ + 5)] = pred[idx];
      }
    }
  }

  for (int h = 0; h < output_height_; h++) {
    for (int w = 0; w < output_width_; w++) {
      for (int n = 0; n < num_anchor_; n++) {
        Detail detail;
        auto box = detail.mutable_bbox();
        //center x, y, w, h
        auto feature = feature_map[h * output_width_ + w][n];
        box->set_x((w + sigmoid(feature[0])) / float(output_width_));
        box->set_y((h + sigmoid(feature[1])) / float(output_height_));
        box->set_width((exp(feature[2]) * biases_[2 * n]) / float(output_width_));
        box->set_height((exp(feature[3]) * biases_[2 * n + 1]) / float(output_height_));
        detail.set_obj_score(sigmoid(feature[4]));
        vector<float> cls;
        for (int p = 0; p < label_num_; p++) {
          cls.push_back(feature[5 + p]);
        }
        const int N = 5;
        vector<int> maxN = Argmax(cls, N);
        auto sum = softmax_sum(cls, cls[maxN[0]]);
        vector<float> confidences;
        vector<string> classes;
        for (int i = 0; i < N; i++) {
          detail.add_class_(labels_[maxN[i]]);
          detail.add_confidence(exp(cls[maxN[i]] - cls[maxN[0]]) / sum);
        }
        if (detail.obj_score() > threshold_) {
          details.push_back(detail);
        }
      }
    }
  }
}

float YoloDetector::CalIOU(const Box &box, const Box& truth) {
  float w = overlap(box.x(), box.width(), truth.x(), truth.width());
  float h = overlap(box.y(), box.height(), truth.y(), truth.height());
  if (w < 0 || h < 0) {
    return 0;
  }
  float inter_area = w * h;
  float union_area = box.width() * box.height() + truth.width() * truth.height() - inter_area;
  return inter_area * 1.0 / union_area;
}

void YoloDetector::ApplyNMS(vector<Detail>& res, vector<Detail> &boxes, float threshold) {
  std::sort(boxes.begin(), boxes.end(), [](const Detail& a, const Detail& b) {
      return a.obj_score() > b.obj_score();});
  std::map<int, int> p;
  for (int i = 0; i < boxes.size(); i++) {
    auto it = p.find(i);
    if (it != p.end()) {
      continue;
    }
    Detail truth = boxes[i];
    for (int j = i+1; j < boxes.size(); j++) {
      auto it = p.find(j);
      if (it != p.end()) {
        continue;
      }
      Detail box = boxes[j];
      float iou = CalIOU(box.bbox(), truth.bbox());
      if (iou > threshold) {
        p[j] = 1;
      }
    }
  }

  for (int i = 0; i < boxes.size(); i++) {
    auto it = p.find(i);
    if (it == p.end()) {
      Detail detail = boxes[i];
      // redo coordinates
      detail.mutable_bbox()->set_x(
          width_* (boxes[i].bbox().x() - int(boxes[i].bbox().width()/2.0f)));
      detail.mutable_bbox()->set_y(
          height_* (boxes[i].bbox().y() - int(boxes[i].bbox().height()/2.0f)));
      detail.mutable_bbox()->set_width(width_* boxes[i].bbox().width());
      detail.mutable_bbox()->set_height(height_* boxes[i].bbox().height());
      res.push_back(detail);
    }
  }
}

void YoloDetector::WrapInputLayer(vector<vector<cv::Mat>>* input_batch) {
  Blob<float>* input_layer = net_->input_blobs()[0];
  float* input_data = input_layer->mutable_cpu_data();
  for (int j = 0; j < FLAGS_batch_size; j++) {
    vector<cv::Mat> input_channels;
    for (int i = 0; i < input_layer->channels(); ++i) {
      cv::Mat channel(height_, width_, CV_32FC1, input_data);
      input_channels.push_back(channel);
      input_data += width_ * height_;
    }
    input_batch->push_back(vector<cv::Mat>(input_channels));
  }
}

void YoloDetector::Preprocess(const vector<cv::Mat>& imgs,
    vector<vector<cv::Mat>>* input_batch) {
  /* Convert the input image to the input image format of the network. */
  for (int i = 0; i < FLAGS_batch_size; ++i) {
    vector<cv::Mat>* input_channels = &(input_batch->at(i));
    cv::Mat sample;
    if (i < imgs.size()) {
      sample = imgs[i];
    } else {
      sample = cv::Mat(input_geometry_, CV_8UC3, cv::Scalar(0, 0, 0));
    }
    if (!is_bgr_) {
      cv::cvtColor(sample, sample, cv::COLOR_BGR2RGB);
    }
    cv::Mat sample_resized;
    if (sample.size() != input_geometry_) {
      cv::resize(sample, sample_resized, input_geometry_);
    } else {
      sample_resized = sample;
    }
    cv::Mat sample_normalized;
    sample_resized.convertTo(sample_normalized, CV_32FC3, 1.0/255.0);

    /* This operation will write the separate BGR planes directly to the
     * input layer of the network because it is wrapped by the cv::Mat
     * objects in input_channels. */
    cv::split(sample_normalized, *input_channels);
  }
}

#endif // USE_CAFFE
}  // namespace common
}  // namespace novumind
