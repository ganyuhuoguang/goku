<template>
  <div class="content">
    <Row>
      <Col :span="6">已处理视频：<span class="bold">{{video}}</span> 个</Col>
      <Col :span="4">禁播视频：<span class="bold">{{videoBad}}</span> 个</Col>
      <Col :span="8">实时处理速度：<span class="bold">{{fps}}</span> 帧每秒</Col>
      <Col :span="6">用时：<span class="bold">{{time}}</span></Col>
    </Row>
  </div>

</template>
<script>
  import {mapGetters} from 'vuex';

  export default {
    name: 'statics',
    computed: {
      ...mapGetters([
        'frames',
        'duration',
        'video',
        'time',
        'videoBad'
      ]),
      fps() {
        return (this.frames / (this.duration / 1000) * 100.0 / 100).toFixed(2);
      }
    },
  }
</script>
<style scoped>
  .content {
    text-align: center;
  }

  .bold {
    font-size: 26px;
    font-weight: bold;
  }
</style>
