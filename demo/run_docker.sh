#!/bin/bash

image=registry.corp.novumind.com/dev/common/node
docker pull $image
params=$@

docker run -it \
	-v `pwd`:/app \
	-v `pwd`/../proto/:/app/proto \
	-v /novumind/share/video-sample:/novumind/share/video-sample \
  -w /app \
  --name goku-demo \
	--network=host \
	${image} \
	/bin/bash -c "npm install --registry http://cnpmreg.corp.novumind.com --cache-min 9999999 && \
	npm run dev -- ${params}"
