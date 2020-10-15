<template>
  <div>
    <Menu mode="horizontal" theme="dark" :active-name="defaultActive">
      <div class="layout-logo">视频分析平台
      </div>
      <div class="layout-nav-main">
        <nav-bar-item :routes="permission_routers"></nav-bar-item>
      </div>
    </Menu>
  </div>
</template>
<script>
  import {mapGetters} from 'vuex';
  import NavBarItem from './NavBarItem';

  export default {
    name: 'NavBar',
    components: {
      NavBarItem
    },
    data() {
      return {
      }
    },
    computed: {
      ...mapGetters([
        'permission_routers'
      ]),
      defaultActive() {
        return this.$route.matched[0].path + '/index'
      }
    },
    methods: {
    }
  }
</script>
<style>
  .layout-nav-main {

  }

  img {
    display: inline-block;
    width: 40px;
    height: 45px;
    max-height: 60px;
  }

  .layout-logo {
    width: 150px;
    height: 30px;
    float: left;
    position: relative;
    left: 20px;
    font-size:20px;
    color: #ffffff;
    font-weight: bold;
  }


  a {
    color: #f5f7f9;
  }

</style>
