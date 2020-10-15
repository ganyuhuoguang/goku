#include <gflags/gflags.h>
#include "common/handler.h"
#include "common/time.h"
#include <iostream>
#include <string>
#include <map>
#include <utility>

using novumind::goku::proto::worker::AnnotateVideoResponse;
using ConfFrameNum = std::pair<double, uint64_t>;
using ConfMap = std::map<std::string, ConfFrameNum>;

// output_interval need to multiples of batch_size
DEFINE_int32(output_interval, 64, "Output output_internal frames each time");

namespace novumind {
namespace common {

Handler::Handler(int max_chan_capacity, Pipeline* pipeline) :
    frame_chan_(max_chan_capacity), 
    processed_num_(0) ,
    pipeline_(pipeline) {
  batch_size_ = pipeline_->GetBatchSize();
  batch_num_ = FLAGS_output_interval / batch_size_;
}

bool Handler::GetOutput(AnnotateVideoResponse *resp) {
  if (total_frame_num_ == processed_num_) {
    return false;
  }
  vector<FrameMetadata> metadatas;
  for(int i = 0; i < batch_num_; i++) {
    FrameObj frame_obj;
    vector<FrameObj> objs;
    auto rest_frames = total_frame_num_ - processed_num_;
    auto process_size = (rest_frames < batch_size_) ? rest_frames : batch_size_;
    for(int j = 0; j < process_size; j++) {
      if (frame_chan_.Get(&frame_obj)) {
        objs.emplace_back(frame_obj);
        processed_num_++;
      }
    }
    pipeline_->Forward(objs, metadatas);
  }
  for (auto& metadata : metadatas) {
    *resp->add_metadatas() = metadata;
  }
  resp->set_total_num(total_frame_num_);
  resp->set_processed_num(processed_num_);
  return true;
};

void Handler::GetOutputSummary(VideoSummaryResponse *resp) {
  vector<FrameMetadata> metadatas;
  for (int i = 0; i < total_batch_num_; i++) {
    FrameObj frame_obj;
    vector<FrameObj> objs;
    auto rest_frames = total_frame_num_ - processed_num_;
    auto process_size = (rest_frames < batch_size_) ? rest_frames : batch_size_;
    for (int j = 0; j < process_size; j++) {
      if (frame_chan_.Get(&frame_obj)) {
        objs.emplace_back(frame_obj);
        processed_num_++;
      }
    }
    pipeline_->Forward(objs, metadatas);
  }
  ConfMap conf_map;
  for (auto& metadata : metadatas) {
    uint64_t frame_num = metadata.frame_num(); 
    for (auto& detail : metadata.details()) {
      if (detail.class_(0) != "normal") {
        std::string label = detail.class_(0);
        double conf = detail.confidence(0);
        if(conf_map.find(label) != conf_map.end()) {
          if(conf > conf_map[label].first) {
            conf_map[label] = std::make_pair(conf, frame_num);
          }
        } else {
          conf_map[label] = std::make_pair(conf, frame_num);
        }
      }
    }
  }
  std::cout << "conf_map size: " << conf_map.size() << std::endl;
  if (!conf_map.size()) {
    auto label_conf = resp->add_label_confs();
    label_conf->set_label("normal");
    label_conf->set_confidence(1.0);
    label_conf->set_frame_num(0);
  } else {
    for (auto iter = conf_map.begin(); iter != conf_map.end(); iter++) {
      auto label_conf = resp->add_label_confs();
      label_conf->set_label(iter->first);
      label_conf->set_confidence((iter->second).first);
      label_conf->set_frame_num((iter->second).second);
    }
  }
  resp->set_total_frame_num(total_frame_num_);
};

}  // namespace common
}  // namespace novumind
