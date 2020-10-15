#!/bin/bash

image=registry.corp.novumind.com/dev/goku/go

params=$@

docker run -it \
    -v `pwd`/../..:/root/go/src/novumind/goku \
    -v /novumind/share/video-sample:/novumind/share/video-sample/ \
    --network=host ${image} \
    /bin/bash -c \
   "cd /root/go/src/novumind/goku/go/init_demo/&& \
    ./run_locally.sh $params"
