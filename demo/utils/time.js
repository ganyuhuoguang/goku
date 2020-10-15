export function formatTime(time) {
  const dateDiff = new Date().getTime() - time;
  const dayDiff = Math.floor(dateDiff / (24 * 3600 * 1000));//计算出相差天数
  const leave1 = dateDiff % (24 * 3600 * 1000);    //计算天数后剩余的毫秒数
  const hours = Math.floor(leave1 / (3600 * 1000)) + dayDiff * 24;//计算出小时数
  //计算相差分钟数
  const leave2 = leave1 % (3600 * 1000);   //计算小时数后剩余的毫秒数
  const minutes = Math.floor(leave2 / (60 * 1000));//计算相差分钟数
  //计算相差秒数
  const leave3 = leave2 % (60 * 1000);     //计算分钟数后剩余的毫秒数
  const seconds = Math.round(leave3 / 1000);
  return wrap(hours) + ":" + wrap(minutes) + ":" + wrap(seconds);
}

function wrap(n) {
  return ((n < 10) ? '0' + n : n);
}


export function timeToString(time, format) {
  let date = new Date(time);
  return formatDate(date, format);
};

function formatDate(date, fmt) {
  if (/(y+)/.test(fmt)) {
    fmt = fmt.replace(RegExp.$1, (date.getFullYear() + '').substr(4 - RegExp.$1.length));
  }
  let o = {
    'M+': date.getMonth() + 1,
    'd+': date.getDate(),
    'h+': date.getHours(),
    'm+': date.getMinutes(),
    's+': date.getSeconds()
  };
  for (let k in o) {
    if (new RegExp(`(${k})`).test(fmt)) {
      let str = o[k] + '';
      fmt = fmt.replace(RegExp.$1, (RegExp.$1.length === 1) ? str : padLeftZero(str));
    }
  }
  return fmt;
}

function padLeftZero(str) {
  return ('00' + str).substr(str.length)
}
