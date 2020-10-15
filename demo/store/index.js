import Vuex from 'vuex';

const store = () => new Vuex.Store({
  state: {
    frames: 0,
    duration: 0.1,
    task: {
      labels: [],
      images: []
    },
    video: 0,
    time:"00:00:00",
    videoBad:0
  },
  getters: {
    frames: state => state.frames,
    task: state => state.task,
    video: state => state.video,
    duration: state => state.duration,
    time: state => state.time,
    videoBad: state =>state.videoBad
  },
  mutations: {
    SET_FRAMES: (state, frames) => {
      state.frames = frames;
    },
    SET_TASK: (state, task) => {
      state.task = task;
    },
    SET_VIDEO: (state, video) => {
      state.video = video;
    },
    SET_DURATION: (state, duration) => {
      state.duration = duration;
    },
    SET_TIME: (state, time) =>{
      state.time = time;
    },
    SET_VIDEO_BAD:(state, bad) =>{
      state.videoBad = bad;
    }
  },
});

export default store;
