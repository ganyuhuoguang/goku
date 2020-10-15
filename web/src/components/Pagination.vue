<template>
  <div style="margin: 20px 0 10px 0;overflow: hidden">
    <Page :total="total" :page-size="pageSize" :current="pageIndex" show-total show-elevator
          @on-change="handleChange" style="height: 40px;">
    </Page>
  </div>
</template>
<script>
  import {listTasks} from '@/api/task'

  export default {
    name: 'Pagination',
    props: {
      total: {
        type: Number,
        default: 0
      },
      pageSize: {
        type: Number,
        default: 15
      },
      pageIndex: {
        type: Number,
        default: 1
      },
      fetchData: {
        type: String
      },
      searchOption: {
        type: Object
      }
    },
    methods: {
      handleChange(val) {
        let pagination = {
          total: this.total,
          pageSize: this.pageSize,
          pageIndex: val
        };
        if (this.pageIndex !== val) {
          if (this.fetchData === 'task') {
            listTasks(pagination, this.searchOption).then(resp => {
              this.$emit('page-change', {data: resp.data.tasks, pagination: resp.data.page})
            });
          }
        }
      }
    }
  }
</script>
<style>
  .ivu-page {
    float: right;
  }
</style>
