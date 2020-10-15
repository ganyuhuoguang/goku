<template>
  <div>
    <Row>
      <Col :span="14">
      <Table :data="tasks" :columns="taskColumns" stripe></Table>
      </Col>
      <Col :span="10">
      <div v-if="task!==null" class="result">
        <pre>{{JSON.stringify(task,null,2)}}</pre>
      </div>
      </Col>
    </Row>
  </div>
</template>
<script>
  import {timeToString} from '../utils/time'

  const statusMap = {
    RUNNING: {color: 'blue', title: '进行中'},
    SUCCESS: {color: 'green', title: '完成'},
    PENDING: {color: 'yellow', title: '准备中'},
    FAILED: {color: 'red', title: '失败'},
  };

  const taskStatusToShown = status => {
    return statusMap[status].color;
  };

  const taskStatus = status => {
    return statusMap[status].title;
  };
  export default {
    name: 'taskTable',
    props: {
      taskList: {
        type: Array
      }
    },
    data() {
      return {
        tasks:[],
        taskColumns: [
          {
            title:'ID',
            key:'id'
          },
          {
            title: '视频源',
            key: 'videoUrl',
            align: 'center',
          }, {
            title: '状态',
            key: 'statusCode',
            width: 100,
            align: 'center',
            render: (h, params) => {
              const status = params.row.statusCode;
              return h('Tag', {
                props: {
                  type: 'border',
                  color: taskStatusToShown(status)
                }
              }, taskStatus(status))
            }
          }, {
            title: '开始时间',
            key: 'startTime',
            align: 'center',
            render: (h, params) => {
              const row = params.row;
              return h('span', timeToString(parseInt(row.startTime) / 1000000,
                'yyyy-MM-dd hh:mm:ss'));
            }
          },
          {
            title: '结束时间',
            key: 'endTime',
            align: 'center',
            render: (h, params) => {
              const row = params.row;
              return h('span', timeToString(parseInt(row.endTime) / 1000000,
                'yyyy-MM-dd hh:mm:ss'));
            }
          }, {
            title: '操作',
            key: 'action',
            width: 150,
            align: 'center',
            render: (h, params) => {
              const row = params.row;
              let isDisabled = true;
              if (row.statusCode === 'SUCCESS') {
                isDisabled = false;
              }
              return h('div', [
                h('Button', {
                  props: {
                    type: 'info',
                    size: 'small',
                    disabled: isDisabled
                  },
                  on: {
                    click: () => {
                      this.info(row.id);
                    }
                  }
                }, '查看结果')
              ]);
            }

          }
        ],
        task: null
      }
    },
    methods: {
      async info(id) {
        const res = await this.$axios.$get('/web/task/result/' + id);
        this.task = res.metadatas;
      },
    },
    mounted(){
      this.tasks = this.taskList;
    }
  }
</script>
<style scoped>
  .result{
    max-height:1000px;
    overflow: auto;
  }
</style>
