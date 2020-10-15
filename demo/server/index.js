const express = require('express');
const {Nuxt, Builder} = require('nuxt');

const host = process.env.HOST || '0.0.0.0';
const port = process.env.PORT || 3000;
const api = require('./api');
const expressWs = require('express-ws');
const events = require('events');
const cluster = require('cluster');
let eventEmitter = new events.EventEmitter();
eventEmitter.setMaxListeners(1000);
const bodyParser = require('body-parser');


// let prgOffset = 0;
// let qOffset = 0;
// let consumerPayloads = [{topic: 'progress', offset: prgOffset}, {
//   topic: 'queue',
//   offset: qOffset
// }];
//
// const kafkaConfig = require('../config/kafka.config');
// const kafka = require('kafka-node');
//
// let client = new kafka.KafkaClient({kafkaHost: kafkaConfig.kafkaHost});
// let consumer = new kafka.Consumer(client, consumerPayloads, kafkaConfig.consumerOptions);
// const offset = new kafka.Offset(client);
//
// client.on('ready', () => {
//   console.log('Kafka client connected...');
//   client.loadMetadataForTopics(["start", "queue", "progress", "stop"], (err, resp) => {
//     console.log(JSON.stringify(resp))
//   });
// });
//
// offset.fetch([{topic: 'progress', partition: 0, time: -1}], function (err, data) {
//   prgOffset = data['progress']['0'][0];
//   console.log('progress:', prgOffset);
// });
//
// offset.fetch([{topic: 'queue', partition: 0, time: -1}], function (err, data) {
//   qOffset = data['queue']['0'][0];
//   console.log('queue:', qOffset);
// });
//
// client.on('error', err => {
//   console.log('Kafka client error:', err);
//   // client.close(); // Comment out for client on close
// });
//
// client.on('close', err => {
//   console.log('Kafka client closed...');
//   setTimeout(() => {
//     let c = new kafka.KafkaClient({kafkaHost: kafkaConfig.kafkaHost});
//     c.on('ready', function () {
//       c.loadMetadataForTopics(["queue", "progress", "start"], (err, resp) => {
//         if (err) {
//           // console.log(err);
//         } else {
//           console.log('reloading..');
//         }
//       });
//     });
//   }, 5000);
// });

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
const inference = protoDescriptor.novumind.goku.proto.worker;

if (cluster.isWorker) {
  console.log('Worker ' + process.pid + ' has started.');
  process.on('message', function (msg) {
    let wc = new inference.Worker(msg.address, grpc.credentials.createInsecure());
    let call = wc.annotateVideoSummary();
    call.on('data', function (response) {
      // Send message to master process.
      let res = {res: response, addr: msg.address};
      process.send(res);
      console.log("receive: " + msg.address, response)

    });
    call.on('error', function (e) {
      // An error has occurred and the stream has been closed.
      console.log("error", e)
    });
    call.on('end', function () {
      console.log('end')
    });
    for (let task of msg.queue) {
      let annotateVideoRequest = {
        request_id: task.id,
        video_path: task.videoUrl
      };
      call.write(annotateVideoRequest)
    }
    call.end()
  });
}
else {
  const app = express();
  app.use(bodyParser.json());
  app.use(bodyParser.urlencoded({limit: '100mb', extended: true}));
  expressWs(app);
  app.set('port', port);
  app.use('/web', api);

  console.log('Master ' + process.pid + ' has started.');

  // Be notified when worker processes die.
  cluster.on('exit', (worker, code, signal) => {
    console.log('worker %d died (%s). restarting...',
      worker.process.pid, signal || code);
    // cluster.fork();
  });

  //Listening Kafka
  app.ws('/ws', (ws, req) => {
    consumer.on('message', message => {
      console.log('Consume topic:', message.topic);
      console.log(message);
      try {
        let msg = JSON.parse(message.value);
        if (message.topic === "progress") {
          msg.topic = "progress";
          prgOffset = parseInt(message.offset) + 1;
        } else {
          msg.topic = "queue";
          qOffset = parseInt(message.offset) + 1;
        }
        ws.send(JSON.stringify(msg));
      } catch (e) {
        // console.error('Send error:', e);
        console.log('miss sending');
      }
    });
    consumer.on('error', err => {
      console.error('Kafka consumer error', err);
    });
  });


  //Listening inference core
  app.ws('/live', (ws, req) => {
    ws.on('message', function (msg) {
      let taskQueue = chunkArray(JSON.parse(msg), cores.length);
      cores.forEach((val,index)=>{
        let worker = cluster.fork();
        worker.send({address: cores[index], queue: taskQueue[index]});
        worker.on('message', msg => {
          console.log('send: ' + msg.addr, msg.res)
          ws.send(JSON.stringify(msg.res))
        });
      });
    });
  });

  // Import and Set Nuxt.js options
  let config = require('../nuxt.config.js');
  let server;
  config.dev = !(process.env.NODE_ENV === 'production');

  async function start() {
    // Init Nuxt.js
    const nuxt = new Nuxt(config);

    // Build only in dev mode
    if (config.dev) {
      const builder = new Builder(nuxt);
      await builder.build()
    }

    // Give nuxt middleware to express
    app.use(nuxt.render);

    // Listen the server
    server = app.listen(port, host);
    console.log('Server listening on http://' + host + ':' + port); // eslint-disable-line no-console
    process.on('uncaughtException', (err) => {
      console.log('Uncaught', err.message);
    });
  }

  start();
}


function chunkArray(arr, size) {
  let results = [];
  for (let i = 0; i < size; i++) {
    results.push(arr.filter((value, index) => {
      if (index % size === i)
        return value
    }))
  }
  return results;
}


