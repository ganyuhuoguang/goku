#ifndef COMMON_OPENCV_UTIL_H_
#define COMMON_OPENCV_UTIL_H_

#include <cuda.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/cuda.hpp>
#include <iostream>
#include <string>

using std::string;
using std::endl;
using std::cout;

namespace novumind {
namespace common {

inline std::string TypeString(int number) {
    int img_type_int = number % 8;
    std::string img_type_string;
    switch (img_type_int) {
        case 0:
            img_type_string = "8U";
            break;
        case 1:
            img_type_string = "8S";
            break;
        case 2:
            img_type_string = "16U";
            break;
        case 3:
            img_type_string = "16S";
            break;
        case 4:
            img_type_string = "32S";
            break;
        case 5:
            img_type_string = "32F";
            break;
        case 6:
            img_type_string = "64F";
            break;
        default:
            break;
    }
    // find channel
    int channel = number / 8 + 1;
    std::stringstream type;
    type<<"CV_"<<img_type_string<<"C"<<channel;
    return type.str();
}

inline void PrintMat(cv::Mat image) {
  cout<<"=================="<<endl;
  cout<<"cols: "<<image.cols<<endl;
  cout<<"rows: "<<image.rows<<endl;
  cout<<"size: "<<image.size()<<endl;
  cout<<"step: "<<image.step<<endl;
  cout<<"elem: "<<image.elemSize()<<endl;
  cout<<"type: "<<TypeString(image.type())<<endl;
  cout<<"continuous: "<<image.isContinuous()<<endl;
  cout<<"data: "<<(uint64_t)image.data<<endl;
  cout<<"data start: "<<(uint64_t)image.datastart<<endl;
  cout<<"data end: "<<(uint64_t)image.dataend<<endl;
}

inline void PrintGpuMat(cv::cuda::GpuMat image) {
  cout<<"=================="<<endl;
  cout<<"cols: "<<image.cols<<endl;
  cout<<"rows: "<<image.rows<<endl;
  cout<<"size: "<<image.size()<<endl;
  cout<<"step: "<<image.step<<endl;
  cout<<"elem: "<<image.elemSize()<<endl;
  cout<<"type: "<<TypeString(image.type())<<endl;
  cout<<"continuous: "<<image.isContinuous()<<endl;
  cout<<"data: "<<(uint64_t)image.data<<endl;
  cout<<"data start: "<<(uint64_t)image.datastart<<endl;
  cout<<"data end: "<<(uint64_t)image.dataend<<endl;
}

inline void WriteGpuImage(string path, cv::cuda::GpuMat image) {
  cv::Mat mat;
  image.download(mat);
  cv::imwrite(path, mat);
}

}  // namespace common
}  // namespace novumind
#endif  // COMMON_OPENCV_UTIL_H_

