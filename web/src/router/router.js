import Main from '@/views/layout/Main';

export const constantRouterMap = [
  {
    path: '/404',
    name: '404',
    hidden: true,
    meta: {
      title: '404页面不存在'
    },
    component: () => import('@/views/error-page/404.vue')
  }
];

export const asyncRouterMap = [
  {
    path: '/',
    name: '',
    hidden: true,
    redirect: '/task'
  },
  {
    path: '/task',
    icon: 'filing',
    name: '任务管理',
    component: Main,
    redirect: '/task/index',
    dropdown: false,
    children: [
      {
        path: 'index',
        name: '任务列表',
        component: () => import('@/views/task/index.vue')
      }
    ]
  },
  {
    path: '/model',
    icon: 'network',
    name: '模型管理',
    component: Main,
    redirect: '/model/index',
    dropdown: false,
    children: [
      {
        path: 'index',
        name: '模型列表',
        component: () => import('@/views/model/index.vue')
      }
    ]
  },
  {path: '*', redirect: '/404', hidden: true}
];
