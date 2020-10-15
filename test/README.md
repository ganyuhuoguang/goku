# 视频分析平台压测
======================
## 机器数据
cpu: xeon 4核心

内存: 32GB

## 测试结果
### parallel=300 second=1
CONCURRENT REQUESTS: 300

TOTAL REQUESTS: 556

TOTAL TIME(s): 1.01506495476

SUCEED RATE: 100.00%

QPS: 547.75

Latency: 0.16
### parallel=900 second=10
CONCURRENT REQUESTS: 900

TOTAL REQUESTS: 4798

TOTAL TIME(s): 10.1738278866

SUCEED RATE: 100.00%

QPS: 471.60

Latency: 0.25

综上，当前机器峰值在550/s左右，在持续高并发情况下在470/s左右。
