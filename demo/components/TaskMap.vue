<template>
  <div>
    <Row :gutter="1">
      <Col :span="2" v-for="(task, index) in tasklist" :key="task.id">
        <div class="snapshot">
          <div class="error" v-show="task.statusCode===2">
            <p style="color:#EB7A77" v-for="tl in task.results"
               v-if="tl.confidence<1">{{tl.label}}
            </p>
            <p v-if="task.results.length===0" style="color:#91AD70">Normal</p></div>
          <img :src="'data:image/png;base64,'+task.snapshot" :id="task.id">
        </div>
      </Col>
    </Row>
  </div>
</template>

<script>
  import {w3cwebsocket} from 'websocket'
  import {formatTime} from '../utils/time'

  const threshold = 0.85;

  export default {
    name: "TaskMap",
    props: {
      tasklist: {
        type: Array
      }
    },
    data() {
      return {
        socket: new w3cwebsocket('ws://localhost:3000/live', 'echo-protocol'),
        start: new Date().getTime(),
        begin: new Date().getTime(),
        videoBad: 0,
        video: 0,
        now: 0,
        consumed: 0,
      }
    },
    methods: {
      setFrame(frame) {
        this.$store.commit('SET_FRAMES', frame);
      },
      setDuration(time) {
        this.$store.commit('SET_DURATION', time);
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
    },
    mounted() {
      this.socket.onopen = () => {
        console.log('WebSocket Client Connected...');
        if (this.socket.readyState === this.socket.OPEN) {
          this.socket.send(JSON.stringify(this.tasklist));
          this.start = new Date().getTime();
        }
      };

      this.socket.onmessage = e => {
        let data = JSON.parse(e.data);
        console.log(data);
        this.now = new Date().getTime();
        this.consumed += parseInt(data.total_frame_num);
        this.setDuration(this.now - this.start);
        this.setFrame(this.consumed);
        let task = this.tasklist.find(x => {
          return x.id === data.request_id;
        });
        task.statusCode = 2;
        let results = [];
        let isBad = false;
        for (let lc of data.label_confs) {
          if (lc.confidence > threshold && lc.label.toLowerCase()!=='normal') {
            let r = {};
            r.label = lc.label;
            r.confidence = parseFloat(lc.confidence);
            results.push(r);
            isBad = true;
          }
        }
        task.results = results;
        if (isBad) {
          this.videoBad += 1;
          this.setVideoBad(this.videoBad);
        }
        this.setTime();
        this.video += 1;
        this.setVideo(this.video);
        if (this.video % this.tasklist.length === 0) {
          setTimeout(() => {
            for (let t of this.tasklist) {
              t.statusCode = 0;
            }
            this.socket.send(JSON.stringify(this.tasklist));
          }, 2500)

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

<style scoped>
  img {
    width: 100%;
    display: block;
    height: 120px;
    opacity: 0.9;
  }

  .snapshot {
    margin-top: 1px;
    height: 120px;
    position: relative;
  }

  .error {
    height: 100%;
    color: white;
    background-color: #1C1C1C;
    opacity: 0.8;
    z-index: 200;
    position: absolute;
    width: 100%;
  }

  .error p {
    text-align: center;
    line-height: 100px;
    font-weight: bold;
    font-size: 20px;
  }


</style>
