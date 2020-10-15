<template>
  <div>
    <p
      :style="{textAlign: 'center', color: color, fontSize: countSize, fontWeight: countWeight}">
      <span v-cloak>{{ endVal }}{{ unit }}</span></p>
    <slot name="intro"></slot>
  </div>
</template>
<script>
  export default {
    name:'InfoCardText',
    props: {
      val: {
        type: Number,
        default: 0
      },
      color: String,
      countSize: {
        type: String,
        default: '30px'
      },
      countWeight: {
        type: Number,
        default: 700
      }
    },
    data() {
      return {
        unit: '',
        endVal:''
      };
    },
    methods:{
      transformValue (val) {
        let endVal = 0;
        let unit = '';
        if (val < 1000) {
          endVal = val;
        } else if (val >= 1000 && val < 1000000) {
          endVal = parseInt(val / 1000);
          unit = 'K+';
        } else if (val >= 1000000 && val < 10000000000) {
          endVal = parseInt(val / 1000000);
          unit = 'M+';
        } else {
          endVal = parseInt(val / 1000000000);
          unit = 'B+';
        }
        return {
          val: endVal,
          unit: unit
        };
      }
    },
    mounted(){
      let res = this.transformValue(this.val);
      this.endVal = res.val;
      this.unit = res.unit;
    }
  }
</script>
<style>

</style>
