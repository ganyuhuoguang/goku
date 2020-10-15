#include <gflags/gflags.h>
#include "common/classifier.h"
#include "common/math_util.h"
#include "common/file_util.h"
#include <iostream>
#ifdef USE_TENSORRT
#include "common/tensorrt/convert.h"
#include <cstring>
#endif // USE_TENSORRT


// all models need to have the same batch size, 
// batch size is only used for caffe models, not able for TensorRT
DEFINE_int32(batch_size, 8, "Forward use batch size");

namespace novumind {
namespace common {

#ifdef USE_CAFFE
Classifier::Classifier(
    const string& model_file,
    const string& trained_file,
    const string& mean_file,
    const string& label_file,
    const vector<double>& thresholds, 
    const vector<int>& interests,
    float stdvar, 
    int base_size,
    int top_k, 
    bool is_bgr) : 
    thresholds_(thresholds), interests_(interests), stdvar_(stdvar),
    base_size_(base_size), is_bgr_(is_bgr) {
// set inference mode, caffe can choose gpu or cpu
#ifdef USE_GPU
  Caffe::set_mode(Caffe::GPU);
#else
  Caffe::set_mode(Caffe::CPU);
#endif // USE_GPU
  // Load the network.
  net_.reset(new Net<float>(model_file, TEST));
  net_->CopyTrainedLayersFrom(trained_file);
  // Get input shape
  n_ = FLAGS_batch_size;
  Blob<float>* input_layer = net_->input_blobs()[0];
  c_ = input_layer->channels();
  CHECK_EQ(c_, 3) << "Input layer should have 3 channels.";
  w_ = input_layer->width();
  h_ = input_layer->height();
  vol_chl_ = w_ * h_;
  input_geometry_ = cv::Size(w_, h_);
  // Get output shape
  Blob<float>* output_layer = net_->output_blobs()[0];
  output_c_ = output_layer->channels();
  auto output_width = output_layer->width();
  auto output_height = output_layer->height();
  vol_output_ = output_width * output_height * output_c_;
  // Reshape the net for batch process
  input_layer->Reshape(n_, c_, h_, w_);
  net_->Reshape();
  // Load the binaryproto mean file. 
  if (mean_file == "") {
    SetMean();
  } else {
    SetMean(mean_file);
  }
  // Load labels.
  labels_ = ReadFileByLine(label_file);
  CHECK_EQ(labels_.size(), output_c_)
    << "Number of labels is different from the output layer dimension.";
  // Check thresholds length & interests length
  CHECK_EQ(thresholds_.size(), interests_.size())
    << "thresholds & intersts have different size.";
  // Set output top k
  top_k_ = (labels_.size() < top_k) ? labels_.size() : top_k;
}

/* Load the mean file in binaryproto format. */
void Classifier::SetMean(const string& mean_file) {
  // Read mean file
  BlobProto blob_proto;
  ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);
  // Convert from BlobProto to Blob<float>
  Blob<float> mean_blob;
  mean_blob.FromProto(blob_proto);
  CHECK_EQ(mean_blob.channels(), c_)
    << "Number of channels of mean file doesn't match input layer.";
  // The format of the mean file is planar 32-bit float BGR or grayscale.
  vector<cv::Mat> channels;
  float* data = mean_blob.mutable_cpu_data();
  for (int i = 0; i < c_; ++i) {
    // Extract an individual channel.
    cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
    channels.push_back(channel);
    data += mean_blob.height() * mean_blob.width();
  }
  // Merge the separate channels into a single image.
  cv::Mat mean;
  cv::merge(channels, mean);
  // Compute the global mean pixel value and create a mean image
  // filled with this value.
  cv::Scalar channel_mean = cv::mean(mean);
  mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);
  // std::cout << mean_ << std::endl;
}

void Classifier::SetMean() {
  // create RGB mean channels
  vector<cv::Mat> mean_channels;
  // if input order is bgr
  if (is_bgr_) {
    for (int i=0; i<3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(bgr_mean_[i]));
      mean_channels.push_back(channel);
    }
  // if input order is rgb
  } else {
    for (int i=0; i<3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(rgb_mean_[i]));
      mean_channels.push_back(channel);
    }
  }
  // merge the seperate channels into a single image
  cv::merge(mean_channels, mean_);
}

void Classifier::WrapInputLayer(vector<vector<cv::Mat>>* input_batch) {
  // get input layer pointer
  Blob<float>* input_layer = net_->input_blobs()[0];
  float* input_data = input_layer->mutable_cpu_data();
  // allocate input_batch memory
  for (int j = 0; j < n_; j++) {
    vector<cv::Mat> input_channels;
    // allocate one channel's memory pointer
    for (int i = 0; i < c_; ++i) {
      cv::Mat channel(h_, w_, CV_32FC1, input_data);
      input_channels.push_back(channel);
      input_data += vol_chl_;
    }
    // collect all pointers to input_batch
    input_batch->push_back(input_channels);
  }
}

void Classifier::Preprocess(const vector<cv::Mat>& imgs,
    vector<vector<cv::Mat>>* input_batch) {
    /* Convert the input image to the input image format of the network. */
  for (int i = 0; i < n_; ++i) {
    // get image, if it is empty, then get an all-zero image for it
    // this may happen in the last batch
    cv::Mat sample;
    if (i < imgs.size()) {
      sample = imgs[i];
    } else {
      if (base_size_ != 0) {
        sample = cv::Mat(cv::Size(base_size_, base_size_), CV_8UC3, cv::Scalar(0, 0, 0));
      } else {
        sample = cv::Mat(input_geometry_, CV_8UC3, cv::Scalar(0, 0, 0));
      }
    }
    // transfer channel order
    if (!is_bgr_) {
      cv::cvtColor(sample, sample, cv::COLOR_BGR2RGB);
    }
    cv::Mat sample_resized;
    // if base size is not 0, first resize then crop
    if (base_size_ != 0 ) {
      // calculate resize size & resize the image
      float short_edge = float ((sample.cols < sample.rows) ? sample.cols : sample.rows);
      float resize_ratio = float(base_size_) / short_edge;
      cv::Size resize_size =  cv::Size(
          int(resize_ratio * sample.cols), int(resize_ratio * sample.rows));
      cv::resize(sample, sample_resized, resize_size);
      // crop middle for input geometry
      auto xx = int((sample_resized.cols - w_) / 2);
      auto yy = int((sample_resized.rows - h_) / 2);
      auto crop_rect = cv::Rect(xx, yy, w_, h_);
      sample_resized = sample_resized(crop_rect);
    // if base size is 0
    } else {
      // resize the image
      cv::resize(sample, sample_resized, input_geometry_);
    }
    // convert cv mat from CV_8UC3 to CV_32FC3
    cv::Mat sample_float;
    sample_resized.convertTo(sample_float, CV_32FC3);
    cv::Mat sample_normalized;
    // normalize image, subtracted by mean mat and divided by stdvar_
    cv::subtract(sample_float, mean_, sample_normalized);
    sample_normalized /= stdvar_;
    // get input mat pointer
    vector<cv::Mat>* input_channels = &(input_batch->at(i));
    // split input into channels to fit-in mat pointer
    cv::split(sample_normalized, *input_channels);
  }
}

vector<vector<float>> Classifier::Predict(const vector<cv::Mat>& imgs) {
// caffe need to set gpu mode when construction& predict are run in different threads
#ifdef USE_GPU
  Caffe::set_mode(Caffe::GPU);
#else
  Caffe::set_mode(Caffe::CPU);
#endif // USE_GPU
  // declaration of input batch vector
  vector<vector<cv::Mat>> input_batch;
  // call the function to allocate image pointers to net input pointer
  WrapInputLayer(&input_batch);
  // do preprocess
  Preprocess(imgs, &input_batch);
  // call net forwarding
  net_->Forward();
  // Copy the output layer to a std::vector
  Blob<float>* output_layer = net_->output_blobs()[0];
  vector<vector<float>> outputs;
  const float* begin = output_layer->cpu_data();
  for (int i = 0; i < imgs.size(); ++i) {
    // calculate output pointer for each image
    const float* local_begin = begin + i * vol_output_;
    const float* local_end = local_begin + vol_output_;
    // push data into a vector
    outputs.push_back(vector<float>(local_begin, local_end));
  }
  return outputs;
}

vector<Detail> Classifier::Classify(const vector<cv::Mat>& imgs) {
  // get network output
  auto outputs = Predict(imgs);
  vector<Detail> results;
  for (auto& output:outputs) {
    // if interest_ is empty means to do multi-cls, and return a top k result
    if (interests_.empty()) {
      // calculate top k output
      vector<int> maxN = Argmax(output, top_k_);
      Detail result;
      // push top k output into result
      for (int i = 0; i < top_k_; ++i) {
        int idx = maxN[i];
        result.add_class_(labels_[idx]);
        result.add_confidence(output[idx]);
      }
      results.push_back(result);
    // if interest_ is not empty means we focus on the interest channels
    } else {
      Detail result;
      // push interest channel to result
      for (int i = 0; i < thresholds_.size(); i++) {
        auto confidence = output[interests_[i]];
        if (confidence > thresholds_[i]) {
          result.add_class_(labels_[interests_[i]]);
          result.add_confidence(confidence);
        }
      }
      results.push_back(result);
    }
  }
  return results;
}

#endif // USE_CAFFE
#ifdef USE_TENSORRT
Classifier::Classifier(
    const string& engine_file,
    const string& output_blob_file,
    const string& mean_file,
    const string& label_file,
    const vector<double>& thresholds, 
    const vector<int>& interests,
    float stdvar,
    int base_size,
    int top_k,
    bool is_bgr) :
    thresholds_(thresholds), interests_(interests), 
    stdvar_(stdvar), is_bgr_(is_bgr), base_size_(base_size){
  // read engine file
  // To generate an engine file, you can use a tool in cpp/tools/trt_engine_gen
  std::ifstream in_file(engine_file.c_str(), std::ios::in | std::ios::binary);
  if (!in_file.is_open()) {
    std::cout << "Failed 1to open engine file" << std::endl;
    exit(1);
  }
  std::cout << "1" << std::endl;
  std::streampos begin, end;
  begin = in_file.tellg();
  in_file.seekg(0, std::ios::end);
  end = in_file.tellg();
  std::size_t size = end - begin;
  in_file.seekg(0, std::ios::beg);
  std::unique_ptr<unsigned char[]> engine_data(new unsigned char[size]);
  in_file.read((char*)engine_data.get(), size);
  in_file.close();
  // create a engine
  runtime_ = nvinfer1::createInferRuntime(gLogger);
  engine_ = runtime_->deserializeCudaEngine(
     (const void*)engine_data.get(), size, nullptr);
  if (engine_ == nullptr) {
    std::cout << "Fail in rebuilding engine..." << std::endl;
  }
  context_ = engine_->createExecutionContext();
  if (context_ == nullptr) {
    std::cout << "Fail in creating execution context..." << std::endl;
  }
  std::cout << "2" << std::endl;
  // get engine's max batch size
  n_ = engine_->getMaxBatchSize();
  std::cout << "7" << std::endl;
  // get input index on buffers_
  input_index_ = engine_->getBindingIndex("data");
  // get output_blob_names, then get output index on buffers_
  std::cout << output_blob_file << std::endl;
  auto output_blob_names = ReadFileByLine(output_blob_file);
  std::cout << "9" << std::endl;
  std::cout << output_blob_names[0].c_str() << std::endl;
  output_index_ = engine_->getBindingIndex(output_blob_names[0].c_str());
  std::cout << "6" << std::endl;
  // get input& output size
  Dims3 input_dims = static_cast<Dims3&&>(engine_->getBindingDimensions(input_index_));
  Dims3 output_dims = static_cast<Dims3&&>(engine_->getBindingDimensions(output_index_));
  c_ = input_dims.d[0];
  h_ = input_dims.d[1];
  w_ = input_dims.d[2];
  output_c_ = output_dims.d[0];
  output_h_ = output_dims.d[1];
  output_w_ = output_dims.d[2];
  input_geometry_ = cv::Size(w_, h_);
  std::cout << "4" << std::endl;
  // caculate the volume of (image, channel & output)
  vol_chl_ = h_ * w_;
  int vol_img = c_ * vol_chl_;
  vol_output_ = output_c_ * output_h_ * output_w_;
  // allocate gpu memory for buffers_
  input_size_ = vol_img * n_ * sizeof(float);
  output_size_ = output_c_ * output_h_ * output_w_ * n_ * sizeof(float);
  std::cout << "5" << std::endl;
  CHECK(cudaMalloc(&buffers_[input_index_], input_size_));
  CHECK(cudaMalloc(&buffers_[output_index_], output_size_));
  // allocate cpu memory for input_batch_ & output_batch_;
  input_batch_ = new float[vol_img * n_];
  output_batch_ = new float[vol_output_ * n_];
  // initialize cuda stream
  CHECK(cudaStreamCreate(&stream_));
  // load labels
  labels_ = ReadFileByLine(label_file);
  // set mean
  SetMean(mean_file);
  std::cout << "3" << std::endl;
  /* Check thresholds length & interests length*/
  if (thresholds_.size() != interests_.size()) {
    std::cout << "thresholds & intersts have different size." << std::endl;
    exit(1);
  }
  top_k_ = (labels_.size() < top_k) ? labels_.size() : top_k;
}

void Classifier::SetMean(const string& mean_file) {
  // create RGB mean mat from mean list file 
  vector<string> mean_vec = ReadFileByLine(mean_file);
  for (int i = 0; i < mean_vec.size(); i++) {
    if (is_bgr_) {
      bgr_mean_[i] = atof(mean_vec[i].c_str());
    } else {
      rgb_mean_[i] = atof(mean_vec[i].c_str());
    }
  }
  vector<cv::Mat> mean_channels;
  // if the channel order is bgr
  if (is_bgr_) {
    for (int i=0; i < 3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(bgr_mean_[i]));
      mean_channels.push_back(channel);
    }
  // if the channel order is rgb
  } else {
    for (int i=0; i < 3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(rgb_mean_[i]));
      mean_channels.push_back(channel);
    }
  }
  cv::merge(mean_channels, mean_);
}

void Classifier::WrapInputLayer(vector<vector<cv::Mat>>* input_batch) {
  // allocate input_batch memory
  float* input_data = input_batch_;
  for (int j = 0; j < n_; j++) {
    vector<cv::Mat> input_channels;
    // allocate one channel's memory pointer
    for (int i = 0; i < c_; ++i) {
      cv::Mat channel(h_, w_, CV_32FC1, input_data);
      input_channels.push_back(channel);
      input_data += vol_chl_;
    }
    // collect all pointers to input_batch
    input_batch->push_back(input_channels);
  }
}

void Classifier::Preprocess(const vector<cv::Mat>& imgs, vector<vector<cv::Mat>>* input_batch) {
  for (int i = 0; i < n_; i++) {
    // get image, if not exists, then use an all-zero mat instead
    cv::Mat sample;
    if (i < imgs.size()) {
      sample = imgs[i];
    } else {
      if (base_size_ != 0) {
        sample = cv::Mat(cv::Size(base_size_, base_size_), CV_8UC3, cv::Scalar(0, 0, 0));
      } else {
        sample = cv::Mat(input_geometry_, CV_8UC3, cv::Scalar(0, 0, 0));
      }
    }
    // change channel order
    if (!is_bgr_) {
      cv::cvtColor(sample, sample, cv::COLOR_BGR2RGB);
    }
    cv::Mat sample_resized;
    // if base_size is not 0, resize first then crop 
    if (base_size_ != 0) {
      float short_edge = float ((sample.cols < sample.rows) ? sample.cols : sample.rows);
      float resize_ratio = float(base_size_) / short_edge;
      cv::Size resize_size =  cv::Size(
          int(resize_ratio * sample.cols), int(resize_ratio * sample.rows));
      cv::resize(sample, sample_resized, resize_size);
      auto xx = int((sample_resized.cols - w_) / 2);
      auto yy = int((sample_resized.rows - h_) / 2);
      auto crop_rect = cv::Rect(xx, yy, w_, h_);
      sample_resized = sample_resized(crop_rect);
    } else {
      // if base size is 0, resize the input
      cv::resize(sample, sample_resized, input_geometry_);
    }
    cv::Mat sample_float;
    sample_resized.convertTo(sample_float, CV_32FC3);
    // normalize image, first subtracted by mean mat, then divided by standard variance
    cv::Mat sample_normalized;
    cv::subtract(sample_float, mean_, sample_normalized);
    sample_normalized /= stdvar_;
    // get input mat pointer
    vector<cv::Mat>* input_channels = &(input_batch->at(i));
    // split input into channels to fit-in mat pointer
    cv::split(sample_normalized, *input_channels);
  }
}

vector<vector<float>> Classifier::Predict(const vector<cv::Mat>& imgs) {
  // declaration of input batch vector
  vector<vector<cv::Mat>> input_batch;
  // call the function to allocate image pointers to net input pointer
  WrapInputLayer(&input_batch);
  // do preprocess
  Preprocess(imgs, &input_batch);
  // copy input data from host to gpu device
  CHECK(cudaMemcpyAsync(
      buffers_[input_index_], input_batch_, input_size_, cudaMemcpyHostToDevice, stream_));
  // do inference on gpu
  context_->enqueue(n_, buffers_, stream_, nullptr);
  // copy output data from gpu device to host
  CHECK(cudaMemcpyAsync(
      output_batch_, buffers_[output_index_], output_size_, cudaMemcpyDeviceToHost, stream_));
  // do sync
  cudaStreamSynchronize(stream_);
  // copy output data to vector
  vector<vector<float>> outputs;
  for (int i = 0; i < imgs.size(); ++i) {
    const float* local_begin = output_batch_ + i * vol_output_;
    const float* local_end = local_begin + vol_output_;
    outputs.push_back(vector<float>(local_begin, local_end));
  }
  return outputs;
}

vector<Detail> Classifier::Classify(const vector<cv::Mat>& imgs) {
  // get network output
  auto outputs = Predict(imgs);
  vector<Detail> results;
  for (auto& output:outputs) {
    // if interest_ is empty means to do multi-cls, and return a top k result
    if (interests_.empty()) {
      // calculate top k output
      vector<int> maxN = Argmax(output, top_k_);
      Detail result;
      // push top k output into result
      for (int i = 0; i < top_k_; ++i) {
        int idx = maxN[i];
        result.add_class_(labels_[idx]);
        result.add_confidence(output[idx]);
      }
      results.push_back(result);
    // if interest_ is not empty means we focus on the interest channels
    } else {
      Detail result;
      // push interest channel to result
      for (int i = 0; i < thresholds_.size(); i++) {
        auto confidence = output[interests_[i]];
        if (confidence > thresholds_[i]) {
          result.add_class_(labels_[interests_[i]]);
          result.add_confidence(confidence);
        }
      }
      results.push_back(result);
    }
  }
  return results;
}

#endif // USE_TENSORRT

#ifdef USE_NOVURT
Classifier::Classifier(
    const string& novu_model_file,
    const string& caffe_model_file,
    const string& trained_file,
    const string& mean_file,
    const string& label_file,
    const vector<double>& thresholds, 
    const vector<int>& interests,
    float stdvar, 
    int base_size,
    int top_k, 
    bool is_bgr) : 
    thresholds_(thresholds), interests_(interests), stdvar_(stdvar),
    base_size_(base_size), is_bgr_(is_bgr), 
    ibuf_(new novu::Buffer<float>), obuf_(new novu::Buffer<float>) {
  Caffe::set_mode(Caffe::CPU);
  // Load the network.         novu::buffer_t
  std::cout << "creating nnet!" << std::endl;
  novu_net_ = novu::CreateNNet(novu_model_file.c_str());
  if (novu_net_ == NULL) {
    std::cout << "nnet creation failed!" << std::endl;
    exit(1);
  }
  std::cout << "nnet created, initializing" << std::endl;
  //int init_state = novu_net_->Init();
  //std::cout << "nnet initialized with return" << init_state << std::endl;
  caffe_net_.reset(new Net<float>(caffe_model_file, TEST));
  caffe_net_->CopyTrainedLayersFrom(trained_file);
  // Get novu_net_'s input&output shape
  novu_net_->get_ifm_dims(n_, c_, h_, w_);
  novu_net_->get_ofm_dims(nnet_on_, nnet_oc_, nnet_oh_, nnet_ow_);
  vol_chl_ = w_ * h_;
  input_geometry_ = cv::Size(w_, h_);
  // Allocate novu_net_'s input&output buffer
  ibuf_->Alloc(n_, c_, h_, w_);
  obuf_->Alloc(nnet_on_, nnet_oc_, nnet_oh_, nnet_ow_);
  vol_novu_out_ = nnet_on_ * nnet_oc_ * nnet_oh_ * nnet_ow_ * sizeof(float);
  // Get caffe_net_'s input shape
  Blob<float>* input_layer = caffe_net_->input_blobs()[0];
  caffe_c_ = input_layer->channels();
  caffe_w_ = input_layer->width();
  caffe_h_ = input_layer->height();
  CHECK_EQ(caffe_c_, nnet_oc_)
    << "novu net output channel is not equal to caffe net input channel";
  CHECK_EQ(caffe_h_, nnet_oh_)
    << "novu net output channel is not equal to caffe net input channel";
  CHECK_EQ(caffe_w_, nnet_ow_)
    << "novu net output channel is not equal to caffe net input channel";
  input_layer->Reshape(n_, caffe_c_, caffe_h_, caffe_w_);
  caffe_net_->Reshape();
  // Get output shape
  Blob<float>* output_layer = caffe_net_->output_blobs()[0];
  output_c_ = output_layer->channels();
  output_w_ = output_layer->width();
  output_h_ = output_layer->height();
  vol_output_ = output_w_ * output_h_ * output_c_;
  // Load the binaryproto mean file. 
  if (mean_file == "") {
    SetMean();
  } else {
    SetMean(mean_file);
  }
  // Load labels.
  labels_ = ReadFileByLine(label_file);
  CHECK_EQ(labels_.size(), output_c_)
    << "Number of labels is different from the output layer dimension.";
  // Check thresholds length & interests length
  CHECK_EQ(thresholds_.size(), interests_.size())
    << "thresholds & intersts have different size.";
  // Set output top k
  top_k_ = (labels_.size() < top_k) ? labels_.size() : top_k;
}

/* Load the mean file in binaryproto format. */
void Classifier::SetMean(const string& mean_file) {
  // Read mean file
  BlobProto blob_proto;
  ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);
  // Convert from BlobProto to Blob<float>
  Blob<float> mean_blob;
  mean_blob.FromProto(blob_proto);
  CHECK_EQ(mean_blob.channels(), c_)
    << "Number of channels of mean file doesn't match input layer.";
  // The format of the mean file is planar 32-bit float BGR or grayscale.
  vector<cv::Mat> channels;
  float* data = mean_blob.mutable_cpu_data();
  for (int i = 0; i < c_; ++i) {
    // Extract an individual channel.
    cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
    channels.push_back(channel);
    data += mean_blob.height() * mean_blob.width();
  }
  // Merge the separate channels into a single image.
  cv::Mat mean;
  cv::merge(channels, mean);
  // Compute the global mean pixel value and create a mean image
  // filled with this value.
  cv::Scalar channel_mean = cv::mean(mean);
  mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);
  // std::cout << mean_ << std::endl;
}

void Classifier::SetMean() {
  // create RGB mean channels
  vector<cv::Mat> mean_channels;
  // if input order is bgr
  if (is_bgr_) {
    for (int i=0; i<3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(bgr_mean_[i]));
      mean_channels.push_back(channel);
    }
  // if input order is rgb
  } else {
    for (int i=0; i<3; i++) {
      cv::Mat channel(h_, w_, CV_32FC1, cv::Scalar(rgb_mean_[i]));
      mean_channels.push_back(channel);
    }
  }
  // merge the seperate channels into a single image
  cv::merge(mean_channels, mean_);
}

void Classifier::WrapInputLayer(vector<vector<cv::Mat>>* input_batch) {
  // get input layer pointer
  //Blob<float>* input_layer = net_->input_blobs()[0];
  //float* input_data = input_layer->mutable_cpu_data();
  float* input_data = **ibuf_;
  // allocate input_batch memory
  for (int j = 0; j < n_; j++) {
    vector<cv::Mat> input_channels;
    // allocate one channel's memory pointer
    for (int i = 0; i < c_; ++i) {
      cv::Mat channel(h_, w_, CV_32FC1, input_data);
      input_channels.push_back(channel);
      input_data += vol_chl_;
    }
    // collect all pointers to input_batch
    input_batch->push_back(input_channels);
  }
}

void Classifier::Preprocess(const vector<cv::Mat>& imgs,
    vector<vector<cv::Mat>>* input_batch) {
    /* Convert the input image to the input image format of the network. */
  for (int i = 0; i < n_; ++i) {
    // get image, if it is empty, then get an all-zero image for it
    // this may happen in the last batch
    cv::Mat sample;
    if (i < imgs.size()) {
      sample = imgs[i];
    } else {
      if (base_size_ != 0) {
        sample = cv::Mat(cv::Size(base_size_, base_size_), CV_8UC3, cv::Scalar(0, 0, 0));
      } else {
        sample = cv::Mat(input_geometry_, CV_8UC3, cv::Scalar(0, 0, 0));
      }
    }
    // transfer channel order
    if (!is_bgr_) {
      cv::cvtColor(sample, sample, cv::COLOR_BGR2RGB);
    }
    cv::Mat sample_resized;
    // if base size is not 0, first resize then crop
    if (base_size_ != 0 ) {
      // calculate resize size & resize the image
      float short_edge = float ((sample.cols < sample.rows) ? sample.cols : sample.rows);
      float resize_ratio = float(base_size_) / short_edge;
      cv::Size resize_size =  cv::Size(
          int(resize_ratio * sample.cols), int(resize_ratio * sample.rows));
      cv::resize(sample, sample_resized, resize_size);
      // crop middle for input geometry
      auto xx = int((sample_resized.cols - w_) / 2);
      auto yy = int((sample_resized.rows - h_) / 2);
      auto crop_rect = cv::Rect(xx, yy, w_, h_);
      sample_resized = sample_resized(crop_rect);
    // if base size is 0
    } else {
      // resize the image
      cv::resize(sample, sample_resized, input_geometry_);
    }
    // convert cv mat from CV_8UC3 to CV_32FC3
    cv::Mat sample_float;
    sample_resized.convertTo(sample_float, CV_32FC3);
    cv::Mat sample_normalized;
    // normalize image, subtracted by mean mat and divided by stdvar_
    cv::subtract(sample_float, mean_, sample_normalized);
    sample_normalized /= stdvar_;
    // get input mat pointer
    vector<cv::Mat>* input_channels = &(input_batch->at(i));
    // split input into channels to fit-in mat pointer
    cv::split(sample_normalized, *input_channels);
  }
}

vector<vector<float>> Classifier::Predict(const vector<cv::Mat>& imgs) {
  Caffe::set_mode(Caffe::CPU);
  // declaration of input batch vector
  vector<vector<cv::Mat>> input_batch;
  // call the function to allocate image pointers to net input pointer
  WrapInputLayer(&input_batch);
  // do preprocess
  Preprocess(imgs, &input_batch);
  // call net forwarding
  novu_net_->SetInput(ibuf_);
  novu_net_->GetOutput(obuf_);
  const float* novu_output = **obuf_;
  caffe::Blob<float>* input_layer = caffe_net_->input_blobs()[0];
  float* caffe_input_data = input_layer->mutable_cpu_data();
  memcpy(caffe_input_data, novu_output, vol_novu_out_);
  caffe_net_->Forward();
  // Copy the output layer to a std::vector
  Blob<float>* output_layer = caffe_net_->output_blobs()[0];
  vector<vector<float>> outputs;
  const float* begin = output_layer->cpu_data();
  for (int i = 0; i < imgs.size(); ++i) {
    // calculate output pointer for each image
    const float* local_begin = begin + i * vol_output_;
    const float* local_end = local_begin + vol_output_;
    // push data into a vector
    outputs.push_back(vector<float>(local_begin, local_end));
  }
  return outputs;
}

vector<Detail> Classifier::Classify(const vector<cv::Mat>& imgs) {
  // get network output
  auto outputs = Predict(imgs);
  vector<Detail> results;
  for (auto& output:outputs) {
    // if interest_ is empty means to do multi-cls, and return a top k result
    if (interests_.empty()) {
      // calculate top k output
      vector<int> maxN = Argmax(output, top_k_);
      Detail result;
      // push top k output into result
      for (int i = 0; i < top_k_; ++i) {
        int idx = maxN[i];
        result.add_class_(labels_[idx]);
        result.add_confidence(output[idx]);
      }
      results.push_back(result);
    // if interest_ is not empty means we focus on the interest channels
    } else {
      Detail result;
      // push interest channel to result
      for (int i = 0; i < thresholds_.size(); i++) {
        auto confidence = output[interests_[i]];
        if (confidence > thresholds_[i]) {
          result.add_class_(labels_[interests_[i]]);
          result.add_confidence(confidence);
        }
      }
      results.push_back(result);
    }
  }
  return results;
}

#endif // USE_NOVURT
}  // namespace common
}  // namespace novumind
