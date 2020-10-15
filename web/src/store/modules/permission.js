import {asyncRouterMap, constantRouterMap} from '@/router/router';

function hasPermission(roles, route) {
  if (route.meta && route.meta.role) {
    return roles.some(role => route.meta.role.indexOf(role) >= 0);
  } else {
    return true;
  }
}

function filterAsyncRouter(asyncRouterMap, roles) {
  let accessedRouters = asyncRouterMap.filter(route => {
    if (hasPermission(roles, route)) {
      if (route.children && route.children.length) {
        route.children = filterAsyncRouter(route.children, roles);
      }
      return true;
    }
    return false;
  });
  return accessedRouters;
}

const permission = {
  state: {
    routers: asyncRouterMap,
    addRouters: []
  },
  mutations: {
    SET_ROUTERS: (state, routers) => {
      state.addRouters = routers;
      state.routers = constantRouterMap.concat(routers);
    }
  },
  actions: {
    GenerateRoutes({commit}) {
      return new Promise(resolve => {
        commit('SET_ROUTERS', asyncRouterMap);
        resolve();
      })
    }
  }
};

export default permission;
