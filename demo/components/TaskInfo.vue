<template>
  <div>
    <div style="text-align: center;font-size:26px;font-weight:bold">处理结果</div>
    <div class="task-title" v-if="task.id">Task: {{taskIndex(task.video)}}</div>
    <div class="tag">
      <div v-if="task.details && task.details.length>0">
        <div v-for="(detail, index) in task.details" class="task-info">
          <div v-if="detail.labels && detail.labels.length>0" class="task-detail">
            <Tag type="border" color="blue">不宜播放
            </Tag>
            <div style="color:#80848f">置信度: {{detail.confidence.toFixed(2)}}</div>
            <img :src="'data:image/png;base64,'+detail.image" style="width: 70%">
          </div>
          <!--<div v-if="detail.labels===undefined">-->
            <!--<Tag type="border" color="green">正常</Tag>-->
          <!--</div>-->
        </div>
      </div>
      <div v-if="task.details && task.details.length===0">
        <Tag type="border" color="green">正常</Tag>
      </div>
    </div>
  </div>
</template>
<script>
  import {mapGetters} from 'vuex';

  export default {
    name: 'TaskInfo',
    computed: {
      ...mapGetters([
        'task',
      ])
    },
    methods: {
      taskIndex(str) {
        return str.split('/')[4];
      }
    }
  }
</script>
<style scoped>
  .task-title {
    padding: 20px;
    font-size: 20px;
    font-weight: bold;
  }

  .task-info {
    padding-left: 10px;
  }

  .tag{
    margin-left:20px;
  }
  .task-detail{
    margin-top:10px;
  }
</style>
