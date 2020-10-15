import Vue from 'vue';
import Router from 'vue-router';
import store from '@/store';
import iView from 'iview';
import {asyncRouterMap} from './router';

Vue.use(Router);

export const router = new Router({
  // mode: 'history',
  routes: asyncRouterMap
});

router.beforeEach((to, from, next) => {
  iView.LoadingBar.start();
  next();
});

router.afterEach(() => {
  iView.LoadingBar.finish();
  window.scrollTo(0, 0);
});
