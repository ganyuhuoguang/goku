<template>
  <div class="layout">
    <Layout>
      <Header>
         <nav-bar></nav-bar>
      </Header>
      <Content style="padding:0 50px;min-height: 80vh;">
        <!--<Card style="min-height: 78vh">-->
          <transition name="fade" mode="out-in">
            <router-view></router-view>
          </transition>
        <!--</Card>-->
      </Content>
      <!--<Footer class="layout-footer-center">2018 &copy; NovuMind</Footer>-->
    </Layout>
  </div>
</template>
<script>

  import NavBar from './NavBar';

  export default {
    name:'Main',
    components:{
      NavBar,
    },
    data() {
      return {};
    },
    computed: {},
    methods: {},
    watch: {},
    mounted() {
    },
    created() {
    }
  };
</script>
<style scoped>
  .layout {
    border: 1px solid #d7dde4;
    background: #f5f7f9;
    position: absolute;
    border-radius: 4px;
    height:100%;
    width:100%;
    overflow: auto;
  }
</style>
