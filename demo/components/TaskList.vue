<template>
  <div style="margin:0 5%">
    <Row :gutter="10">
      <Col :span="2">
        <Button @click="run()" type="primary" v-show="!isStart">开始</Button>
      </Col>
      <Col :span="2">
        <Button @click="stop()" type="error" v-show="!isStop && isStart">停止</Button>
        <Button @click="resume()" type="warning" v-show="isStop">恢复</Button>
      </Col>
      <!--<Button @click="produce()" type="success">Produce</Button>-->
      <!--<Button @click="create()" type="info">Create</Button>-->
    </Row>
    <Row :gutter="30">
      <transition-group name="list" tag="div" class="center">
        <Col :span="4" v-for="(task, index) in tasklist" :key="task.id">
          <Card :padding="0" class="card">
            <img :src="'data:image/png;base64,'+task.snapshot" :id="task.id">
            <div class="center">
              <Progress :percent="task.progress" :stroke-width="5"></Progress>
            </div>
            <div>{{taskIndex(task.video)}}</div>
            <div>{{task.id.substring(0, 6)}}</div>
            <!--<div class="tag">-->
            <!--<div v-if="task.progress===100 && task.labels!==undefined && task.labels.length>0">-->
            <!--<div class="center">-->
            <!--<Tag type="border" :color="tagColor(index)" v-for="(label, index) in task.labels"-->
            <!--:key="index">{{-->
            <!--label}}-->
            <!--</Tag>-->
            <!--</div>-->
            <!--</div>-->
            <!--<div v-else-if="task.progress===100 && task.labels!==undefined &&-->
            <!--task.labels.length===0"-->
            <!--class="center">-->
            <!--<Tag type="border" color="green">正常</Tag>-->
            <!--</div>-->
            <!--<div v-else class="center">-->
            <!--<Tag type="border">处理中</Tag>-->
            <!--</div>-->
            <!--</div>-->
          </Card>
        </Col>
      </transition-group>
    </Row>
  </div>
</template>
<script>
  import {w3cwebsocket} from 'websocket'
  import {formatTime} from '../utils/time'
  const threshold = 0.75;

  export default {
    data() {
      return {
        consumed: 0,
        socket: new w3cwebsocket('ws://localhost:3000/ws', 'echo-protocol'),
        start: new Date().getTime(),
        begin: new Date().getTime(),
        now: 0,
        tasklist: [],
        prgMap: new Map(),
        video: 0,
        isStop: false,
        isStart: false,
        videoBad: 0
      }
    },
    methods: {
      tagColor(i) {
        const colors = ['blue', 'red', 'yellow', '#66327C'];
        return colors[i % colors.length];
      },
      remove(idx) {
        this.tasklist.splice(idx, 1);
      },
      async run() {
        this.start = new Date().getTime();
        this.begin = new Date().getTime();
        this.isStart = true;
        await this.$axios.$get('/web/task/start');
      },
      resultInfo(task) {
        this.$axios.$get('/web/task/result/' + task.id).then(res => {
          console.log('details', res.details)
          task.labels = res.labels;
          if (res.details.length > 0 && res.details[0].confidence > threshold) {
            task.details = res.details;
            this.videoBad += 1;
            this.setVideoBad(this.videoBad);
          }
          else {
            task.details = []
          }
          this.setTask(task);
        });
      },
      async stop() {
        await this.$axios.$get('/web/task/stop');
        this.isStop = true;
      },
      async resume() {
        await this.$axios.$get('/web/task/start');
        this.isStop = false;
      },
      setFrame(frame) {
        this.$store.commit('SET_FRAMES', frame);
      },
      setDuration(time) {
        this.$store.commit('SET_DURATION', time);
      },
      setTask(task) {
        this.$store.commit('SET_TASK', task);
      },
      setVideo(video) {
        this.$store.commit('SET_VIDEO', video);
      },
      setVideoBad(bad) {
        this.$store.commit('SET_VIDEO_BAD', bad);
      },
      setTime() {
        let time = formatTime(this.begin);
        this.$store.commit('SET_TIME', time);
      },
      taskIndex(str) {
        return str.split('/')[4];
      },
      taskFetch(id) {
        this.$axios.$get('/web/task/info/' + id).then(data => {
          if (data.task) {
            const i = this.tasklist.findIndex(x => x !== undefined && x.id === data.task.id);
            if (i === -1 && !this.prgMap.has(id)) {
              this.tasklist.push(data.task);
            }
          }
        });
      }
    },
    mounted() {
//      this.tasklist = [...this.tasks];
//      this.listTask(0, 24);
      if (this.tasklist.length === 0) {
        this.begin = new Date().getTime();
        this.start = new Date().getTime();
      }
      this.socket.onopen = () => {
        console.log('WebSocket Client Connected...');
      };
      this.socket.onmessage = e => {
        let data = JSON.parse(e.data);
        console.log(data, new Date().getTime());
        if (data.topic === "queue") {
          this.taskFetch(data.task_id);
        } else {
          this.isStart = true;
          console.log('processing:' + data.task_id, this.prgMap.get(data.task_id));
          if
          (!this.prgMap.has(data.task_id) || (this.prgMap.has(data.task_id) &&
            this.prgMap.get(data.task_id)) <
            parseInt(data.processed_num)) {
            this.consumed += parseInt(data.processed_once_num);
            this.now = new Date().getTime();
            this.setDuration(this.now - this.start);
            this.setFrame(this.consumed);
            let i = this.tasklist.findIndex(x => x !== undefined && x.id === data.task_id);
            this.prgMap.set(data.task_id, parseInt(data.processed_num));
            if (i >= 0 && this.tasklist[i] !== undefined) {
              this.setTime();
              let progress = parseInt(data.processed_num) / parseInt(data.total_num) *
                100.0;
              this.tasklist[i].progress = Number(progress.toFixed(2));
              if (parseInt(data.processed_num) === parseInt(data.total_num)) {
                this.video += 1;
                this.setVideo(this.video);
                this.resultInfo(Object.assign({}, this.tasklist[i]));
                console.log('removing:', this.tasklist[i].video);
                this.remove(i);
              }
            } else {
              console.log('not in tasklist', data.task_id);
            }
          }
        }
      };

      this.socket.onerror = () => {
        console.log('Websocket error');
        this.socket.onclose = () => {
          console.log('WebSocket Client Closing...')
        }
      };

      this.socket.onclose = () => {
        console.log('WebSocket Client Closing...')
      };
    }
  }
</script>
<style>
  img {
    width: 100%;
    display: block;
    height: 160px;
    opacity: 0.9;
  }

  .card {
    margin-top: 12%;
    display: inline-block;
  }

  .center {
    text-align: center;
  }

  .list-enter-active, .list-leave-active {
    transition: all 0.3s;
  }

  .list-enter, .list-leave-to
    /* .list-leave-active for below version 2.1.8 */
  {
    opacity: 0;
    transform: translateY(50px);
  }

</style>
