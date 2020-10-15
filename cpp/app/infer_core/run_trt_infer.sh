#!/bin/bash

name=tensorrt_infer
image=dev/goku/cpp-trt
docker pull ${image}

docker rm -f ${name}
rm -rf build
params=$@

nvidia-docker run -it \
  --name ${name} \
  -v /novumind/share:/novumind/share \
  -v `pwd`/../../../:/root/work \
  --ipc=host \
  --network=host \
  --privileged \
  ${image} \
  /bin/bash -c "cd /root/work/cpp/app/infer_core && ./trt_locally.sh ${params}"
