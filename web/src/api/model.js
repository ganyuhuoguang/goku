import fetch from '@/utils/fetch'

export function listModels() {
  return fetch({
    url: '/api/list_models',
    method: 'post',
  });
}

export function listModelTypes() {
  return fetch({
    url: '/api/list_model_types',
    method: 'post'
  })
}

export function updateModelType(modelId, modelTypes) {
  return fetch({
    url: '/api/update_model_types',
    method: 'post',
    data: {modelId: modelId, modelTypes: modelTypes}
  })
}

export function addModel(model, modelTypes) {
  return fetch({
    url: '/api/add_model',
    method: 'post',
    data: {model: model, modelTypes: modelTypes}
  })
}

export function getModel(id){
  return fetch({
    url: '/api/get_model',
    method: 'post',
    data: {modelId: id}
  })
}
