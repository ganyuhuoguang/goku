<template>
  <div>
    <div class="nav-title">{{$route.name}}</div>
    <Row :gutter="10" class="search-bar">
      <Col :lg="4" :sm="10" :md="6" :xs="10">
        <Select v-model="searchOption.statusCode"
                class="input-border" style="width:80%" placeholder="请选择状态">
          <Option :value="-1">全部</Option>
          <Option v-for="(val,key) in statuslist" :value="key" :key="key">
            {{val.title}}
          </Option>
        </Select>
      </Col>
      <Col :lg="4" :sm="4" :md="4" :xs="4">
        <Button icon="search" @click="search()"
                type="primary">搜索
        </Button>
      </Col>
    </Row>
    <Row class="line-break">
      <Table :data="taskList" :columns="taskColumns" stripe :loading="loading"></Table>
      <Pagination :total="pagination.total" :pageIndex="pagination.pageIndex"
                  @page-change="changePage" :fetchData="'task'"
                  :searchOption="searchOption"></Pagination>
    </Row>
    <Modal v-model="taskResultModal">
      <p slot="header">
        <span>分析结果</span>
      </p>
      <div style="max-height: 500px;overflow: auto">
        <pre>{{JSON.stringify(taskResults,undefined,2)}}</pre>
      </div>
    </Modal>
  </div>
</template>
<script>
  import Pagination from '@/components/Pagination'
  import {pagination, statusMap, taskStatusToShown, taskStatus, timeToString} from
      '@/utils/filter'
  import {listTasks, fetchClassifyTaskResult} from '@/api/task'

  export default {
    name: 'task',
    components: {
      Pagination
    },
    data() {
      return {
        loading: false,
        taskList: [],
        taskColumns: [
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
            title: '分类结果',
            key: 'labels',
            align: 'center',
            render: (h, params) => {
              let labelArr = [];
              params.row.labels.forEach((val, idx) => {
                labelArr.push(h('Tag', {
                  props: {
                    type: 'border',
                    color: 'blue'
                  }
                }, val))
              });
              return h('div', labelArr);
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
              if (parseInt(row.endTime) === 0) {
                return h('span', '');
              }
              return h('span', timeToString(parseInt(row.endTime) / 1000000, 'yyyy-MM-dd hh:mm:ss'));
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
                      this.getTaskResult(row.id);
                    }
                  }
                }, '查看')
              ]);
            }

          }
        ],
        statuslist: statusMap,
        searchOption: {
          statusCode: -1
        },
        pagination: pagination,
        taskResults: [],
        taskResultModal: false
      }
    },
    methods: {
      search() {
        this.pagination.pageIndex = 1;
        this.list();
      },
      changePage(obj) {
        this.pagination = obj.pagination;
        this.taskList = obj.data;
      },
      list() {
        listTasks(this.pagination, this.searchOption).then(resp => {
          this.taskList = resp.data.tasks;
          this.pagination = resp.data.page;
          this.loading = false;
        }).catch(() => {
          this.taskList = [];
          this.loading = false;
        })
      },
      getTaskResult(id) {
        this.taskResultModal = true;
        fetchClassifyTaskResult(id).then(resp => {
          this.taskResults = resp.data.taskResults;
        }).catch(err => {
          this.taskResults = [];
        })
      }
    },
    created() {
      this.list();
    }


  }
</script>
