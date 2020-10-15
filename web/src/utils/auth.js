import Cookies from 'js-cookie';

const TokenKey = 'SMOToken';

export function getToken() {
  return Cookies.get(TokenKey);
}

export function setToken(token) {
  return Cookies.set(TokenKey, token, { expires: 1/12, path: '' });
}

export function removeToken() {
  return Cookies.remove(TokenKey);
}

export function getName(){
  return Cookies.get('name');
}

export function setName(name){
  return Cookies.set('name', name, { expires: 1/12, path: '' });
}

export function removeName() {
  return Cookies.remove('name');
}
