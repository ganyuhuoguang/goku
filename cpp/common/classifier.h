#ifndef COMMON_CLASSIFIER_H_
#define COMMON_CLASSIFIER_H_
#define CPU_ONLY 1  //add for test

#include <vector>
#include <string>
#include <memory>
#include <opencv2/opencv.hpp>
#ifdef USE_CAFFE
#include <caffe/caffe.hpp>
#endif //USE_CAFFE
#include <gflags/gflags.h>
#include "storage.pb.h"
#ifdef USE_TENSORRT
#include <cuda_runtime_api.h>
#include "common/tensorrt/common.h"
#include "NvCaffeParser.h"
#include "NvInferPlugin.h"
#endif // USE_TENSORRT
#ifdef USE_NOVURT
#include <caffe/caffe.hpp>
#include "novu_nnet.h"   ///novu_rt/novu_nnet.h

#endif // USE_NOVURT


DECLARE_int32(batch_size);

#ifdef USE_CAFFE
using namespace caffe;
#endif //USE_CAFFE
#ifdef USE_NOVURT
using namespace caffe;
#endif //USE_NOVURT
using std::vector;
using std::string;
using novumind::goku::proto::storage::Detail;

namespace novumind {
namespace common {

#ifdef USE_CAFFE
class Classifier {
 public:
  /* Construct function
  param @model_file	caffe model prototxt
	@trained_file	pretrained model file(*.caffemodel)
	@mean_file	caffe model mean file, used in input normalization
	@label_file	model's label list file
  	@thresholds	is the thresholds for interests channels. 
			That means it has to have the same size of interests.
			If we want a topN output, then thresholds should be empty. 
  	@interests	is the output channel indice which we are interested,
  			which can be like [0, 1, 2...]
  			This is because for bi-classfication, we always want the output of channel 1,
  			but for multi-classification, we might want all output channels.
	@stdvar		stdvar is the standard variance of input data,
			some model's are trained with data normalized to N(0,1)
	@base_size	is the resize size of input image, if the preprocess include resize&crop.
			if only need resize, then base_size should be set to 0
	@top_k		for multi-classification softmax output, we want to get top_k output
	@is_bgr		to indicate whether the model is trained with (b, g, r) ordered image
  */
  Classifier(
      const string& model_file, 
      const string& trained_file,
      const string& mean_file,
      const string& label_file,
      const vector<double>& thresholds,
      const vector<int>& interests,
      float stdvar = 1.0,
      int base_size = 0,
      int top_k = 1,
      bool is_bgr = true);
  // Classify batched images
  vector<Detail> Classify(const vector<cv::Mat>& imgs);
  // Get batch size of the model
  int GetBatchSize() {
    return n_;
  }
 private:
  // Set mean mat with mean_file
  void SetMean(const string& mean_file);
  // Set mean mat with default bgr_mean_ or rgb_mean_
  void SetMean();
  // Allocate input layer pointer to input image mat
  void WrapInputLayer(vector<vector<cv::Mat>>* input_batch);
  // Preprocess the images to fit-in input layer
  void Preprocess(const vector<cv::Mat>& imgs, vector<vector<cv::Mat>>* input_batch);
  // Get network output
  vector<vector<float>> Predict(const vector<cv::Mat>& mats);

 private:
  // net_ pointer
  std::shared_ptr<Net<float> > net_;
  // net_ size
  int n_, c_, h_, w_;
  int output_c_;
  // input image size
  cv::Size input_geometry_;
  // resize image base size
  int base_size_;
  // input_channel volume
  int vol_chl_;
  // output blob volume
  int vol_output_;
  // class labels list
  vector<std::string> labels_;
  // default image mean value 
  float rgb_mean_[3] = {122.7717, 115.9465, 102.9801};
  float bgr_mean_[3] = {102.9801, 115.9465, 122.7717};
  // mean mat
  cv::Mat mean_;
  // model channel order
  bool is_bgr_;
  // input image standard variance
  float stdvar_;
  // threshold for each interested channel
  vector<double> thresholds_;
  // interested channel idx
  vector<int> interests_;
  // top k output
  int top_k_;
};
#endif //USE_CAFFE
#ifdef USE_TENSORRT
class Classifier {
 public:
  /* Construct function
  param @engine_file	the TensorRT engine file
	@output_blob_file	indicate the output layer name, which is saved as a list file
	@mean_file	the TensorRT model mean file, which is different from caffe mean file.
			Because TensorRT lib cannot read binaryproto,
			we use a list file to indicate the mean value of each input channel
	@label_file	model's label list file
  	@thresholds	is the thresholds for interests channels. 
			That means it has to have the same size of interests.
			If we want a topN output, then thresholds should be empty. 
  	@interests	is the output channel indice which we are interested,
  			which can be like [0, 1, 2...]
  			This is because for bi-classfication, we always want the output of channel 1,
  			but for multi-classification, we might want all output channels.
	@stdvar		stdvar is the standard variance of input data,
			some model's are trained with data normalized to N(0,1)
	@base_size	is the resize size of input image, if the preprocess include resize&crop.
			if only need resize, then base_size should be set to 0
	@top_k		for multi-classification softmax output, we want to get top_k output
	@is_bgr		to indicate whether the model is trained with (b, g, r) ordered image
  */
  Classifier(
      const string& engine_file, 
      const string& output_blob_file,
      const string& mean_file,
      const string& label_file,
      const vector<double>& thresholds, 
      const vector<int>& interests,
      float stdvar = 1.0,
      int base_size = 0,
      int top_k = 1,
      bool is_bgr = true);
  // Destruction function, release memory allocated on cpu&gpu, release tensorrt handlers
  ~Classifier() {
    context_->destroy();
    engine_->destroy();
    runtime_->destroy();
    cudaStreamDestroy(stream_);
    CHECK(cudaFree(buffers_[0]));
    CHECK(cudaFree(buffers_[1]));
    delete[] input_batch_;
    delete[] output_batch_;
  }
  // Classify batched images
  vector<Detail> Classify(const vector<cv::Mat>& imgs);
  // Get batch size of model
  int GetBatchSize() {
    return n_;
  }
 private:
  // Set mean mat with mean_file
  void SetMean(const string& mean_file);
  // Allocate input layer pointer to input image mat
  void WrapInputLayer(vector<vector<cv::Mat>>* input_batch);
  // Preprocess the images to fit-in input layer
  void Preprocess(const vector<cv::Mat>& imgs, vector<vector<cv::Mat>>* input_batch);
  // Get network output
  vector<vector<float>> Predict(const vector<cv::Mat>& imgs);
 private:
  // tensorrt handlers
  nvinfer1::IRuntime* runtime_;
  nvinfer1::ICudaEngine* engine_;
  nvinfer1::IExecutionContext* context_;
  cudaStream_t stream_;
  // input shape
  int n_, c_, h_, w_;
  // input image shape
  cv::Size input_geometry_;
  // resize image base size
  int base_size_;
  // output shape
  int output_c_, output_h_, output_w_;
  // input size of (one channel, one output volume)
  int vol_chl_, vol_output_;
  // input&output volume size
  size_t input_size_, output_size_;
  // input&output gpu memory pointers
  void* buffers_[2];
  // input&output cpu memory pointers
  float* input_batch_;
  float* output_batch_;
  // input&output idx on buffers_
  int input_index_, output_index_;
  // default image mean value 
  float rgb_mean_[3] = {122.7717, 115.9465, 102.9801};
  float bgr_mean_[3] = {102.9801, 115.9465, 122.7717};
  // mean mat
  cv::Mat mean_;
  // model channel order
  bool is_bgr_;
  // input image standard variance
  float stdvar_;
  // threshold for each interested channel
  vector<double> thresholds_;
  // interested channel idx
  vector<int> interests_;
  // top k output
  int top_k_;
  // class labels list
  vector<std::string> labels_;
};
#endif //USE_TENSORRT
#ifdef USE_NOVURT
class Classifier {
 public:
  /* Construct function
  param @novu_model_file	novu model file path
	@caffe_model_file	caffe model prototxt
	@trained_file		pretrained model file(*.caffemodel)
	@mean_file		caffe model mean file, used in input normalization
	@label_file		model's label list file
  	@thresholds		is the thresholds for interests channels. 
				That means it has to have the same size of interests.
				If we want a topN output, then thresholds should be empty. 
  	@interests		is the output channel indice which we are interested,
  				which can be like [0, 1, 2...]
  				This is because for bi-classfication, we always want the output of channel 1,
  				but for multi-classification, we might want all output channels.
	@stdvar			stdvar is the standard variance of input data,
				some model's are trained with data normalized to N(0,1)
	@base_size		is the resize size of input image, if the preprocess include resize&crop.
				if only need resize, then base_size should be set to 0
	@top_k			for multi-classification softmax output, we want to get top_k output
	@is_bgr			to indicate whether the model is trained with (b, g, r) ordered image
  */
  Classifier(
      const string& novu_model_file, 
      const string& caffe_model_file, 
      const string& trained_file,
      const string& mean_file,
      const string& label_file,
      const vector<double>& thresholds,
      const vector<int>& interests,
      float stdvar = 1.0,
      int base_size = 0,
      int top_k = 1,
      bool is_bgr = true);
  // Classify batched images                     
  vector<Detail> Classify(const vector<cv::Mat>& imgs);
  // Get batch size of the model                         vector<std::shared_ptr<novu::Buffer<float>>>& ibuf,
   //   vector<std::shared_ptr<novu::Buffer<float>>>& obuf);
  int GetBatchSize() {
    return n_;
  }

    // const string& model_file,
    // const string& trained_file,
    // const string& mean_file,
    // const string& label_file,
    // const vector<double>& thresholds, 
    // const vector<int>& interests,
    // float stdvar, 
    // int base_size,
    // int top_k, 
    // bool is_bgr,


 private:
  // Set mean mat with mean_file
  void SetMean(const string& mean_file);
  // Set mean mat with default bgr_mean_ or rgb_mean_
  void SetMean();
  // Allocate input layer pointer to input image mat
  void WrapInputLayer(vector<vector<cv::Mat>>* input_batch);
  // Preprocess the images to fit-in input layer
  void Preprocess(const vector<cv::Mat>& imgs, vector<vector<cv::Mat>>* input_batch);
  // Get network output
  vector<vector<float>> Predict(const vector<cv::Mat>& mats);

 private:
  // net_ pointer
  std::shared_ptr<caffe::Net<float>> caffe_net_;
  novu::NNet* novu_net_;//std::unique_ptr<novu::NNet>
  std::shared_ptr<novu::Buffer<float>>  ibuf_;//  std::shared_ptr<novu::buffer_t>  std::vector<std::shared_ptr<novu::Buffer<float>>>
  std::shared_ptr<novu::Buffer<float>>  obuf_;//buffer_t
  // net_ size
  size_t n_, c_, h_, w_;
  size_t nnet_on_, nnet_oc_, nnet_oh_, nnet_ow_;
  int caffe_c_, caffe_h_, caffe_w_;
  int output_c_, output_h_, output_w_;
  // input image size
  cv::Size input_geometry_;
  // resize image base size
  int base_size_;
  // input_channel volume
  int vol_chl_;
  // novu net output volume
  int vol_novu_out_;
  // output blob volume
  int vol_output_;
  // class labels list
  vector<std::string> labels_;
  // default image mean value 
  float rgb_mean_[3] = {122.7717, 115.9465, 102.9801};
  float bgr_mean_[3] = {102.9801, 115.9465, 122.7717};
  // mean mat
  cv::Mat mean_;
  // model channel order
  bool is_bgr_;
  // input image standard variance
  float stdvar_;
  // threshold for each interested channel
  vector<double> thresholds_;
  // interested channel idx
  vector<int> interests_;
  // top k output
  int top_k_;
};
#endif // USE_NOVURT

}  // namespace common
}  // namespace novumind
#endif //COMMON_CLASSIFIER_H_
