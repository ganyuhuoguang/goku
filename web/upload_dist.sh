#!/bin/bash
dist=$1
image=registry.corp.novumind.com/dev/common/node
docker pull $image

docker run -it \
  -v `pwd`:/src \
  -w /src \
	--network=host \
	${image} \
	/bin/bash -c "./upload_locally.sh ${dist}"
