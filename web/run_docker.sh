#!/bin/bash

image=registry.corp.novumind.com/dev/common/node
docker pull $image

docker run -it \
	-v `pwd`:/src \
        -w /src \
	--network=host \
	${image} \
	/bin/bash -c "npm install --registry http://cnpmreg.corp.novumind.com --cache-min 9999999 && \
	npm run dev"
