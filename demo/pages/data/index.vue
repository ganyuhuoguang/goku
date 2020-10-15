<template>
  <div>
      <task-table :taskList="tasks"></task-table>
  </div>
</template>
<script>
  import TaskTable from '../../components/TaskTable.vue'

  export default {
    name: 'taskResult',
    components: {
      TaskTable,
    },
    async asyncData({app}) {
      const res = await app.$axios.$get('/web/task/list/0/20');
      return {tasks: res.tasks}
    },

  }
</script>
