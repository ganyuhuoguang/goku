<template>
  <div class="layout">
    <Layout>
      <Header :style="{position: 'fixed', width: '100%'}">
        <Row>
          <Col :span="24">
            <statics></statics>
          </Col>
        </Row>
      </Header>
      <Content :style="{marginTop:'60px'}">
        <Row>
         <task-map :tasklist="tasks"></task-map>
        </Row>
      </Content>
    </Layout>
  </div>
</template>

<script>
  import Statics from '~/components/Statics.vue'
  import TaskMap from '~/components/TaskMap.vue'

  export default {
    name: "live",
    components: {
      Statics,
      TaskMap
    },
    async asyncData({app}) {
      const res = await app.$axios.$get('/web/task/queue');
      return {tasks: res.tasks}
    },
  }
</script>

<style scoped>
  .layout {
    /*border: 1px solid #d7dde4;*/
    background: #f5f7f9;
    position: relative;
    border-radius: 4px;
    overflow: hidden;
  }

  .ivu-layout-header {
    background-color: #1C1C1C;
    color: #fff;
    font-size: 16px;
    text-align: center;
    z-index: 99;
  }

  .list {
    border-right: 1px solid #e9eaec;
  }
</style>
