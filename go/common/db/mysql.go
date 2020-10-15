package db

import (
	"database/sql"
	"novumind/goku/go/common/id"
	spb "novumind/goku/proto/go/storage"
	wpb "novumind/goku/proto/go/worker"

	_ "github.com/go-sql-driver/mysql"
	"github.com/golang/protobuf/proto"
)

type MysqlClient struct {
	DB *sql.DB
}

// Client for mysql read/write
func NewMysqlClient(serverAddr string) (*MysqlClient, error) {
	conn := "root:novumind" + "@tcp(" + serverAddr + ")/goku?charset=utf8"
	db, err := sql.Open("mysql", conn)
	if err != nil {
		return nil, err
	}
	return &MysqlClient{
		DB: db,
	}, nil
}

func (mc *MysqlClient) Transaction() (*sql.Tx, error) {
	tx, err := mc.DB.Begin()
	if err != nil {
		return nil, err
	}
	return tx, nil
}

// Add model to database
func (mc *MysqlClient) InsertModel(model *spb.Model, modifiedTime int64, tx *sql.Tx) error {
	modelInfoBytes, _ := proto.Marshal(model)
	var err error
	if tx != nil {
		_, err = tx.Exec(
			"INSERT INTO model(id, name, modified_time, info) VALUES (?, ?, ?, ?)",
			model.Id, model.Name, modifiedTime, modelInfoBytes)
	} else {
		_, err = mc.DB.Exec(
			"INSERT INTO model(id, name, modified_time, info) VALUES (?, ?, ?, ?)",
			model.Id, model.Name, modifiedTime, modelInfoBytes)
	}
	return err
}

// Get model by model_id
func (mc *MysqlClient) GetModel(modelId string) (*spb.Model, error) {
	var model spb.Model
	var modelInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM model where id = ?", modelId)
	if err := row.Scan(&modelInfoBytes); err != nil {
		return nil, err
	}
	_ = proto.Unmarshal(modelInfoBytes, &model)
	return &model, nil
}

// Get model by model_name
func (mc *MysqlClient) GetModelByName(modelName string) (*spb.Model, error) {
	var model spb.Model
	var modelInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM model where name = ?", modelName)
	if err := row.Scan(&modelInfoBytes); err != nil {
		return nil, err
	}
	_ = proto.Unmarshal(modelInfoBytes, &model)
	return &model, nil
}

// Update model
func (mc *MysqlClient) UpdateModel(model *spb.Model, modifiedTime int64, tx *sql.Tx) error {
	modelInfoBytes, _ := proto.Marshal(model)
	var err error
	if tx != nil {
		_, err = tx.Exec("UPDATE model SET info= ?,modified_time= ? WHERE id = ?", modelInfoBytes, modifiedTime, model.Id)
	} else {
		_, err = mc.DB.Exec("UPDATE model SET info= ?,modified_time= ? WHERE id = ?", modelInfoBytes, modifiedTime, model.Id)
	}
	return err
}

// Get models
func (mc *MysqlClient) GetModels() ([]*spb.Model, error) {
	modelSlice := make([]*spb.Model, 0)
	rows, err := mc.DB.Query("SELECT info FROM model ORDER BY name")
	if err != nil {
		return nil, err
	}
	for rows.Next() {
		var modelInfoBytes []byte
		var modelInfo spb.Model
		if err := rows.Scan(&modelInfoBytes); err != nil {
			return nil, err
		}
		_ = proto.Unmarshal(modelInfoBytes, &modelInfo)
		modelSlice = append(modelSlice, &modelInfo)
	}
	return modelSlice, nil
}

// Add model group to database
func (mc *MysqlClient) InsertModelGroup(modelGroup *spb.ModelGroup, modifiedTime int64, tx *sql.Tx) error {
	modelGroupInfoBytes, _ := proto.Marshal(modelGroup)
	var err error
	if tx != nil {
		_, err = tx.Exec("INSERT INTO model_group(id, name, modified_time, info) VALUES (?, ?, ?, ?)", modelGroup.Id, modelGroup.TopicName, modifiedTime, modelGroupInfoBytes)
	} else {
		_, err = mc.DB.Exec("INSERT INTO model_group(id, name, modified_time, info) VALUES (?, ?, ?, ?)", modelGroup.Id, modelGroup.TopicName, modifiedTime, modelGroupInfoBytes)
	}
	return err
}

// Add model group to database
func (mc *MysqlClient) UpdateModelGroup(modelGroup *spb.ModelGroup, modifiedTime int64, tx *sql.Tx) error {
	modelGroupInfoBytes, _ := proto.Marshal(modelGroup)
	var err error
	if tx != nil {
		_, err = tx.Exec("UPDATE model_group SET info=?,modified_time=? where id=?", modelGroupInfoBytes, modifiedTime, modelGroup.Id)
	} else {
		_, err = mc.DB.Exec("UPDATE model_group SET info=?,modified_time=? where id=?", modelGroupInfoBytes, modifiedTime, modelGroup.Id)
	}
	return err
}

// Get model group by model_group_id
func (mc *MysqlClient) GetModelGroup(modelGroupId string) (*spb.ModelGroup, error) {
	var modelGroupInfo spb.ModelGroup
	var modelGroupInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM model_group where id = ?", modelGroupId)
	if err := row.Scan(&modelGroupInfoBytes); err != nil {
		return nil, err
	}
	_ = proto.Unmarshal(modelGroupInfoBytes, &modelGroupInfo)
	return &modelGroupInfo, nil
}

// Get model group by model_group_id
func (mc *MysqlClient) GetModelGroupByName(modelGroupName string) (*spb.ModelGroup, error) {
	var modelGroupInfo spb.ModelGroup
	var modelGroupInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM model_group where name = ?", modelGroupName)
	if err := row.Scan(&modelGroupInfoBytes); err != nil {
		return nil, err
	}
	_ = proto.Unmarshal(modelGroupInfoBytes, &modelGroupInfo)
	return &modelGroupInfo, nil
}

// Get model groups
func (mc *MysqlClient) GetModelGroups() ([]*spb.ModelGroup, error) {
	modelGroupSlice := make([]*spb.ModelGroup, 0)
	rows, err := mc.DB.Query("SELECT id, info FROM model_group")
	if err != nil {
		return nil, err
	}
	for rows.Next() {
		var id string
		var modelGroupBytes []byte
		var modelGroup spb.ModelGroup
		if err := rows.Scan(&id, &modelGroupBytes); err != nil {
			return nil, err
		}
		_ = proto.Unmarshal(modelGroupBytes, &modelGroup)
		modelGroupSlice = append(modelGroupSlice, &modelGroup)
	}
	return modelGroupSlice, err
}

// Insert task to database
func (mc *MysqlClient) InsertTask(task *spb.Task) error {
	taskInfoBytes, _ := proto.Marshal(task)
	var status = task.StatusCode
	_, err := mc.DB.Exec(
		"INSERT INTO task(id, info, status, start_time, end_time) VALUES (?, ?, ?, ?, ?)",
		task.Id, taskInfoBytes, status, task.StartTime, task.EndTime)
	return err
}

// Get task by statusCode
func (mc *MysqlClient) GetDemoTasks() ([]*spb.Task, error) {
	taskSlice := make([]*spb.Task, 0)
	var rows *sql.Rows
	var err error
	rows, err = mc.DB.Query("SELECT info FROM task WHERE status in (0, 1) ORDER BY start_time")
	if err != nil {
		return nil, err
	}
	for rows.Next() {
		var taskBytes []byte
		var task spb.Task
		if err := rows.Scan(&taskBytes); err != nil {
			return nil, err
		}
		_ = proto.Unmarshal(taskBytes, &task)
		taskSlice = append(taskSlice, &task)
	}
	return taskSlice, nil
}

// Get task by task_id
func (mc *MysqlClient) GetTask(taskId string) (*spb.Task, error) {
	var taskInfo spb.Task
	var taskInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM task where id = ?", taskId)
	if err := row.Scan(&taskInfoBytes); err != nil {
		return nil, err
	}
	_ = proto.Unmarshal(taskInfoBytes, &taskInfo)
	return &taskInfo, nil
}

// Get task result
func (mc *MysqlClient) GetTaskResult(taskId string) ([]*spb.ModelOutput, error) {
	modelOutputSlice := make([]*spb.ModelOutput, 0)
	rows, err := mc.DB.Query("SELECT info FROM task_result where task_id = ?", taskId)
	if err != nil {
		return nil, err
	}
	for rows.Next() {
		var modelOutputBytes []byte
		var modelOutput spb.ModelOutput
		if err := rows.Scan(&modelOutputBytes); err != nil {
			return nil, err
		}
		_ = proto.Unmarshal(modelOutputBytes, &modelOutput)
		modelOutputSlice = append(modelOutputSlice, &modelOutput)
	}
	return modelOutputSlice, nil
}

// Get classify task result
func (mc *MysqlClient) GetClassifyTaskResult(taskId string) (*wpb.VideoSummaryResponse, error) {
	row := mc.DB.QueryRow("SELECT info FROM task_result where task_id = ?", taskId)
	var videoSummaryInfoBytes []byte
	var videoSummary wpb.VideoSummaryResponse
	if err := row.Scan(&videoSummaryInfoBytes); err != nil {
		if err != sql.ErrNoRows {
			return nil, err
		}
	}
	_ = proto.Unmarshal(videoSummaryInfoBytes, &videoSummary)
	return &videoSummary, nil
}

// Get task by statusCode
func (mc *MysqlClient) GetTasks(offset int, limit int, statusCode int) ([]*spb.Task, int, error) {
	taskSlice := make([]*spb.Task, 0)
	var num int
	var rows *sql.Rows
	var err error
	if statusCode == -1 { // get all tasks when status == -1
		rows, _ = mc.DB.Query("SELECT COUNT(*) AS count FROM task ")
		if rows.Next() {
			rows.Scan(&num)
		}
		rows, err = mc.DB.Query("SELECT info FROM task ORDER BY start_time DESC limit ?, ?", offset, limit)
	} else {
		rows, _ = mc.DB.Query("SELECT COUNT(*) AS count FROM task WHERE status = ?", statusCode)
		if rows.Next() {
			rows.Scan(&num)
		}
		rows, err = mc.DB.Query("SELECT info FROM task where status = ? ORDER BY start_time DESC limit ?, ?", statusCode, offset, limit)
	}
	if err != nil {
		return nil, num, err
	}
	for rows.Next() {
		var taskBytes []byte
		var task spb.Task
		if err := rows.Scan(&taskBytes); err != nil {
			return nil, num, err
		}
		_ = proto.Unmarshal(taskBytes, &task)
		taskSlice = append(taskSlice, &task)
	}
	return taskSlice, num, nil
}

// Update Task time and status
func (mc *MysqlClient) UpdateTaskTimeAndStatus(taskId string) error {
	rows, err := mc.DB.Query("SELECT MAX(end_time), MIN(start_time), COUNT(*), MIN(end_time) FROM task_result WHERE task_id = ? ", taskId)
	if err != nil {
		return err
	}
	var maxEndTime, minStartTime, minEndTime int64
	var resultNum int
	if rows.Next() {
		rows.Scan(&maxEndTime, &minStartTime, &resultNum, &minEndTime)
	}
	var taskInfo spb.Task
	var taskInfoBytes []byte
	row := mc.DB.QueryRow("SELECT info FROM task WHERE id = ?", taskId)
	if err := row.Scan(&taskInfoBytes); err != nil {
		return err
	}
	_ = proto.Unmarshal(taskInfoBytes, &taskInfo)
	modelNum := len(taskInfo.ModelGroupIds)
	taskStatus := taskInfo.StatusCode
	if taskStatus == spb.Task_PENDING { // if task's status still is  PENDING and one of its' jobs is success or running, update the task
		if resultNum != 0 {
			if resultNum == modelNum && minEndTime != 0 {
				taskInfo.StartTime = minStartTime
				taskInfo.EndTime = maxEndTime
				taskInfo.StatusCode = spb.Task_SUCCESS
				taskInfoBytesTmp, _ := proto.Marshal(&taskInfo)
				_, err := mc.DB.Exec("UPDATE task SET status = ?, start_time = ?, end_time = ?, info = ? WHERE id = ?", spb.Task_SUCCESS, minStartTime, maxEndTime, taskInfoBytesTmp, taskId)
				return err
			} else {
				taskInfo.StartTime = minStartTime
				taskInfo.StatusCode = spb.Task_RUNNING
				taskInfoBytesTmp, _ := proto.Marshal(&taskInfo)
				_, err := mc.DB.Exec("UPDATE task SET status = ?, start_time = ?,  info = ? WHERE id = ?", spb.Task_RUNNING, minStartTime, taskInfoBytesTmp, taskId)
				return err
			}
		} else {
			return nil
		}
	} else if taskStatus == spb.Task_RUNNING { //if task's status is RUNNING and all its' jobs is finished, update the task
		if resultNum == modelNum {
			if minEndTime != 0 {
				taskInfo.EndTime = maxEndTime
				taskInfo.StatusCode = spb.Task_SUCCESS
				taskInfoBytesTmp, _ := proto.Marshal(&taskInfo)
				_, err := mc.DB.Exec("UPDATE task SET end_time = ?, status = ?, info = ? WHERE id = ?", maxEndTime, spb.Task_SUCCESS, taskInfoBytesTmp, taskId)
				return err
			} else {
				return nil
			}
		}
	} else { //if task's status is FINISH or FAILED, do nothing
		return nil
	}
	return nil
}

// Update TaskInfo
func (mc *MysqlClient) UpdateTask(taskInfo *spb.Task) error {
	taskInfoBytes, _ := proto.Marshal(taskInfo)
	_, err := mc.DB.Exec("UPDATE task SET start_time = ?, end_time = ?, status = ?, info = ? WHERE id = ?", taskInfo.StartTime, taskInfo.EndTime, taskInfo.StatusCode, taskInfoBytes, taskInfo.Id)
	return err
}

// Insert TaskResult
func (mc *MysqlClient) InsertTaskResult(task *spb.Task, modelId string, taskResult *spb.ModelOutput) error {
	id := id.GenerateId()
	taskResultBytes, _ := proto.Marshal(taskResult)
	_, err := mc.DB.Exec(
		"INSERT INTO task_result(id, task_id, model_id, info, start_time, end_time) VALUES (?, ?, ?, ?, ?, ?)",
		id, task.Id, modelId, taskResultBytes, task.StartTime, 0)
	return err
}

func (mc *MysqlClient) Close() error {
	err := mc.DB.Close()
	return err
}
