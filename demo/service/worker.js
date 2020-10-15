const grpc = require('grpc');
const PROTO_PATH = __dirname + '/../proto/worker.proto';
const cores = require('../config/inference.config');
const protoLoader = require('@grpc/proto-loader');
// Suggested options for similarity to existing grpc.load behavior
const packageDefinition = protoLoader.loadSync(
  PROTO_PATH,
  {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    arrays: true,
    objects: true,
    oneofs: true
  });
const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
// The protoDescriptor object has the full package hierarchy
const worker = protoDescriptor.novumind.goku.proto.worker;
let workerClientArr = [];
for (let addr of cores) {
  const wc = new worker.Worker(addr, grpc.credentials.createInsecure());
  workerClientArr.push(wc);
}

const video = ['/novumind/share/video-sample/53.mp4', '/novumind/share/video-sample/11.mp4', '/novumind/share/video-sample/34.mp4']


function runAnnotateVideo(workerClient, videoArr, callback) {
  let call = workerClient.annotateVideoSummary();
  call.on('data', function (response) {
    console.log('Response request_id: ' + response.request_id + " label: " + response.label +
      " confidence: " + response.confidence)
  });
  call.on('error', function (e) {
    // An error has occurred and the stream has been closed.
    console.log("error", e)
  });
  call.on('end', callback);
  for (let v of videoArr) {
    let annotateVideoRequest = {
      request_id: '56942e06d1a062c2e5c5a8484044ce3e',
      video_path: v
    };
    call.write(annotateVideoRequest)
  }
  // call.end();
}

var cluster = require('cluster');

if (cluster.isWorker) {
  console.log('Worker ' + process.pid + ' has started.');
  process.on('message', function (msg) {
    let wc = new worker.Worker(msg.add, grpc.credentials.createInsecure());
    let call = wc.annotateVideoSummary();
    call.on('data', function (response) {
      // Send message to master process.
      process.send({process: 'This is from worker ' + process.pid + '.', response: response})

    });
    call.on('error', function (e) {
      // An error has occurred and the stream has been closed.
      console.log("error", e)
    });
    call.on('end', function () {
      console.log('end')
    });
    for (let v of video) {
      let annotateVideoRequest = {
        request_id: '56942e06d1a062c2e5c5a8484044ce3e',
        video_path: v
      };
      call.write(annotateVideoRequest)
    }
    call.end();
  });

}

if (cluster.isMaster) {

  console.log('Master ' + process.pid + ' has started.');

  // Fork workers.
  for (var i = 0; i < 2; i++) {
    var child = cluster.fork();

    // Receive messages from this worker and handle them in the master process.
    child.on('message', function (msg) {
      console.log('Master ' + process.pid + ' received message from worker ' + this.pid + '.', msg);
    });

    // Send a message from the master process to the worker.
    child.send({add: cores[i]});
  }

  // Be notified when worker processes die.
  cluster.on('death', function (worker) {
    console.log('Worker ' + worker.pid + ' died.');
  });

}

