#ifndef COMMON_TENSORRT_CONVERT_H_ 
#define COMMON_TENSORRT_CONVERT_H_ 

#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <sys/stat.h>
#include <cmath>
#include <time.h>
#include <cuda_runtime_api.h>
#include <memory>
#include <cstring>
#include <algorithm>

#include "NvCaffeParser.h"
#include "NvInferPlugin.h"
#include "common/tensorrt/common.h"

static Logger gLogger;
using namespace nvinfer1;
using namespace nvcaffeparser1;
using namespace plugin;

namespace novumind {
namespace common {
namespace tensorrt {

/********The function to transfer a caffe model to TensorRT engine********/
// @param:	model_file		caffe_prototxt file path
// @param:	pretraied_file		caffemodel file path
// @param:	outputs			network outputs
// @param:	batch_size		batch size which we want to run with
// @param:	plugin_factory		factory for plugin layers
// @param:	trt_model_stream	output stream for TensorRT model
inline bool CaffeToTRTModel(
    const std::string& model_file,
    const std::string& pretrained_file,
    const std::vector<std::string>& outputs,
    unsigned int batch_size,
    nvcaffeparser1::IPluginFactory* plugin_factory,
    nvinfer1::IHostMemory*& trt_model_stream) {
  
  // create the builder
  nvinfer1::IBuilder* builder = createInferBuilder(gLogger);
  
  // parse the caffe model to populate the network, then set the outputs
  nvinfer1::INetworkDefinition* network = builder->createNetwork();
  nvcaffeparser1::ICaffeParser* parser = nvcaffeparser1::createCaffeParser();
  if (plugin_factory != nullptr) {
    parser->setPluginFactory(plugin_factory);
  }
  
  std::cout << "Begin parsing model..." << std::endl;
  const nvcaffeparser1::IBlobNameToTensor* blob_name_to_tensor = parser->parse(
      model_file.c_str(),
      pretrained_file.c_str(),
      *network,
      DataType::kFLOAT);
  std::cout << "End parsing model..." << std::endl;
  // specify which tensors are outputs
  for (auto& s : outputs) {
    network->markOutput(*blob_name_to_tensor->find(s.c_str()));
  }
  
  // Build the engine
  builder->setMaxBatchSize(batch_size);
  builder->setMaxWorkspaceSize(16 << 20);
  
  std::cout << "Begin building engine..." << std::endl;

  // last_layer = network->getLayer(network.num_layers - 1)
  // if not last_layer.get_output(0):
  // network.mark_output(network.get_layer(network.num_layers - 1).get_output(0))

  nvinfer1::ICudaEngine* engine = builder->buildCudaEngine(*network);
  if (engine == nullptr) {
    std::cout << "Fail in building engine..." << std::endl;
    return false;
  }
  std::cout << "End building engine..." << std::endl;
  
  // we don't need the network any more, and we can destroy the parser
  network->destroy();
  parser->destroy();
  
  // serialize the engine, then close everything down
  trt_model_stream = engine->serialize();
  engine->destroy();
  builder->destroy();
  nvcaffeparser1::shutdownProtobufLibrary();
  return true;
}

inline bool SaveTRTModel(
    const std::string& engine_file, 
    nvinfer1::IHostMemory* trt_model_stream) {
  std::ofstream outfile(engine_file.c_str(), std::ios::out | std::ios::binary);
  if (!outfile.is_open()) {
    std::cout << "Failed to open engine file" << std::endl;
    return false;
  }
  unsigned char* p = (unsigned char*)(trt_model_stream->data());
  outfile.write((char*)p, trt_model_stream->size());
  outfile.close();
  return true;
}

} // namespace tensorrt
} // namespace common
} // namespace novumind
#endif // COMMON_TENSORRT_CONVERT_H_ 
