const config = require('../config/kafka.config');
const kafka = require('kafka-node');
const client = new kafka.Client(config.host);
const producer = new kafka.Producer(client, {
  partitionerType: 0
});

class kafkaProducer {
  static produce(topic, message, cb) {
    producer.on('ready', function () {
      console.log('Kafka client is ready to produce...')
    });
    let producerPayloads = [{
        topic: topic,
        messages: message,
      }];
    producer.send(producerPayloads, function (err, result) {
      if (err) {
        console.log('error', err);
        cb(err, null)
      }
      // console.log('produce', result);
      cb(null, result)
    });
    producer.on('error', function (err) {
      console.log('error', err);
    });
  }
}

module.exports = kafkaProducer;


