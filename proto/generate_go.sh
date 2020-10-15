#!/bin/bash

# Generate proto codes for golang.
# The root of go src/ directory       ../../../   *src/novumind/goku/proto
output_dir="go_gen"
mkdir -p ${output_dir} || exit 1

if [[ ! "`pwd`" == /mnt/data/myproject/goku-master/proto ]]; then
  echo "ERROR: Not in go src directory."
  echo `pwd`
  exit 1
fi

protoc --proto_path=. -I. --go_out=plugins=grpc:${output_dir} api.proto || exit 1
protoc -I. --grpc-gateway_out=logtostderr=true:${output_dir} api.proto || exit 1
protoc -I. --swagger_out=logtostderr=true:go/api api.proto || exit 1

protoc --proto_path=. -I. --go_out=plugins=grpc:${output_dir} storage.proto || exit 1
protoc -I. --grpc-gateway_out=logtostderr=true:${output_dir} storage.proto || exit 1
protoc -I. --swagger_out=logtostderr=true:go/storage storage.proto || exit 1

protoc --proto_path=. -I. --go_out=plugins=grpc:${output_dir} msg.proto || exit 1
protoc -I. --grpc-gateway_out=logtostderr=true:${output_dir} msg.proto || exit 1
protoc -I. --swagger_out=logtostderr=true:go/msg msg.proto || exit 1

protoc --proto_path=. -I. --go_out=plugins=grpc:${output_dir} worker.proto || exit 1
protoc -I. --grpc-gateway_out=logtostderr=true:${output_dir} worker.proto || exit 1
protoc -I. --swagger_out=logtostderr=true:go/worker worker.proto || exit 1
