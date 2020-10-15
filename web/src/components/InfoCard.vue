<template>
  <Card :padding="0">
    <div class="infor-card-con">
      <Col class="infor-card-icon-con" :style="{backgroundColor: color, color: 'white'}" span="8">
      <Row class="height-100" type="flex" align="middle" justify="center">
        <Icon :type="iconType" :size="iconSize"></Icon>
      </Row>
      </Col>
      <Col span="16" class="height-100">
      <Row type="flex" align="middle" justify="center" class="height-100">
        <info-card-text
          :val="val"
          :color="color"
          :countSize="countSize"
          :countWeight="countWeight"
        >
          <p class="infor-intro-text" slot="intro">{{ text }}</p>
        </info-card-text>
      </Row>
      </Col>
    </div>
  </Card>
</template>

<script>
  import InfoCardText from '@/components/InfoCardText'
  export default {
    name: 'infoCard',
    components:{
      InfoCardText
    },
    props: {
      val: {
        type: Number,
        default: 0
      },
      color: String,
      iconType: String,
      countSize: {
        type: String,
        default: '30px'
      },
      countWeight: {
        type: Number,
        default: 700
      },
      iconSize: {
        type: Number,
        default: 40
      },
      text: String,
    }
  };
</script>
<style>
  .infor-card-icon-con {
    height: 100%;
  }

  .height-100 {
    height: 100%;
  }

  .infor-card-con {
    height: 100px;
  }

  .infor-intro-text {
    font-size: 12px;
    font-weight: 500;
    color: #C8C8C8;
  }
</style>
