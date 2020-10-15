#!/bin/bash

# Generate proto codes for cpp.

output_dir="cpp_gen/"
proto_dir="."
mkdir -p ${output_dir} || exit 1
protoc --proto_path=${proto_dir} --cpp_out=${output_dir} \
  common.proto || exit 1

protoc --proto_path=${proto_dir} --cpp_out=${output_dir} \
  storage.proto || exit 1

protoc --proto_path=${proto_dir} --cpp_out=${output_dir} \
  --grpc_out=${output_dir} --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
  worker.proto || exit 1

protoc --proto_path=${proto_dir} --cpp_out=${output_dir} \
  --grpc_out=${output_dir} --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` \
  helloworld.proto || exit 1

