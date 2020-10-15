const router = require('express').Router();
const kafka = require('../../service/kafka_producer');
const db = require('../../service/mysql');
const protobuf = require('protobufjs');
const uuidv4 = require('uuid/v4');
const ffmpeg = require('fluent-ffmpeg');
const fs = require('fs');
const path = require('path');

router.get('/start', function (req, res) {
  kafka.produce('start', 'start', function (err, result) {
    if (result) {
      res.json({name: result})
    } else {
      res.json({name: 'error'})
    }
  });
});

router.get('/stop', function (req, res) {
  kafka.produce('stop', 'stop', function (err, result) {
    if (result) {
      res.json({name: result})
    } else {
      res.json({name: 'error'})
    }
  });
});

// router.get('/add/:index', function (req, res) {
//   const data = {
//     videoPath: '/novumind/share/video-sample/' + req.params.index + '.mp4',
//     modelIds: ['1']
//   };
//   const options = {
//     hostname: '192.168.1.100',
//     port: 20000,
//     path: '/create_task',
//     method: 'POST',
//     headers: {
//       'Content-Type': 'application/json',
//     }
//   };
//   const request = http.request(options, function (response) {
//     response.on('data', function (body) {
//       console.log('Body: ' + body);
//     });
//   });
//   request.on('error', function (e) {
//     console.error('problem with request: ' + e.message);
//   });
//   // write data to request body
//   request.write(JSON.stringify(data));
//   request.end();
//   res.sendStatus(200)
// });

function addTask(url, nameArr, callback) {
  const id = uuidv4().replace(/-/g, '');
  protobuf.load('proto/storage.proto', function (err, root) {
    if (err) {
      console.error('Load proto:', err);
      callback({error: 'read proto failed'});
      return;
    }
    const taskProto = root.lookupType("Task");
    ffmpeg(url).screenshots({
      timestamps: ['50%'],
      filename: '%f.png',
      folder: 'screenshots',
      size: '320x240'
    }).on('error', (err, stdout, stderr) => {
      console.error('Screenshot error:', err);
    }).on('end', function () {
      const file = fs.readFileSync('screenshots/' + path.basename(url) + '.png');
      let newTask = {};
      newTask.id = id;
      newTask.videoUrl = url;
      newTask.modelGroupNames = nameArr;
      newTask.endTime = 0;
      newTask.startTime = new Date().getTime()*1000000;
      newTask.statusCode = 0;
      newTask.snapshot = Buffer.from(file, 'binary');
      const message = taskProto.create(newTask);
      const buffer = taskProto.encode(message).finish();
      db.query('insert into task (id, info, status, start_time, end_time) values (?,?,?,?,?)', function (err, data) {
        if (err) {
          callback({error: err})
        }
      }, id, buffer, 0, newTask.startTime, 0)
    });
  });
}

function taskHandler(task, cb) {
  if (task === undefined || task === null) {
    cb({});
    return;
  }
  protobuf.load('proto/storage.proto', function (err, root) {
    if (err) {
      console.error('Load proto:', err);
      cb({});
      return;
    }
    const taskProto = root.lookupType("Task");
    const decodeMsg = taskProto.decode(task.info).toJSON();
    let newTask = {};
    newTask.id = task.id;
    newTask.video = decodeMsg.videoUrl;
    newTask.progress = 0;
    newTask.statusCode = task.status;
    newTask.snapshot = decodeMsg.snapshot;
    newTask.startTime = decodeMsg.startTime;
    newTask.endTime = decodeMsg.endTime;
    cb(newTask);
  });
}

function taskListHandler(arr, cb) {
  let list = [];
  if (arr === undefined || arr.length === 0) {
    cb(list);
    return;
  }
  protobuf.load('proto/storage.proto', function (err, root) {
    if (err) {
      console.error('Load proto:', err);
      cb(list);
      return;
    }
    const taskProto = root.lookupType("Task");
    for (a of arr) {
      let decodeMsg = taskProto.decode(a.info).toJSON();
      let newTask = {};
      newTask.id = a.id;
      newTask.videoUrl = decodeMsg.videoUrl;
      newTask.statusCode = a.status;
      newTask.startTime = decodeMsg.startTime;
      newTask.endTime = decodeMsg.endTime;
      newTask.snapshot = decodeMsg.snapshot;
      list.push(newTask);
    }
    cb(list);
  });
}

router.get('/info/:id', function (req, res) {
  const sql = "select * from task where id = ?";
  db.query(sql, function (err, data) {
    if (err) {
      console.error('Fetch task info error:', err);
      res.status(500).json({});
      return;
    }
    taskHandler(data[0], function (result) {
      res.json({task: result});
    })
  }, req.params.id);
});

router.post('/add',function(req,res){
  for (let i = 0; i < 100; i++) {
    addTask('/novumind/share/video-sample/' + i + ".mp4", ['violence'], function (err) {
      if (err) {
        res.status(500).json({tasks: []});
        return;
      }
    });
  }
  res.status(200).json({});
});

router.get('/queue', function (req, res) {
  let sql = "select * from task where status=0";
  db.query(sql, function (err, data) {
    if (err) {
      console.error("Fetch task list error:", err);
      res.status(500).json({tasks: []});
      return;
    }
    taskListHandler(data, function (result) {
      res.json({tasks: result});
    });
  });
});

router.get('/list/:from/:to', function (req, res) {
  let sql = "select * from task where status=2 order by end_time desc limit ?,?";
  if (!isNaN(parseInt(req.params.from)) && !isNaN(parseInt(req.params.to))) {
    db.query(sql, function (err, data) {
      if (err) {
        console.error("Fetch task list error:", err);
        res.status(500).json({tasks: []});
        return;
      }
      taskListHandler(data, function (result) {
        res.json({tasks: result});
      });
    }, parseInt(req.params.from), parseInt(req.params.to));
  }
});

router.get('/result/:id', function (req, res) {
  const sql = "select * from task_result where task_id = ?";
  db.query(sql, function (err, data) {
    if (err) {
      console.error("Fetch task result error:", err);
      res.status(500).json({details: [], labels: []});
      return;
    }
    protobuf.load('proto/storage.proto', function (err, root) {
      if (err) {
        console.error('Load proto error:', err);
        res.status(500).json({details: [], labels: []});
        return;
      }
      const modelOutput = root.lookupType("ModelOutput");
      let labels = [];
      let metadatas = [];
      let examples = [];
      for (let task of data) {
        const decodeMsg = modelOutput.decode(task.info).toJSON();
        const exampleInfo = decodeMsg.exampleInfo;
        const meta = decodeMsg.metadatas;
        if (!labels.includes(exampleInfo.tag)) {
          labels.push(exampleInfo.tag)
        }
        let info = {};
        info.frameNum = exampleInfo.frameNum;
        info.labels = exampleInfo.tag;
        info.image = exampleInfo.image;
        info.confidence = exampleInfo.confidence;
        examples.push(info);
        metadatas.push(meta);
      }
      // console.log('details', examples);
      // console.log('labels', labels);
      res.json({details: examples, labels: labels, metadatas: metadatas});
    });
  }, req.params.id);

});

// router.get('/result/:id', function (req, res) {
//   const sql = "select * from task_result where task_id = ?";
//   db.query(sql, function (err, data) {
//     if (err) {
//       console.error("Fetch task result error:", err);
//       res.status(500).json({details: [], labels: []});
//       return;
//     }
//     else {
//       ffmpeg(req.query.video).ffprobe(function (err, metadata) {
//         if (err) {
//           console.error('cannot find video: ', err);
//           res.status(500).json({details: [], labels: []});
//           return;
//         }
//         let fps = eval('(' + metadata.streams[0].avg_frame_rate + ')');
//         protobuf.load('proto/storage.proto', function (err, root) {
//           if (err) {
//             console.error('Load proto error:', err);
//             res.status(500).json({details: [], labels: []});
//           }
//           const modelOutput = root.lookupType("ModelOutput");
//           let labels = [];
//           let metadatas = [];
//           for (let task of data) {
//             let decodeMsg = modelOutput.decode(task.info).toJSON();
//             for (let item of decodeMsg.metadatas) {
//               if (item.details.length > 0) {
//                 let tempLabels = [];
//                 for (let d of item.details) {
//                   if (d.class === undefined || d.class.length < 1) {
//                     continue
//                   }
//                   for (let label of d.class) {
//                     tempLabels.push(label);
//                     if (!labels.includes(label)) {
//                       labels.push(label)
//                     }
//                   }
//                 }
//                 if (metadatas.length < 3) {
//                   let info = {};
//                   info.frameNum = item.frameNum;
//                   info.modelId = item.modelId;
//                   info.labels = tempLabels;
//                   metadatas.push(info);
//                 }
//               }
//             }
//           }
//           let timestamps = metadatas.map(a => parseInt(a.frameNum) / fps);
//           ffmpeg(req.query.video).screenshots({
//             timestamps: timestamps,
//             filename: req.params.id + '-%i.png',
//             folder: 'screenshots',
//             size: '320x240'
//           }).on('error', (err, stdout, stderr) => {
//             console.error('Screenshot error:', err);
//           }).on('end', function () {
//             metadatas.forEach((item, index) => {
//               const file = fs.readFileSync('screenshots/' + req.params.id + '-' + (index + 1) + '.png');
//               item.image = new Buffer(file, 'binary').toString('base64');
//             });
//             res.json({details: metadatas, labels: labels})
//           });
//         });
//       });
//     }
//   }, req.params.id);
// });

module.exports = router;
