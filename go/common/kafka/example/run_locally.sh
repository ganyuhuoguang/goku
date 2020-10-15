#!/bin/bash

pwd="`pwd`"
repo="${pwd}/../../../.."

echo "Generate proto files ..."
cd ${repo}/proto && ./generate_go.sh || exit 1

echo "Building kafka..."
cd ${pwd}/ && go build . || exit 1

echo "Runing kafka..."
cd ${pwd}/ && ./example $@|| exit 1

