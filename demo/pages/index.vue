<template>
  <div class="container">
    <img src="../assets/img/logo_nav.png" id="logo">
    <p>视频分析平台</p>
    <div class="button-group">
      <Button type="primary" class="button" @click="toLive()">视频审查</Button>
      <br/><br/>
      <!--<Button type="success" class="button" @click="toClassify()">-->
        <!--视频分类-->
      <!--</Button>-->
      <br/><br/>
      <!--<Button class="button" @click="toData()">视频结构化</Button>-->
    </div>
  </div>
</template>

<script>

  export default {
    name: "index",
    async asyncData({app}) {
      await app.$axios.$post('/web/task/add')
    },
    methods: {
      toClassify() {
        this.$router.push('/classify');
      },
      toData() {
        this.$router.push('/data');
      },
      toLive() {
        this.$router.push('/live');
      }
    }

  }
</script>

<style>
  .container {
    display: block;
    position: absolute;
    top: 50%;
    left: 50%;
    width: 600px;
    height: 400px;
    margin-left: -300px;
    margin-top: -200px;
    text-align: center;
    box-shadow: 0 1px 4px 2px rgba(125, 121, 125, 0.4);
  }

  #logo {
    width: 100px;
    height: 80px;
    display: block;
    position: relative;
    left: 50%;
    top: 30px;
    margin-left: -50px;
  }

  .button-group {
    position: relative;
    top: 35%;
  }

  p {
    top: 15%;
    font-weight: bold;
    position: relative;
    font-size: 20px;
  }

  .button {
    width: 200px;
  }

</style>
