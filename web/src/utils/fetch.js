import axios from 'axios'
import {Modal} from 'iview'
import store from '../store'
import {getToken} from '@/utils/auth'

const service = axios.create({
  // timeout: 5000,
});

service.interceptors.request.use(config => {
  if (store.getters.token) {
    config.headers['Grpc-Metadata-X-Authorization'] = getToken();
  }
  return config;
}, error => {
  // Do something with request error
  console.log(error); // for debug
  Promise.reject(error);
});

service.interceptors.response.use(
  response => {
    return response;
  },
  error => {
    console.dir(error);
    if (error.response.status === 401 && (error.config.url.indexOf('/api/user/login') === -1)) {
      Modal.warning({
        title:'请求超时',
        content:'请求已超时，请重新登录',
        okText:'重新登录',
        onOk:()=>{
          store.dispatch('FedLogOut').then(() => {
            window.location.reload()
          })
        }
      })
    }
    return Promise.reject(error.response);
  }
);

export default service
