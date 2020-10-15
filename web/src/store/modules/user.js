import {login, logout, getUser} from '@/api/user';
import {getToken, setToken, removeToken, getName, setName, removeName} from '@/utils/auth';

const user = {
  state: {
    token: getToken(),
    name: '',
    roles: []
  },

  mutations: {
    SET_TOKEN: (state, token) => {
      state.token = token;
    },
    SET_NAME: (state, name) => {
      state.name = name;
    },
    SET_ROLES: (state, roles) => {
      state.roles = roles;
    }
  },

  actions: {
    Login({commit}, userInfo) {
      const username = userInfo.username.trim();
      const pass = userInfo.password;
      return new Promise((resolve, reject) => {
        login(username, pass).then(response => {
          setToken(response.token);
          commit('SET_TOKEN', response.token);
          commit('SET_NAME', username);
          setName(username);
          resolve()
        }).catch(error => {
          reject(error)
        })
      })
    },
    GetInfo({commit, state}) {
      return new Promise((resolve, reject) => {
        getUser(getName()).then(res => {
          const user = res.user;
          setName(user.username);
          commit('SET_NAME', user.username);
          let roles = ['admin'];
          if (!user.isAdmin) {
            roles = ['user'];
          }
          commit('SET_ROLES', roles);
          resolve(roles);
        }).catch(err => {
          reject(err);
        });
      })
    },
    LogOut({commit, state}) {
      return new Promise((resolve, reject) => {
        logout(state.name).then(() => {
          commit('SET_TOKEN', '');
          commit('SET_ROLES', []);
          removeToken();
          removeName();
          resolve();
        }).catch(error => {
          reject(error);
        })
      })
    },
    FedLogOut({commit}) {
      return new Promise(resolve => {
        commit('SET_TOKEN', '');
        commit('SET_ROLES', []);
        removeToken();
        removeName();
        resolve();
      })
    }

  }
};

export default user;
