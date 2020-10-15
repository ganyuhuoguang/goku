#!/bin/bash

pwd=`pwd`
caffe_root=/home/xiaolonglu/caffe
cd ${caffe_root}/src/caffe/proto && protoc caffe.proto --cpp_out=. && \
#mkdir ${caffe_root}/include/caffe/proto && \
mv ${caffe_root}/src/caffe/proto/caffe.pb.h ${caffe_root}/include/caffe/proto || exit 1

cd ${pwd}/../../../proto && sh generate_cpp.sh || exit 1
cd ${pwd} && mkdir -p build  && cd build && \
  cmake -D BUILD_WITH_CUDA=OFF -D BUILD_WITH_CAFFE=ON -D BUILD_WITH_NOVURT=ON .. &&  make -j && echo "flags: $@" && \
  ./InferCore  $@
