package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"github.com/confluentinc/confluent-kafka-go/kafka"
	"github.com/golang/glog"
	"github.com/golang/protobuf/proto"
	"golang.org/x/net/context"
	"io"
	"novumind/goku/go/common/db"
	msg "novumind/goku/go/common/kafka"
	mpb "novumind/goku/proto/go/msg"
	spb "novumind/goku/proto/go/storage"
	wpb "novumind/goku/proto/go/worker"
	"os/exec"
	"strconv"
	"strings"
	"sync"
	"time"
)

type WorkerManager struct {
	WorkerClient wpb.WorkerClient
	MysqlClient  *db.MysqlClient
	TaskQueue    []*kafka.Message
	TaskMap      map[string]int // Used to avoid repeate consuming kafka message
	Producer     *msg.Producer
	Consumer     *msg.Consumer
	Mutex        sync.Mutex
	StartChan    chan int
	QuitChan     chan int
	Restart      bool
}

// Listen the "start" topic and the specified task topic at the same time
func (wm *WorkerManager) ListenKafka() {
	// Used for "start" topic
	glog.Error("Error when pull message")
	for {
		msg, err := wm.Consumer.Pull(100)
		if err != nil {
			glog.Error("Error when pull message: ", err)
			continue
		}
		if *msg.TopicPartition.Topic == "start" {
			wm.Mutex.Lock()
			wm.Restart = false
			wm.Mutex.Unlock()
			wm.StartChan <- 1
			wm.Consumer.AckMsg(msg)
			continue
		} else if *msg.TopicPartition.Topic == "stop" {
			wm.Mutex.Lock()
			wm.Restart = true
			wm.Mutex.Unlock()
			wm.Consumer.AckMsg(msg)
			continue
		}
		// Task topic message
		var taskMsg = new(mpb.RequiredModelsMsg)
		if err := proto.Unmarshal(msg.Value, taskMsg); err != nil {
			glog.Errorf("Error when parse the message! %v\n", err)
			continue
		}
		// Just for debug
		task, err := wm.MysqlClient.GetTask(taskMsg.TaskId)
		if err != nil {
			glog.Errorf("Error when get task info from database!",
				"Error: %v, %s\n", err, task.VideoUrl)
			continue
		}
		glog.Info("Receive task :", task.Id, task.VideoUrl)
		if _, exist := wm.TaskMap[taskMsg.TaskId]; exist {
			glog.Info("!!!!!!!!!!!!!Duplate message!!!!!!!!!!!!!!: ", task.Id, task.VideoUrl)
			continue
		}
		wm.Mutex.Lock()
		wm.TaskQueue = append(wm.TaskQueue, msg)
		wm.TaskMap[taskMsg.TaskId] = 1
		wm.Mutex.Unlock()
		glog.Info("After append task, the length of the queue is:", len(wm.TaskQueue))
	}
}

// Before annotating the videos, send the first "displayNum"th tasks id to the frontend
// to display the task picture on the webpage.
func (wm *WorkerManager) SendInitTasksToFront() {
	for {
		glog.Info("Preparing send the task id to frontend!")
		taskLen := len(wm.TaskQueue)
		glog.Info("The task queue length is ", taskLen)
		if taskLen < *displayNum {
			time.Sleep(2 * time.Second)
			continue
		}
		for i := 0; i < *displayNum; i++ {
			wm.Mutex.Lock()
			msg := wm.TaskQueue[i]
			wm.Mutex.Unlock()
			var taskMsg = new(mpb.RequiredModelsMsg)
			if err := proto.Unmarshal(msg.Value, taskMsg); err != nil {
				glog.Errorf("Error when parse the message! %v\n", err)
			}
			glog.Info(taskMsg)
			jsonBytes, _ := json.Marshal(taskMsg)
			tp, err := wm.Producer.Push(*queueTopic, jsonBytes)
			if err != nil {
				glog.Error("Produce message failed: ", err)
				// Probable generate an infinite loop or duplicate messages if kafka down or unstable
				i--
				time.Sleep(2 * time.Second)
			}
			// Just for debug
			if tp != nil {
				glog.Infof("Produce message to topic %s[%d] at offset %v\n",
					*tp.Topic, tp.Partition, tp.Offset)
			}
		}
		break
	}
}

func (wm *WorkerManager) CheckInference() {
	<-wm.StartChan
	glog.Info("Start inference ..............................")
	req := &wpb.CheckInferenceCoreRequest{}
	for {
		res, err := wm.WorkerClient.CheckInferenceCore(context.Background(), req)
		if err != nil {
			glog.Errorf("Error when check the inference! %v\n", err)
			time.Sleep(2 * time.Second)
			continue
		}
		glog.Info("Check inference core response: ", res.IsAble, " ", wm.Restart, " ", len(wm.TaskQueue))
		if res.IsAble && !wm.Restart && len(wm.TaskQueue) > 0 {
			glog.Info("The task queue length: ", len(wm.TaskQueue))
			// Get the first task from the task queue
			msg := wm.TaskQueue[0]
			var taskMsg = new(mpb.RequiredModelsMsg)
			if err := proto.Unmarshal(msg.Value, taskMsg); err != nil {
				glog.Errorf("Error when parse the message! %v\n", err)
				// TODO: Need to send progress message to front?
				wm.RemoveFirstTask(taskMsg.TaskId)
				err = wm.Consumer.AckMsg(msg)
				if err != nil {
					glog.Error("Error when ack kafka message.", err)
				}
				continue
			}
			task, err := wm.MysqlClient.GetTask(taskMsg.TaskId)
			if err != nil {
				glog.Errorf("Error when get task from database : %v, %s\n", err)
				time.Sleep(2 * time.Second)
				continue
			}
			task.StatusCode = spb.Task_RUNNING
			err = wm.MysqlClient.UpdateTask(task)
			if err != nil {
				glog.Errorf("Error when update task status 'RUNNING' to database!  %v, %s\n",
					err, task.VideoUrl)
				time.Sleep(2 * time.Second)
				continue
			}
			// Start annotate the video
			err = wm.AnnotateVideo(task)
			if wm.Restart {
				continue
			}
			if err != nil {
				glog.Errorf("Error when annotate the video! %v\n", err)
				task.StatusCode = spb.Task_FAILED
				task.EndTime = time.Now().UnixNano() // Unit: ns
				err = wm.MysqlClient.UpdateTask(task)
				if err != nil {
					glog.Errorf("Error when update task status 'FAILED' to database! %v, %s\n",
						err, task.VideoUrl)
					time.Sleep(2 * time.Second)
					continue
				}
			}
			glog.Info("Commint msg")
			// Commit the kafka offset
			err = wm.Consumer.AckMsg(msg)
			if err != nil {
				glog.Error("Error when ack kafka message.", err)
			}
			wm.RemoveFirstTask(task.Id)
			go wm.SendTaskToFront()
		} else {
			time.Sleep(2 * time.Second)
		}
	}
	wm.QuitChan <- 1
}

// TODO: annotate video by specified model ids
func (wm *WorkerManager) AnnotateVideo(task *spb.Task) error {
	req := &wpb.AnnotateVideoRequest{
		RequestId: task.Id,
		VideoPath: task.VideoUrl,
	}
	var err error
	var resultsMap map[string][]*spb.FrameMetadata
	var totalNum int64
	var fps float64
	fps = GetVideoFps(task.VideoUrl)
	if fps <= 0 {
		glog.Error("Error when get the fps of the video!")
		return errors.New("Error when get the fps of the video!")
	}
ForLoop:
	for {
		if wm.Restart {
			return nil
		}
		glog.Info("Annotate video request: ", req)
		stream, err := wm.WorkerClient.AnnotateVideo(context.Background(), req)
		if err != nil {
			glog.Errorf("Fail to call AnnotateVideo!%v\n", err)
			time.Sleep(2 * time.Second)
			continue
		}
		// resultMap, key is the model id and value is the model output
		resultsMap = make(map[string][]*spb.FrameMetadata, 0)
		for {
			res, err := stream.Recv()
			if err != nil {
				if err == io.EOF {
					glog.Info("=============Finished!=============")
					break ForLoop
				} else {
					glog.Errorf("Error when fetching AnnotateVideo response !%v\n", err)
					return err
				}
			}
			// Process the metadata
			for _, metadata := range res.Metadatas {
				resultsMap[metadata.ModelId] = append(resultsMap[metadata.ModelId], metadata)
			}
			totalNum = res.TotalNum
			if res.ProcessedNum != res.TotalNum {
				go wm.SendTaskProgressToFront(res.ProcessedNum, res.TotalNum, task.Id)
			}
		}
	}
	// Update task status and end time
	task.StatusCode = spb.Task_SUCCESS
	task.EndTime = time.Now().UnixNano() // Unit: ns
	err = wm.MysqlClient.UpdateTask(task)
	if err != nil {
		glog.Error("Error when update task status 'SUCCESS' to database!", err)
		return errors.New("Error when update task status 'SUCCESS' to database!")
	}
	// Insert the task result into the database
	for modelId, result := range resultsMap {
		// SnapShot the image
		exampleInfo, err := SnapShotVideo(task.VideoUrl, result, 0, fps)
		// Just for debug
		glog.Info("modelId: ", modelId, "tag: ", exampleInfo.Tag, "confidence: ", exampleInfo.Confidence)
		if err != nil {
			glog.Error("Error when snapshot the video for example info!!!")
			continue
		}
		modelOutput := &spb.ModelOutput{
			Metadatas:   result,
			ExampleInfo: exampleInfo,
		}
		err = wm.MysqlClient.InsertTaskResult(task, modelId, modelOutput)
		if err != nil {
			glog.Error("Error when write task result into database!", err)
		}
	}
	wm.SendTaskProgressToFront(totalNum, totalNum, task.Id)
	return nil
}

func (wm *WorkerManager) SendTaskProgressToFront(processedNum, totalNum int64, taskId string) error {
	// Send message to the front end to display the progress
	progressMsg := mpb.TaskProgressMsg{
		TaskId:           taskId,
		ProcessedNum:     processedNum,
		TotalNum:         totalNum,
		ProcessedOnceNum: 64,
	}
	if processedNum == totalNum {
		if totalNum%64 != 0 {
			progressMsg.ProcessedOnceNum = totalNum % 64
		}
	}
	glog.Info("The processed num: ", progressMsg.ProcessedNum, "	The total num: ",
		progressMsg.TotalNum, "   The processed once num: ", progressMsg.ProcessedOnceNum)
	jsonBytes, _ := json.Marshal(progressMsg)
	tp, err := wm.Producer.Push(*progressTopic, jsonBytes)
	if err != nil {
		glog.Error("Produce message failed: ", err)
		return err
	}
	// Just for debug
	if tp != nil {
		glog.Infof("Produce message to topic %s[%d] at offset %v\n",
			*tp.Topic, tp.Partition, tp.Offset)
	}
	return nil
}

func (wm *WorkerManager) SendTaskToFront() {
	// Send the displayNumth video to frontend
	wm.Mutex.Lock()
	var msg *kafka.Message
	var taskMsg = new(mpb.RequiredModelsMsg)
	if len(wm.TaskQueue) > *displayNum {
		msg = wm.TaskQueue[*displayNum-1]
	} else {
		if len(wm.TaskQueue) == 0 {
			wm.Mutex.Unlock()
			return
		}
		msg = wm.TaskQueue[len(wm.TaskQueue)-1]
	}
	if err := proto.Unmarshal(msg.Value, taskMsg); err != nil {
		glog.Errorf("Error when parse the message! %v\n", err)
	}
	glog.Info(taskMsg)
	jsonBytes, _ := json.Marshal(taskMsg)
	//err = wm.Producer.Produce("queue", jsonBytes)
	tp, err := wm.Producer.Push(*queueTopic, jsonBytes)
	if err != nil {
		glog.Error("Produce message failed: ", err)
	}
	if tp != nil {
		glog.Infof(
			"Produce message to topic %s[%d] at offset %v\n", *tp.Topic, tp.Partition, tp.Offset)
	}
	wm.Mutex.Unlock()
}

func (wm *WorkerManager) RemoveFirstTask(taskId string) {
	// Delete the first task from the task queue
	wm.Mutex.Lock()
	if len(wm.TaskQueue) > 0 {
		wm.TaskQueue = wm.TaskQueue[1:]
	}
	delete(wm.TaskMap, taskId)
	wm.Mutex.Unlock()
}

func GetVideoFps(videoPath string) float64 {
	var buffer bytes.Buffer
	glog.Info("Get the fps of the video:", videoPath)
	ffmpegCommand := "ffmpeg -i " + videoPath + " 2>&1 | sed -n \"s/.*, \\(.*\\) fp.*/\\1/p\""
	cmd := exec.Command("bash", "-c", ffmpegCommand)
	cmd.Stdout = &buffer
	err := cmd.Run()
	if err != nil {
		glog.Error("Could not get snapshot for the specified frame number!", err)
	}
	fps, _ := strconv.ParseFloat(strings.Trim(buffer.String(), "\n"), 64)
	glog.Info("The fps of the video:", fps)
	return fps
}

func SnapShotVideo(videoPath string, result []*spb.FrameMetadata, frameNum int64,
	fps float64) (*spb.ExampleInfo, error) {
	var cmd *exec.Cmd
	var buffer bytes.Buffer
	var tag string
	var confidence float32
	if frameNum == -1 {
		cmd = exec.Command(
			"ffmpeg", "-i", videoPath, "-vframes", "1", "-s", "160x100", "-f", "singlejpeg", "-")
		cmd.Stdout = &buffer
	} else {
		// Get the frame num
		for _, metadata := range result {
			if metadata.Details != nil {
				frameNum = metadata.FrameNum
				tag = metadata.Details[0].Class[0]
				confidence = float32(metadata.Details[0].Confidence[0])
			}
		}
		glog.Info("Get the frameNum and confidence and tag are : ",
			frameNum, " ", confidence, " ", tag)
		// Calculate the timeOffset
		timeOffset := fmt.Sprintf("%.3f", (float64(frameNum) / fps))
		glog.Info("The offset of the video:", timeOffset)
		buffer.Reset()
		cmd = exec.Command("ffmpeg", "-i", videoPath, "-vframes", "1", "-ss", timeOffset,
			"-s", "320x200", "-f", "singlejpeg", "-")
		cmd.Stdout = &buffer
	}
	if err := cmd.Run(); err != nil {
		glog.Error("Could not get a snapshot from the video!", err)
		return nil, err
	}
	exampleInfo := &spb.ExampleInfo{
		FrameNum:   int32(frameNum),
		Tag:        tag,
		Confidence: confidence,
		Image:      buffer.Bytes(),
	}
	return exampleInfo, nil
}
