export function isvalidUsername(str) {
  const valid_map = ['admin', 'editor'];
  return valid_map.indexOf(str.trim()) >= 0;
}

export function validateURL(str) {
  const urlregex = /^(https?|ftp):\/\/([a-zA-Z0-9.-]+(:[a-zA-Z0-9.&%$-]+)*@)*((25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9][0-9]?)(\.(25[0-5]|2[0-4][0-9]|1[0-9]{2}|[1-9]?[0-9])){3}|([a-zA-Z0-9-]+\.)*[a-zA-Z0-9-]+\.(com|edu|gov|int|mil|net|org|biz|arpa|info|name|pro|aero|coop|museum|[a-zA-Z]{2}))(:[0-9]+)*(\/($|[a-zA-Z0-9.,?'\\+&%$#=~_-]+))*$/;
  return urlregex.test(str);
}

export function validateLowerCase(str) {
  const reg = /^[a-z]+$/;
  return reg.test(str);
}

export function validateUpperCase(str) {
  const reg = /^[A-Z]+$/;
  return reg.test(str);
}

export function validateAlphabets(str) {
  const reg = /^[A-Za-z]+$/;
  return reg.test(str);
}

export function validateFloat(val) {
  const reg = /^\d+(\.\d+)?$/;
  return reg.test(val);
}

export function validateInt(val){
  const reg = /^[0-9]+$/;
  return reg.test(val);
}

export function validateIP(str){
  const reg = /(?=(\b|\D))(((\d{1,2})|(1\d{1,2})|(2[0-4]\d)|(25[0-5]))\.){3}((\d{1,2})|(1\d{1,2})|(2[0-4]\d)|(25[0-5]))(?=(\b|\D))/;
  return reg.test(str)
}

export function validateEmail(str) {
  const reg = /^[A-Za-z\d]+([-_.][A-Za-z\d]+)*@([A-Za-z\d]+[-.])+[A-Za-z\d]{2,5}$/;
  return reg.test(str);
}

