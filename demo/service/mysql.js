const mysql = require('mysql');
const config = require('../config/db.config');

const handleMysql = function () {
  let db = mysql.createConnection(config);
  connect();

  function connect() {
      db.connect(handleError);
      db.on('error', handleError);
  }

  function handleError(err) {
    if (err) {
      if (err.code === 'PROTOCOL_CONNECTION_LOST') {
        this.connect();
      } else {
        console.error(err.stack || err);
      }
    }
  }

  return {
    query: function (sql, callback) {
      let values = Array.prototype.slice.call(arguments, 2);
      db.query(sql, values, function (err, result, fields) {
        if (err) callback(err, null);
        else {
          callback(null, result);
        }
      });
    },
    close: function () {
      db.end(function (err) {
        if (err) {
          throw err;
        } else {
          console.info("db disconnected...");
        }
      })
    },
  };
}();
module.exports = handleMysql;

