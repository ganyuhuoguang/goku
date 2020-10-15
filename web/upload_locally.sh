#!/bin/bash
dist=$1
npm install --registry http://cnpmreg.corp.novumind.com --cache-min 9999999
npm run build
tar -czvf dist.tar.gz dist/
mkdir -p /tmp
cd /tmp
git clone https://readonly:novumind_readonly@gitlab.corp.novumind.com/novumind/web-dist.git
git config --global user.email "novuface@novumind.com"
git config --global user.name "novuface"
git config --global http.sslverify false
mkdir -p /tmp/web-dist/${dist}/
mv /src/dist.tar.gz /tmp/web-dist/${dist}/
cd /tmp/web-dist
git add . && git commit -m "upload new version"
git push

