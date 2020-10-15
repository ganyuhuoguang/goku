#!/bin/bash

pwd="`pwd`"
repo="${pwd}/../../../../"

echo "Generate proto files ..."
cd ${repo}/proto && ./generate_go.sh || exit 1

echo "Building db..."
cd ${pwd}/ && go build . || exit 1

echo "Runing DB example..."
cd ${pwd}/ && ./example $@|| exit 1

