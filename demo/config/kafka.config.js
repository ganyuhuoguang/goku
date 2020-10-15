const options = {
  host: '192.168.3.12:2181',
  kafkaHost: '192.168.3.12:9092',
  consumerOptions: {
    autoCommit: true,
    autoCommitIntervalMs: 1000,
    sessionTimeout: 30000,
    requestTimeout: 40000,
    fetchMaxWaitMs: 1000,
    fetchMaxBytes: 1024 * 1024,
    connectRetryOptions: {
      retries: 50,
      factor: 3,
      minTimeout: 8000,
      maxTimeout: 12000,
      randomize: true
    },
    // If set to 'buffer', values will be returned as raw buffer objects.
    encoding: 'utf8',
    keyEncoding: 'utf8',
    fromOffset: true
  }
};

module.exports = options;
