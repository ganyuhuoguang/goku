<template>
  <div :id="bindto"></div>
</template>
<script>
  import 'c3/c3.min.css'
  import c3 from 'c3/c3.min'

  export default {
    name: 'Chart',
    props: {
      bindto: {
        type: String,
        required: true
      },
      xs: Object,
      type: String,
      data: Array,
      tooltip: {
        type: Boolean,
        default: true
      },
      color: {
        type: Array,
        required: false,
        default: function () {
          return ['#2d8cf0', '#5DAC81', '#f25e43', '#FFB11B', '#6A4C9C']
        }
      },
      legendShow: {
        type: Boolean,
        default: true
      },
      xlabel: String,
      ylabel1: String,
      ylabel2: String,
      xmax: Number,
      ymax: Number,
      ymin: Number,
      xmin: Number,
      types: Object,
      legendHide: Array,
      axes: Object,
      params: Object,
      xgrid: Boolean,
      ygrid: Boolean,
      ylines: Array,
      rotated: Boolean,
      xtype: String,
      xcategories: Array
    },
    watch: {
      data() {
        this.generateGraph();
      }
    },
    computed: {
      ylabelShow() {
        if (this.ylabel2 !== undefined) {
          return true;
        } else {
          return false;
        }
      }
    },
    methods: {
      generateGraph() {
        c3.generate({
          bindto: '#' + this.bindto,
          color: {
            pattern: this.color
          },
          data: {
            xs: this.xs,
            columns: this.data,
            type: this.type,
            types: this.types,
            onclick: function (d, element) {
            },
            axes: this.axes
          },
          legend: {
            show: this.legendShow
          },
          axis: {
            rotated: this.rotated,
            x: {
              type: this.xtype,
              categories: this.xcategories,
              label: {
                text: this.xlabel,
                position: 'outer-center'
              },
              max: this.xmax
            },
            y: {
              label: {
                text: this.ylabel1,
                position: 'outer-middle'
              },
              min: this.ymin,
              max: this.ymax,
              padding: {bottom: 0}
//              tick: {
//                format: d3.format('.2f')
//              }
            },
            y2: {
              show: this.ylabelShow,
              label: {
                text: this.ylabel2,
                position: 'outer-middle'
              },
              min: this.ymin,
              max: this.ymax,
              padding: {bottom: 0}
//              tick: {
//                format: d3.format('.2f')
//              }
            }
          },
          gauge: {
            label: {
              format: function (value, ratio) {
                return value + '%'
              }
            }
          },
          tooltip: {
            show: this.tooltip,
            grouped: false,
            format: {
              title: function (x) {
                return  ' ' + x
              },
              value: function (value, ratio, id, index) {
                if (id !== 'Model') {
                  return value
                }
              }
            }
          },
          point: {
            r: 3
          },
          grid: {
            x: {
              show: this.xgrid
            },
            y: {
              show: this.ygrid,
              lines: this.ylines
            }
          }
        })
      }
    },
    mounted() {
      this.generateGraph();
    },
  }
</script>
<style>

</style>
