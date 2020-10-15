<template>
  <div>
    <div class="nav-title">{{$route.name}}</div>
    <!--<Row :gutter="10" class="search-bar">-->
    <!--<Col :lg="4" :sm="10" :md="6" :xs="10">-->
    <!--<Input v-model="searchOption.name" placeholder="请输入名字..."-->
    <!--class="input-border">-->
    <!--</Input>-->
    <!--</Col>-->
    <!--<Col :lg="4" :sm="4" :md="4" :xs="4">-->
    <!--<Button icon="search" @click="search()"-->
    <!--type="primary">搜索-->
    <!--</Button>-->
    <!--</Col>-->
    <!--</Row>-->
    <Row type="flex" justify="start" style="padding-right:15px">
      <Col span="1">
        <Button type="primary" icon="plus" @click="newModel">新增</Button>
      </Col>
    </Row>
    <Row class="line-break">
      <Table :data="modelList" :columns="modelColumns" stripe :loading="loading"></Table>
    </Row>
    <Modal v-model="addModelModal">
      <p slot="header">
        <span>模型接入</span>
      </p>
      <div>
        <Steps :current="current" size="small" style="margin-bottom:5%;">
          <Step title="模型类型"></Step>
          <Step title="模型"></Step>
        </Steps>
        <Form ref="addModel" :model="addModelForm" :rules="addModelRules" :label-width="150"
              label-position="left">
          <div v-show="current===0">
            <FormItem label="选择">
              <RadioGroup v-model="addModelForm.select">
                <Radio label="select">选择已有</Radio>
                <Radio label="new">新建</Radio>
              </RadioGroup>
            </FormItem>

            <FormItem label="类型" v-if="addModelForm.select==='select'" prop="modelGroup">
              <Select style="width:80%" v-model="addModelForm.modelGroup" :label-in-value="true"
                      placeholder="请选择类型" @on-change="setModelGroup" multiple>
                <Option v-for="item in groups" :value="item.id" :key="item.id"
                        :label="item.topicName"></Option>
              </Select>
            </FormItem>
            <FormItem v-if="addModelForm.select==='new'" label="类型" prop="modelType"
                      style="width:82%">
              <Input v-model="addModelForm.modelType" placeholder="模型类型"
                     @on-blur="setModelType"></Input>
            </FormItem>
          </div>
          <div v-show="current===1">
            <FormItem label="名称" prop="name" style="width:82%">
              <Input v-model="addModelForm.name" placeholder="模型名称"></Input>
            </FormItem>
            <FormItem label="模型文件路径" prop="modelParam.novuModelFile" style="width:82%">
              <Input v-model="addModelForm.modelParam.novuModelFile" placeholder="模型文件路径"></Input>
            </FormItem>
            <FormItem label="模型输入均值文件路径" style="width:82%">
              <Input v-model="addModelForm.modelParam.meanFile" placeholder="模型均值文件路径"></Input>
            </FormItem>
            <FormItem label="模型输出标签文件路径" prop="modelParam.labelFile" style="width:82%">
              <Input v-model="addModelForm.modelParam.labelFile" placeholder="模型标签文件路径"></Input>
            </FormItem>

            <FormItem label="归一化标准差" style="width:82%" prop="modelParam.stddev">
              <Input v-model="addModelForm.modelParam.stddev" placeholder="输入在需要归一化时的标准差"></Input>
            </FormItem>
            <FormItem label="Base Size" style="width:82%" prop="modelParam.baseSize">
              <Input v-model="addModelForm.modelParam.baseSize" placeholder="裁剪前的尺寸"></Input>
            </FormItem>
            <FormItem label="Top K" style="width:82%" prop="modelParam.topK">
              <Input v-model="addModelForm.modelParam.topK" placeholder="多分类返回Top K个输出结果"></Input>
            </FormItem>
            <FormItem label="通道顺序" style="width:82%">
              <RadioGroup v-model="addModelForm.channel" @on-change="setChannel">
                <Radio label="BGR">BGR</Radio>
                <Radio label="RGB">RGB</Radio>
              </RadioGroup>
            </FormItem>
            <FormItem label="标签阈值设定" style="width:82%">
              <Row>
                <Col :span="10" class="item-header" style="border-right:1px solid #dddee1">标签序号
                </Col>
                <Col :span="10" class="item-header">阈值</Col>
                <Col :span="2">
                  <Button type="dashed" shape="circle" icon="plus-round"
                          style="color:#19be6b"
                          @click="addItem()"></Button>
                </Col>
                <div v-for="(item,index) in addModelForm.modelParam.thresholdIdx" :key="index">
                  <Col
                    :span="10"><Input v-model="item.idx" class="input-border"
                                      @on-blur="checkInt(index)"></Input></Col>
                  <Col
                    :span="10"><Input v-model="item.threshold" class="input-border-right"
                                      @on-blur="checkFloat(index)"></Input></Col>
                  <Col
                    :span="2">
                    <Button type="dashed" shape="circle" icon="close" style="color:#ed3f14"
                            @click="removeItem(index)"></Button>
                  </Col>
                </div>
              </Row>
            </FormItem>
          </div>
        </Form>
      </div>
      <div slot="footer">
        <Button @click="back()" v-if="current===1">上一步</Button>
        <Button type="primary" @click="submitModel()" :loading="btnLoading" v-if="current===1">提交
        </Button>
        <Button v-if="current===0" type="primary" @click="next()">下一步</Button>
      </div>
    </Modal>
    <Modal v-model="updateModelGroupModal">
      <p slot="header">
        <span>模型类型管理</span>
      </p>
      <div>
        <Form ref="updateModelGroup" :model="updateModelGroup" :rules="updateModelGroupRules"
              :label-width="150"
              label-position="left">
          <FormItem label="类型" prop="modelTypes">
            <Select style="width:80%" v-model="updateModelGroup.modelTypes" :label-in-value="true"
                    placeholder="请选择类型" @on-change="changeModelGroup" multiple>
              <Option v-for="item in groups" :value="item.id" :key="item.id"
                      :label="item.topicName"></Option>
            </Select>
          </FormItem>
        </Form>
      </div>
      <div slot="footer">
        <Button type="primary" @click="submitModelGroup()" :loading="btnLoading1">提交
        </Button>
      </div>
    </Modal>
    <Modal v-model="modelModal">
      <p slot="header">
        <span>模型信息</span>
      </p>
      <div style="max-height: 500px;overflow: auto">
        <pre>{{JSON.stringify(model,undefined,2)}}</pre>
      </div>
    </Modal>
  </div>
</template>
<script>
  import {statusMap, taskStatusToShown, taskStatus, timeToString} from '@/utils/filter'
  import {listModels, listModelTypes, updateModelType, addModel, getModel} from '@/api/model'
  import {validateFloat, validateInt} from "../../utils/validate";

  export default {
    name: 'model',
    components: {},
    data() {
      const validInt = (rule, value, callback) => {
        if (value !== null && !validateInt(parseInt(value))) {
          callback(new Error('请填入整数'))
        } else {
          callback()
        }
      };
      const validFloat = (rule, value, callback) => {
        if (value !== null && !validateFloat(parseFloat(value))) {
          callback(new Error('请填入浮点数'))
        } else {
          callback()
        }
      };
      return {
        loading: false,
        modelList: [],
        modelColumns: [
          {
            title: '名字',
            key: 'name',
          }, {
            title: '应用类型',
            key: 'tags',
            align: 'center',
            render: (h, params) => {
              const item = params.row.tags;
              let tags = [];
              for (let t of item) {
                tags.push(
                  h('Tag', {
                    props: {
                      type: 'border',
                      color: 'blue'
                    }
                  }, t)
                )
              }
              tags.push(h('Button', {
                props: {
                  size: 'small',
                  type: 'text',
                  icon: 'edit'
                },
                on: {
                  click: () => {
                    listModelTypes().then(resp => {
                      this.groups = resp.data.modelTypes;
                      let types = []
                      for (let tag of item) {
                        const mg = this.groups.find(a => a.topicName === tag);
                        if (mg !== undefined) {
                          types.push(mg.id);
                        }
                      }
                      this.updateModelGroup.modelTypes = types;
                      this.updateModelGroup.modelId = params.row.id;
                      this.updateModelGroupModal = true;
                    }).catch(() => {
                      this.groups = [];
                    })
                  }
                }
              }, ''));
              return h('div', tags);
            }
          },
          {
            title: '操作',
            key: 'action',
            width: 150,
            align: 'center',
            render: (h, params) => {
              const row = params.row;
              return h('div', [
                h('Button', {
                  props: {
                    type: 'info',
                    size: 'small',
                  },
                  on: {
                    click: () => {
                      this.fetchModel(row.id);
                    }
                  }
                }, '查看')
              ]);
            }
          }
        ],
        addModelModal: false,
        addModelForm: {
          select: '',
          name: '',
          modelParam: {
            novuModelFile: null,
            caffeModelFile: null,
            caffeTrainedFile: null,
            meanFile: null,
            labelFile: null,
            stddev: null,
            baseSize: null,
            topK: null,
            isBgr: true,
            thresholdIdx: []
          },
          channel: 'BGR',
          tags: [],
          modelGroup: [],
          modelType: ''
        },
        addModelType: [],
        addModelRules: {
          name: [{required: true, message: '模型名称不能为空', trigger: 'blur'}],
          'modelParam.novuModelFile': [{required: true, message: '模型文件路径不能为空', trigger: 'blur'}],
          'modelParam.labelFile': [{required: true, message: '模型输出标签文件不能为空', trigger: 'blur'}],
          modelGroup: [{required: true, message: '模型类型不能为空', type: 'array', trigger: 'change'}],
          modelType: [{required: true, message: '模型类型不能为空', trigger: 'blur'}],
          'modelParam.stddev': [{validator: validFloat, trigger: 'blur'}],
          'modelParam.baseSize': [{validator: validInt, trigger: 'blur'}],
          'modelParam.topK': [{validator: validInt, trigger: 'blur'}]
        },
        modelLoading: false,
        btnLoading: false,
        btnLoading1: false,
        groups: [],
        updateModelGroupModal: false,
        updateModelGroup: {
          modelGroup: [],
          modelTypes: [],
          modelId: ''
        },
        updateModelGroupRules: {
          modelTypes: [{required: true, message: '模型类型不能为空', type: 'array', trigger: 'change'}]
        },
        valid1: true,
        valid2: true,
        current: 0,
        modelModal:false,
        model:''
      }
    },
    methods: {
      newModel() {
        listModelTypes().then(resp => {
          this.addModelModal = true;
          this.groups = resp.data.modelTypes;
          if (this.groups.length > 0) {
            this.addModelForm.select = 'select';
          } else {
            this.addModelForm.select = 'new';
          }
        }).catch(err => {
          this.groups = [];
        })
      },
      list() {
        listModels().then(resp => {
          this.modelList = resp.data.models;
          this.loading = false;
        }).catch(() => {
          this.modelList = [];
          this.loading = false;
        })
      },
      addItem() {
        const item = {};
        item.idx = 1;
        item.threshold = 0.1;
        this.addModelForm.modelParam.thresholdIdx.push(item);
      },
      removeItem(index) {
        this.addModelForm.modelParam.thresholdIdx.splice(index, 1);
        if (this.addModelForm.modelParam.thresholdIdx.length === 0) {
          this.valid2 = true;
          this.valid1 = true;
        }
      },
      setChannel(val) {
        if (val === 'BGR') {
          this.addModelForm.modelParam.isBgr = true;
        } else {
          this.addModelForm.modelParam.isBgr = false;
        }
      },
      setModelGroup(item) {
        this.addModelForm.tags = [];
        this.addModelType = [];
        for (let mg of item) {
          this.addModelForm.tags.push(mg.label);
          let mt = {};
          mt.id = mg.value;
          mt.name = mg.label;
          this.addModelType.push(mt);
        }
      },
      setModelType() {
        this.addModelType = [];
        this.addModelForm.tags.push(this.addModelForm.modelType);
        let mt = {};
        mt.id = "";
        mt.name = this.addModelForm.modelType;
        this.addModelType.push(mt);
      },
      submitModel() {
        this.$refs.addModel.validate(valid => {
          if (valid && this.valid1 && this.valid2) {
            this.btnLoading = true;
            let model = {};
            model.name = this.addModelForm.name;
            model.tags = this.addModelForm.tags;
            model.modelParam = this.addModelForm.modelParam;
            addModel(model, this.addModelType).then(resp => {
              this.addModelForm = {
                name: '',
                modelParam: {
                  novuModelFile: null,
                  caffeModelFile: null,
                  caffeTrainedFile: null,
                  meanFile: null,
                  labelFile: null,
                  stddev: null,
                  baseSize: null,
                  topK: null,
                  isBgr: true,
                  thresholdIdx: [],
                  channel: 'BGR'
                },
                tags: [],
                modelGroup: [],
                modelType: ''
              };
              this.current = 0;
              this.list();
              this.btnLoading = false;
              this.addModelModal = false;
            }).catch(err => {
              this.btnLoading = false;
              if (err.status === 400) {
                this.$Message.error('模型名称或模型类型已存在');
              } else {
                this.$Message.error('接入模型失败');
              }
            })
          } else {
            return false;
          }
        })
      },
      submitModelGroup() {
        this.$refs.updateModelGroup.validate(valid => {
          if (valid) {
            this.btnLoading1 = true;
            updateModelType(this.updateModelGroup.modelId, this.updateModelGroup.modelGroup).then(res => {
              this.list();
              this.updateModelGroupModal = false;
              this.btnLoading1 = false;
            }).catch(err => {
              this.btnLoading1 = false;
              this.$Message.error('更新信息失败');
            })
          } else {
            return false;
          }
        })
      },
      checkInt(index) {
        let val = this.addModelForm.modelParam.thresholdIdx[index].idx;
        if (val !== '') {
          if (validateInt(parseInt(val))) {
            this.addModelForm.modelParam.thresholdIdx[index].idx = parseInt(val);
            this.valid1 = true;
          } else {
            this.$Message.error('请填入整数');
            this.valid1 = false;
          }
        } else {
          this.valid1 = false;
        }
      },
      checkFloat(index) {
        let val = this.addModelForm.modelParam.thresholdIdx[index].threshold;
        if (val !== '') {
          if (validateFloat(parseFloat(val))) {
            this.addModelForm.modelParam.thresholdIdx[index].threshold = parseFloat(val);
            this.valid2 = true;
          } else {
            this.$Message.error('请填入浮点数');
            this.valid2 = false;
          }
        } else {
          this.valid2 = false;
        }
      },
      changeModelGroup(item) {
        this.updateModelGroup.modelGroup = [];
        for (let i of item) {
          const mg = this.groups.find(a => a.id === i.value);
          if (!mg.modelIds.includes(this.updateModelGroup.modelId)) {
            mg.modelIds.push(this.updateModelGroup.modelId);
          }
          this.updateModelGroup.modelGroup.push(mg);
        }
      },
      next() {
        if (this.addModelForm.select === 'new') {
          this.$refs.addModel.validateField('modelType', valid => {
            if (!valid) {
              this.current += 1;
            } else {
              return false;
            }
          })
        } else {
          this.$refs.addModel.validateField('modelGroup', valid => {
            if (!valid) {
              this.current += 1;
            } else {
              return false;
            }
          })
        }
      },
      back() {
        if (this.current > 0) {
          this.current -= 1;
        }
      },
      fetchModel(id) {
        getModel(id).then(resp => {
            this.model = resp.data.model;
            this.modelModal = true;
        }).catch(err => {
          this.$Message.error('获取数据失败')
        })
      }
    },
    created() {
      this.list();
    },
    mounted() {

    }
  }
</script>
<style scoped>
  .item-header {
    display: inline-block;
    background-color: #f8f8f9;
    border: 1px solid #e9eaec;
    text-align: center;
    font-weight: bold;
  }
</style>
