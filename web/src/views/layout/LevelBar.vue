<template>
  <Breadcrumb :style="{margin: '20px 0'}">
    <BreadcrumbItem v-for="(item,index) in levelList" :key="item.path"
                    :to="link(item,index)">
      {{item.name}}
    </BreadcrumbItem>
  </Breadcrumb>
</template>
<script>
  export default {
    name: 'LevelBar',
    data() {
      return {
        levelList: null
      }
    },
    watch: {
      $route() {
        this.getBreadcrumb();
      }
    },
    methods: {
      getBreadcrumb() {
        let matched = this.$route.matched.filter(item => item.name);
        const first = matched[0];
        if (first && (first.name !== '首页' || first.path !== '/home')) {
          matched = [{name: '首页', path: '/home'}].concat(matched);
        }
        this.levelList = matched;
      },
      link(item, index) {
        if
        ((item.redirect === undefined || index === this.levelList.length - 1 ||
            item.parent===undefined) &&
          item.path !== '/home') {
          return "";
        } else {
          return item.path || item.redirect;
        }
      }
    },
    created() {
      this.getBreadcrumb();
    }
  }
</script>
