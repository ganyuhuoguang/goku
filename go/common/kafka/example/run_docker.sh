#!/bin/bash

image=registry.corp.novumind.com/dev/goku/go

params=$@

docker run -it \
    -v /novumind/home/lt/git/goku/:/root/go/src/novumind/goku \
    --network=host ${image} \
    /bin/bash -c \
   "cd /root/go/src/novumind/goku/go/common/kafka/example/ && \
    ./run_locally.sh $params"
