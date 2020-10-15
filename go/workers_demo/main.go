package main

import (
	"flag"
	"github.com/confluentinc/confluent-kafka-go/kafka"
	"github.com/golang/glog"
	"google.golang.org/grpc"
	"novumind/goku/go/common/db" //novumind/goku/go/common/db  /mnt/data/myproject/goku-master/go/common/db
	msg "novumind/goku/go/common/kafka"
	wpb "novumind/goku/proto/go/worker"
)

var (
	inferenceAddr   = flag.String("inference_addr", "localhost:8089", "Inference grpc server address.")
	mysqlAddr       = flag.String("mysql_addr", "localhost:3306", "The mysql server address.")
	kafkaAddr       = flag.String("kafka_addr", "localhost:9092", "Kafka ip address.")
	startTopic      = flag.String("start_topic", "start", "Start task topic name.")
	stopTopic       = flag.String("stop_topic", "stop", "Stop task topic name.")
	taskTopic       = flag.String("task_topic", "violence", "Task topic name.")
	progressTopic   = flag.String("progress_topic", "progress", "Task progress topic name.")
	queueTopic      = flag.String("queue_topic", "queue", "Task progress topic name.")
	consumerGroupId = flag.String("consumer_group_id", "groupId", "Kafka consumer group id.")
	dirPath         = flag.String("dir_path", "/novumind/share/video-sample/", "The video path")
	displayNum      = flag.Int("display_num", 5, "The number of videos display on the webpage per worker")
)

func main() {
	flag.Parse()
	defer glog.Flush()

	glog.Info("I am listening the kafka topic: ", *taskTopic)
	// Init grpc client
	conn, err := grpc.Dial(*inferenceAddr, grpc.WithInsecure())
	if err != nil {
		glog.Fatalf("Can not connect to the inference server: %v\n", err)
	}
	workerClient := wpb.NewWorkerClient(conn)
	defer conn.Close()

	// Init mysql client
	mysqlClient, err := db.NewMysqlClient(*mysqlAddr)
	if err != nil {
		glog.Fatalf("Can not init the mysql client: %v\n", err)
	}
	defer mysqlClient.Close()

	// Init kafka producer
	producer, err := msg.NewProducer(*kafkaAddr, true)
	if err != nil {
		glog.Fatalf("Can not init the kafka producer: %v\n", err)
	}
	defer producer.Close()

	// Init kafka consumer
	consumer, err := msg.NewConsumer(*kafkaAddr, []string{*taskTopic, *startTopic,
		*stopTopic}, *consumerGroupId, false)
	if err != nil {
		glog.Fatalf("Can not init the kafka consumer: %v\n", err)
	}
	defer consumer.Close()
	// Init worker manager
	wm := &WorkerManager{
		WorkerClient: workerClient,
		MysqlClient:  mysqlClient,
		TaskQueue:    make([]*kafka.Message, 0),
		TaskMap:      make(map[string]int, 0),
		Producer:     producer,
		Consumer:     consumer,
		StartChan:    make(chan int, 1),
		QuitChan:     make(chan int, 1),
	}

	glog.Error("enter listen kafaka")
	// Listen to the message queue to get all task to the task queue
	go wm.ListenKafka()

	// Send the first X tasks to the frontend when start
	go wm.SendInitTasksToFront()

	// Check the inference component
	go wm.CheckInference()

	<-wm.QuitChan
}
