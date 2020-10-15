import {formatDate} from '@/utils/index'

const timeToString = (time, format) => {
  let date = new Date(time);
  return formatDate(date, format);
};

const statusMap = {
  RUNNING: {color:'blue',title:'进行中'},
  SUCCESS: {color:'green',title:'完成'},
  PENDING: {color:'yellow',title:'准备中'},
  FAILED: {color:'red',title:'失败'},
};

const taskStatusToShown = status => {
  return statusMap[status].color;
};

const taskStatus = status => {
  return statusMap[status].title;
};

const pagination = {
  total: 0,
  pageSize: 15,
  pageIndex: 1
};

function tagColor(i) {
  const colors = ['blue', 'red', 'yellow', '#66327C'];
  return colors[i % colors.length];
};

const HEARTBEAT = 3000;

export {
  timeToString,
  taskStatusToShown,
  taskStatus,
  statusMap,
  tagColor,
  pagination,
  HEARTBEAT
};
