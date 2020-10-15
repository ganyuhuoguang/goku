package main

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"github.com/golang/glog"
	"github.com/golang/protobuf/proto"
	"io/ioutil"
	"math/rand"
	db "novumind/goku/go/common/db"
	"novumind/goku/go/common/id"
	"novumind/goku/go/common/kafka"
	mpb "novumind/goku/proto/go/msg"
	spb "novumind/goku/proto/go/storage"
	"os/exec"
	"path"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

var filePath = flag.String("file_path", "", "The config file path.")

func CheckErr(str string, err error, exit bool) {
	if err == nil {
		return
	}
	if exit {
		glog.Fatal(str, err)
	}
	glog.Error(str, err)
	return
}

func main() {
	flag.Parse()
	if *filePath == "" {
		CheckErr("Error when get filepath", errors.New("The filepath is invalid!"), true)
	}

	config, err := NewConfig(*filePath)
	CheckErr("Error when reading the config file", err, true)
	glog.Info(config)

	mysqlAddr, err := config.GetString("mysql")
	CheckErr("Error when get mysql address.", err, true)

	client, err := db.NewMysqlClient(mysqlAddr)
	CheckErr("Error when create mysql client", err, true)
	defer client.DB.Close()

	tags, err := config.GetStringSlice("tags")
	CheckErr("Error when get tags.", err, true)

	modelNum, err := config.GetInt("modelNum")
	CheckErr("Error when get model number.", err, true)

	var ch = make(chan int, 0)

	glog.Info("===============Starting generate models===============")
	models, err := GenerateModel(client, modelNum)
	CheckErr("Error when generate models.", err, true)

	glog.Info("===============Starting generate model groups===============")
	modelGroups, err := GenerateModelGroup(config, client, models, tags)
	CheckErr("Error when generate model groups.", err, true)

	glog.Info("===============Starting create task===============")
	go CreateTask(config, client, modelGroups, ch)
	<-ch
}

func GenerateRandomInt(min, max int) int {
	rand.New(rand.NewSource(time.Now().UnixNano()))
	return rand.Intn(max-min) + min
}

func GenerateRandomString(length_str int) string {
	str := "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
	bytes := []byte(str)
	result := []byte{}
	r := rand.New(rand.NewSource(time.Now().UnixNano()))
	for i := 0; i < length_str; i++ {
		result = append(result, bytes[r.Intn(len(bytes))])
	}
	return string(result)
}

func GenerateModel(client *db.MysqlClient, modelNum int) ([]*spb.Model, error) {
	models := make([]*spb.Model, 0)
	for m := 0; m < modelNum; m++ {
		name := GenerateRandomString(GenerateRandomInt(4, 8))
		model := &spb.Model{
			Id:       strconv.Itoa(m),
			Name:     name,
			ModelDir: "/model_test",
			Loader: &spb.Loader{
				CaffeLoader: nil,
				TfLoader:    nil,
			},
			Tags: nil,
		}
		fmt.Println("Generate a model:", model)
		models = append(models, model)
	}
	glog.Info("===============Finished generating models===============")
	glog.Info("===============Insert models to database===============")
	for _, model := range models {
		var err = client.InsertModel(model)
		if err != nil {
			return nil, err
		}
	}
	return models, nil
}

func GenerateModelGroup(config *Config, client *db.MysqlClient, models []*spb.Model,
	tags []string) ([]*spb.ModelGroup, error) {
	modelGroups := make([]*spb.ModelGroup, 0)
	modelIds := make([]string, 0)
	for m := 0; m < len(tags); m++ {
		modelIndex, err := config.GetIntSlice(tags[m])
		CheckErr("Error when get model index", err, true)
		for n := 0; n < len(modelIndex); n++ {
			// May get a panic about out of range.
			modelIds = append(modelIds, models[modelIndex[n]].Id)

			model, err := client.GetModel(models[modelIndex[n]].Id)
			CheckErr("Error when get model.", err, true)

			model.Tags = append(model.Tags, tags[m])

			err = client.UpdateModel(model)
			CheckErr("Error update model tags.", err, true)
		}
		modelGroup := &spb.ModelGroup{
			Id:        id.GenerateId(),
			TopicName: tags[m],
			ModelIds:  modelIds,
		}
		glog.Info("Generate a model group", modelGroup)
		modelGroups = append(modelGroups, modelGroup)
		modelIds = modelIds[:0]
	}
	glog.Info("===============Finished generating model groups===============")
	glog.Info("===============Insert model groups to database===============")
	for _, modelGroup := range modelGroups {
		var err = client.InsertModelGroup(modelGroup)
		if err != nil {
			return nil, err
		}
	}
	return modelGroups, nil
}

func HeartBeatWithKafKa(producer *kafka.Producer) {
	ticker := time.NewTicker(2 * time.Minute)
	for {
		<-ticker.C
		metadata, err := producer.Producer.GetMetadata(nil, true, 1000)
		if err != nil {
			glog.Info("Error when get metadata from kafka! ", err)
		} else {
			glog.Info(metadata)
		}
	}
}

func CreateTask(config *Config, client *db.MysqlClient, modelGroups []*spb.ModelGroup, ch chan int) {
	dirPath, err := config.GetString("dirPath")
	CheckErr("Error when get the video path.", err, true)

	// Init kafka producer for api server
	kafkaAddr, err := config.GetString("kafka")
	CheckErr("Error when get the kafka addr.", err, true)

	producer, err := kafka.NewProducer(kafkaAddr, true)
	CheckErr("Error to init the kafka producer!", err, true)
	defer producer.Close()

	go HeartBeatWithKafKa(producer)
	glog.Info(config, dirPath, kafkaAddr, producer)
	for {
		demoTasks, err := client.GetDemoTasks()
		CheckErr("Error when get tasks from database!", err, true)
		if len(demoTasks) <= 99 {
			videosPath, err := GetVideoPathFromDir(dirPath)
			CheckErr("Error when get video path from dir", err, true)

			for index, videoPath := range videosPath {
				// snapshot the video and save the image bytes to task proto
				glog.Info("Snapshot for the task: ", videoPath)
				imageData, err := SnapShotVideo(videoPath)
				CheckErr("The video path or video is invalid!", err, true)

				modelIds, topicName := GenerateModelIdsForVideo(modelGroups, len(modelGroups), index)
				task := &spb.Task{
					Id:         id.GenerateId(),
					VideoUrl:   videoPath,
					StartTime:  time.Now().UnixNano(),
					EndTime:    0,
					StatusCode: spb.Task_PENDING,
					ModelIds:   modelIds,
					Snapshot:   imageData,
				}

				glog.Info("===============Insert task to database===============")

				err = client.InsertTask(task)
				CheckErr("Error when insert task to database! ", err, true)

				msg := &mpb.RequiredModelsMsg{
					TaskId:    task.Id,
					VideoPath: task.VideoUrl,
				}
				msgProtoBytes, err := proto.Marshal(msg)
				CheckErr("Error marshal proto: ", err, true)

				_, err = producer.Push(topicName, msgProtoBytes)
				glog.Info("The task id is ", msg.TaskId)
				CheckErr("Produce message failed: ", err, true)
			}
		} else {
			time.Sleep(2 * time.Second)
		}
	}
	ch <- 1
}

func GenerateModelIdsForVideo(
	modelGroups []*spb.ModelGroup, modelGroupNum, index int) ([]string, string) {
	glog.Info("Topic: ", modelGroups[index%modelGroupNum].TopicName)
	return modelGroups[index%modelGroupNum].ModelIds, modelGroups[index%modelGroupNum].TopicName
}

func GetVideoPathFromDir(dirPath string) ([]string, error) {
	videosPath := make([]string, 0)
	videoFiles, err := ioutil.ReadDir(dirPath)
	if err != nil {
		return nil, err
	}
	for _, file := range videoFiles {
		fileSuffix := filepath.Ext(file.Name())
		if strings.Compare(fileSuffix, ".mp4") == 0 {
			videosPath = append(videosPath, path.Join(dirPath, file.Name()))
		}
	}
	return videosPath, nil
}

func SnapShotVideo(videoPath string) ([]byte, error) {
	glog.Info("Snapshot the video: ", videoPath)
	var cmd *exec.Cmd
	var buffer bytes.Buffer
	cmd = exec.Command(
		"ffmpeg", "-i", videoPath, "-vframes", "1", "-s", "160x100", "-f", "singlejpeg", "-")
	cmd.Stdout = &buffer
	if err := cmd.Run(); err != nil {
		glog.Error("Could not get a snapshot from the video!", err)
		return nil, err
	}
	return buffer.Bytes(), nil
}
