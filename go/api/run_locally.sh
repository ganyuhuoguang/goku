#!/bin/bash

pwd="`pwd`"
repo="${pwd}/../../"

echo "Generate proto files ..."
cd ${repo}/proto && ./generate_go.sh || exit 1

echo "Building Api..."
cd ${pwd}/ && go build . || exit 1

echo "Runing Api..."
cd ${pwd}/ && ./api $@|| exit 1

