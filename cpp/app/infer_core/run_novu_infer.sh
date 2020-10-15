#!/bin/bash

name=infer_core
image=registry.corp.novumind.com/dev/goku/fpga:latest
docker pull ${image}

docker rm -f ${name}
rm -rf build
params=$@

docker run -it \
  --name ${name} \
  -v /novumind/share:/novumind/share \
  -v `pwd`/../../../:/root/work \
  -v /dev/xdma0_c2h_0:/dev/xdma0_c2h_0 \
  -v /dev/xdma0_h2c_0:/dev/xdma0_h2c_0 \
  -v /tmp/.X11-unix/:/tmp/.X11-unix/ \
  --ipc=host \
  --network=host \
  --privileged \
  ${image} \
  /bin/bash -c "cd /root/work/cpp/app/infer_core && ./novu_locally.sh ${params}"
