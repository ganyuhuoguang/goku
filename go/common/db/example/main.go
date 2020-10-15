package main

import (
	"fmt"
	"math/rand"
	db "novumind/goku/go/common/db"
	spb "novumind/goku/proto/go/storage"
	"strconv"
	"time"
)

func randSeq(n int) string {
	var letters = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
	b := make([]rune, n)
	for i := range b {
		b[i] = letters[rand.Intn(len(letters))]
	}
	return string(b)
}

func main() {
	var s1 = rand.NewSource(time.Now().Unix())
	var r1 = rand.New(s1)

	//***   test function InsertModel
	var groups = [5]string{"modelGroup01", "modelGroup02", "modelGroup03", "modelGroup04", "modelGroup05"}
	var matrix = [10][5]int{
		{1, 0, 1, 0, 0},
		{0, 1, 0, 0, 0},
		{0, 1, 0, 1, 0},
		{0, 0, 1, 0, 0},
		{0, 0, 1, 0, 1},
		{0, 1, 0, 0, 0},
		{0, 1, 1, 1, 0},
		{0, 1, 1, 0, 1},
		{1, 1, 0, 0, 0},
		{0, 1, 1, 1, 0},
	}

	var n, m int

	for n = 0; n < 10; n++ {
		id := strconv.Itoa(r1.Intn(1000000))
		name := randSeq(10)
		tags := make([]string, 0)
		for m = 0; m < 5; m++ {
			if matrix[n][m] == 1 {
				tags = append(tags, groups[m])
			}
		}
		model_test := &spb.Model{
			Id:       id,
			Name:     name,
			ModelDir: "/model_test",
			Loader: &spb.Loader{
				CaffeLoader: nil,
				TfLoader:    nil,
			},
			Tags: tags,
		}
		var client, _ = db.NewMysqlClient("127.0.0.1:3306")
		var err = client.InsertModel(model_test)
		if err != nil {
			fmt.Println(err)
		}
	}
	//***   test function GetModel
	var client, _ = db.NewMysqlClient("127.0.0.1:3306")
	modelIds := make([]string, 0)
	models, _ := client.GetModels()
	for _, model := range models {
		modelIds = append(modelIds, model.Id)
	}

	//***   test function InsertModelGroup
	for n = 0; n < 5; n++ {
		id := strconv.Itoa(r1.Intn(1000000))
		models := make([]string, 0)
		for m = 0; m < 10; m++ {
			if matrix[m][n] == 1 {
				models = append(models, modelIds[m])
			}
		}
		modelGroup := &spb.ModelGroup{
			Id:        id,
			TopicName: groups[n],
			ModelIds:  models,
		}
		err := client.InsertModelGroup(modelGroup)
		if err != nil {
			fmt.Println(err)
		}
	}

	//***   test function GetModelGroup
	var err error
	_, err = client.GetModelGroups()
	if err != nil {
		fmt.Println(err)
	}
	//***   test function InsertTask

	for n := 0; n < 10; n++ {
		t := time.Now()
		startTime := t.Unix() + int64(n)
		hh, _ := time.ParseDuration("8h")
		endTime := t.Add(hh).Unix() + int64(n)

		id := strconv.Itoa(r1.Intn(1000000))
		task := &spb.Task{
			Id:         id,
			VideoUrl:   "https://www.baidu.com",
			StartTime:  startTime,
			EndTime:    endTime,
			StatusCode: spb.Task_SUCCESS,
		}
		err = client.InsertTask(task)
		if err != nil {
			fmt.Println(err)
		}
	}

	//***   test function GetTask
	_, err = client.GetTask("546849")
	if err != nil {
		fmt.Println(err)
	}

	//***   test function GetTasks
	_, _, err = client.GetTasks(1, 25, -1)
	if err != nil {
		fmt.Println(err)
	}

	//*** test function InsertTask
	taskTest01 := &spb.Task{
		Id:         "6666",
		VideoUrl:   "https://www.baidu.com",
		StartTime:  0,
		EndTime:    0,
		StatusCode: 0,
	}
	err = client.InsertTask(taskTest01)
	if err != nil {
		fmt.Println(err)
	}
}
