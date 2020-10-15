#!/bin/bash

pwd="`pwd`"
repo="${pwd}/../../"

echo "Generate proto files ..."
cd ${repo}/proto && ./generate_go.sh || exit 1

echo "Building init ..."
cd ${pwd}/ && go build . || exit 1

echo "Runing init..."
cd ${pwd}/ && ./init_demo $@|| exit 1

