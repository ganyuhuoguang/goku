#!/bin/bash

image=registry.corp.novumind.com/dev/common/go

params=$@

docker run -it \
    -v `pwd`/../../../..:/root/go/src/novumind/goku \
    --network=host ${image} \
    /bin/bash -c \
   "cd /root/go/src/novumind/goku/go/common/db/example&& \
    ./run_locally.sh $params"
