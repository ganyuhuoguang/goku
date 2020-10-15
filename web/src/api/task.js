import fetch from '@/utils/fetch'

export function listTasks(pagination, searchOption) {
  let requestData = {
    page: pagination,
    searchOption: searchOption
  };
  return fetch({
    url: '/api/list_classify_tasks',
    method: 'post',
    data: requestData
  });
}

export function fetchTask(id) {
  return fetch({
    url: '/api/get_task',
    method: 'post',
    data: {taskId: id}
  })
}

export function fetchClassifyTaskResult(id) {
  return fetch({
    url: '/api/get_classify_task_result',
    method: 'post',
    data: {taskId: id}
  })
}
