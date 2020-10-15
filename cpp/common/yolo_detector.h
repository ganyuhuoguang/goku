#ifndef COMMON_YOLO_DETECTOR_H_
#define COMMON_YOLO_DETECTOR_H_

#define CPU_ONLY 1  //add for test
#ifdef USE_CAFFE
#include <vector>
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>
#include <caffe/caffe.hpp>
#include <gflags/gflags.h>
#include "storage.pb.h"

DECLARE_int32(batch_size);

using namespace caffe;
using std::vector;
using std::string;
using novumind::goku::proto::storage::Detail;
using novumind::goku::proto::storage::Box;

namespace novumind {
namespace common {

class YoloDetector {
 public:
  // Construct function
  YoloDetector(
      const string& model_file,
      const string& trained_file, 
      const string& label_file,
      float threshold = 0.5,
      bool is_bgr = false);
  // Do inference
  int GetBatchSize() {
    return FLAGS_batch_size;
  }
  vector<vector<Detail>> Detect(const vector<cv::Mat>& imgs); 
 private:
  void WrapInputLayer(vector<vector<cv::Mat>>* input_batch);
  void Preprocess(const vector<cv::Mat>& imgs, vector<vector<cv::Mat>>* input_batch);
  void RegionLayer(std::vector<Detail>& boxes, vector<float>& pred);
  float CalIOU(const Box& box, const Box& truth);
  void ApplyNMS(vector<Detail>& res, vector<Detail>& boxes, float threshold);
 private:
  // net_ poiter
  std::shared_ptr<caffe::Net<float> > net_;
  // net_ size
  int num_channels_, width_, height_;
  int output_width_, output_height_, output_channels_;
  cv::Size input_geometry_;
  // class labels
  vector<string> labels_;
  // anchor number
  const static int num_anchor_ = 5;
  vector<float> biases_ = {0.57273, 0.677385, 1.87446, 2.06253,
     3.33843, 5.47434, 7.88282, 3.52778, 9.77052, 9.16828};
  float threshold_;
  bool is_bgr_;
  int label_num_;
};

}  // namespace common
}  // namespace novumind
#endif // USE_CAFFE
#endif //COMMON_YOLO_DETECTOR_H_
