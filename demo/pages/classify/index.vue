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
      <Content :style="{marginTop:'80px'}">
        <Row>
          <Col :span="18" class="list">
          <task-list></task-list>
          </Col>
          <Col :span="6">
          <task-info></task-info>
          </Col>
        </Row>
      </Content>
    </Layout>
  </div>
</template>

<script>
  import Statics from '~/components/Statics.vue'
  import TaskList from '../../components/TaskList.vue'
  import TaskInfo from '../../components/TaskInfo.vue'

  export default {
    name: "classify",
    components: {
      Statics,
      TaskList,
      TaskInfo
    }
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
    background-color: #283443;
    color: #fff;
    font-size: 16px;
    text-align: center;
    z-index: 99;
  }

  .list {
    border-right: 1px solid #e9eaec;
  }
</style>
