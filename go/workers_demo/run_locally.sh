#!/bin/bash

pwd="`pwd`"
repo="${pwd}/../../"

echo "Generate proto files ..."
#cd ${repo}/proto && ./generate_go.sh || exit 1   for test

echo "Building worker..."
cd ${pwd}/../workers_demo/ && go build . || exit 1

echo "Runing worker..."
cd ${pwd}/../workers_demo/ && ./workers_demo $@|| exit 1
