package main

import (
	"github.com/golang/glog"
	"github.com/golang/protobuf/proto"
	"golang.org/x/net/context"
	"google.golang.org/grpc/codes"
	"google.golang.org/grpc/status"
	"novumind/goku/go/common/db"
	"novumind/goku/go/common/id"
	"novumind/goku/go/common/kafka"
	apb "novumind/goku/proto/go/api"
	mpb "novumind/goku/proto/go/msg"
	spb "novumind/goku/proto/go/storage"
	"time"
)

type ApiServer struct {
	MysqlClient *db.MysqlClient
	Producer    *kafka.Producer
}

func (api *ApiServer) CreateTask(ctx context.Context, req *apb.CreateTaskRequest) (
	*apb.CreateTaskResponse, error) {
	if req == nil || req.VideoPath == "" || req.ModelGroupNames == nil || len(req.ModelGroupNames) == 0 {
		glog.Error("The request for creating task is invalid")
		return nil, status.Errorf(codes.InvalidArgument, "The request for creating task is invalid!")
	}
	task := &spb.Task{
		Id:         id.GenerateId(),
		VideoUrl:   req.VideoPath,
		StartTime:  time.Now().UnixNano(),
		EndTime:    0,
		StatusCode: spb.Task_PENDING,
	}
	for _, modelGroupName := range req.ModelGroupNames {
		msg := &mpb.RequiredModelsMsg{
			TaskId:    task.Id,
			VideoPath: task.VideoUrl,
		}
		msgBytes, _ := proto.Marshal(msg)
		_, err := api.Producer.Push(modelGroupName, msgBytes)
		if err != nil {
			glog.Errorf("Fail to push topic\n%v", err)
			return nil, status.Errorf(codes.Internal, "Fail to create task")
		}
	}
	err := api.MysqlClient.InsertTask(task)
	if err != nil {
		glog.Errorf("Fail to write task in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to create task")
	}
	return &apb.CreateTaskResponse{
		TaskId: task.Id,
	}, nil
}

func (api *ApiServer) CreateClassifyTask(ctx context.Context, req *apb.CreateClassifyTaskRequest) (
	*apb.CreateClassifyTaskResponse, error) {
	if req == nil || req.VideoPath == "" || req.ModelGroupNames == nil || len(req.ModelGroupNames) == 0 {
		glog.Error("The request for creating task is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for creating task is invalid!")
	}
	classifyTaskId := id.GenerateId()
	classifyTask := &spb.Task{
		Id:         id.GenerateId(),
		VideoUrl:   req.VideoPath,
		StartTime:  time.Now().UnixNano(),
		EndTime:    0,
		StatusCode: spb.Task_PENDING,
	}
	err := api.MysqlClient.InsertTask(classifyTask)
	if err != nil {
		glog.Errorf("Fail to write task in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to create classify task")
	}
	for _, modelGroupName := range req.ModelGroupNames {
		msg := &mpb.RequiredModelsMsg{
			TaskId:    classifyTaskId,
			VideoPath: req.VideoPath,
		}
		msgBytes, _ := proto.Marshal(msg)
		_, err := api.Producer.Push(modelGroupName, msgBytes)
		if err != nil {
			glog.Errorf("Fail to push topic \n%v", err)
			return nil, status.Errorf(codes.Internal, "Fail to create task")
		}
	}
	return &apb.CreateClassifyTaskResponse{
		ClassifyTaskId: classifyTaskId,
	}, nil
}

func (api *ApiServer) GetTask(ctx context.Context, req *apb.GetTaskRequest) (
	*apb.GetTaskResponse, error) {
	if req == nil || req.TaskId == "" {
		glog.Error("The request for getting task is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting task is invalid!")
	}
	info, err := api.MysqlClient.GetTask(req.TaskId)
	if err != nil {
		glog.Errorf("Fail to get task in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get task in the database")
	}
	return &apb.GetTaskResponse{
		Task: info,
	}, nil
}

func (api *ApiServer) GetTaskResult(ctx context.Context, req *apb.GetTaskResultRequest) (
	*apb.GetTaskResultResponse, error) {
	if req == nil || req.TaskId == "" {
		glog.Error("The request for getting task result is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting task result is invalid!")
	}
	taskResults, err := api.MysqlClient.GetTaskResult(req.TaskId)
	if err != nil {
		glog.Error("Fail to get task result in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get task result")
	}
	var metadatas []*spb.FrameMetadata
	for _, taskResult := range taskResults {
		for _, metadata := range taskResult.Metadatas {
			metadatas = append(metadatas, metadata)
		}
	}
	return &apb.GetTaskResultResponse{
		Metadatas: metadatas,
	}, nil
}

func (api *ApiServer) GetClassifyTaskResult(
	ctx context.Context, req *apb.GetClassifyTaskResultRequest) (
	*apb.GetClassifyTaskResultResponse, error) {
	if req == nil || req.TaskId == "" {
		glog.Error("The request for getting classify task result is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting classify task result is invalid!")
	}
	taskResult, err := api.MysqlClient.GetClassifyTaskResult(req.TaskId)
	if err != nil {
		glog.Error("Fail to get classify task result in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get classify task result")
	}
	return &apb.GetClassifyTaskResultResponse{
		TaskResults: taskResult,
	}, nil
}

func (api *ApiServer) ListTasks(ctx context.Context, req *apb.ListTasksRequest) (*apb.ListTasksResponse, error) {
	if req == nil || req.Page == nil || req.SearchOption == nil {
		glog.Error("The request for getting task list is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting task list is invalid!")
	}
	offset := int((req.Page.PageIndex - 1) * req.Page.PageSize)
	limit := int(req.Page.PageSize)
	statusCode := int(req.SearchOption.StatusCode)
	taskList, num, err := api.MysqlClient.GetTasks(offset, limit, statusCode)
	if err != nil {
		glog.Error("Fail to get task list in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get task list")
	}
	req.Page.Total = int32(num)
	return &apb.ListTasksResponse{
		Page:  req.Page,
		Tasks: taskList,
	}, nil
}

func (api *ApiServer) ListClassifyTasks(ctx context.Context, req *apb.ListClassifyTasksRequest) (*apb.ListClassifyTasksResponse, error) {
	if req == nil || req.Page == nil || req.SearchOption == nil {
		glog.Error("The request for getting task list is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting task list is invalid!")
	}
	offset := int((req.Page.PageIndex - 1) * req.Page.PageSize)
	limit := int(req.Page.PageSize)
	statusCode := int(req.SearchOption.StatusCode)
	taskList, num, err := api.MysqlClient.GetTasks(offset, limit, statusCode)
	if err != nil {
		glog.Error("Fail to get task list in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get task list")
	}
	var tasks []*spb.Task
	for _, task := range taskList {
		taskResult, err := api.MysqlClient.GetClassifyTaskResult(task.Id)
		if err != nil {
			glog.Errorf("Fail to get task result in the database\n%v", err)
			return nil, status.Errorf(codes.Internal, "Fail to get task result list")
		}
		var labels []string
		if taskResult != nil {
			for _, labelConf := range taskResult.LabelConfs {
				labels = append(labels, labelConf.Label)
			}
		}
		task.Labels = labels
		tasks = append(tasks, task)
	}
	req.Page.Total = int32(num)
	return &apb.ListClassifyTasksResponse{
		Page:  req.Page,
		Tasks: tasks,
	}, nil
}

func (api *ApiServer) AddModel(ctx context.Context, req *apb.AddModelRequest) (
	*apb.AddModelResponse, error) {
	if req == nil || req.Model == nil || req.ModelTypes == nil || len(req.ModelTypes) == 0 {
		glog.Error("The request for adding model is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for adding model is invalid!")
	}
	model, _ := api.MysqlClient.GetModelByName(req.Model.Name)
	if model != nil {
		glog.Error("Duplicate entry")
		return nil, status.Errorf(codes.InvalidArgument,
			"The model named "+req.Model.Name+" is exists")
	}
	modelId := id.GenerateId()
	req.Model.Id = modelId
	tx, err := api.MysqlClient.Transaction()
	if err != nil {
		glog.Errorf("Fail to get Tx with the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to create model")
	}
	var tags []string
	for _, modelType := range req.ModelTypes {
		if modelType.Id == "" {
			var modelIds []string
			modelGroup, _ := api.MysqlClient.GetModelGroupByName(modelType.Name)
			if modelGroup != nil {
				glog.Errorf("The model group named %v is exists", modelType.Name)
				tx.Rollback()
				return nil, status.Errorf(codes.InvalidArgument,
					"The model group named "+modelType.Name+" is exists")
			}
			modelIds = append(modelIds, modelId)
			modelTypeId := id.GenerateId()
			modelType := &spb.ModelGroup{
				Id:        modelTypeId,
				TopicName: modelType.Name,
				ModelIds:  modelIds,
			}
			time := time.Now().Unix()
			err = api.MysqlClient.InsertModelGroup(modelType, time, tx)
			if err != nil {
				glog.Errorf("Fail to insert model group in the database \n%v", err)
				tx.Rollback()
				return nil, status.Errorf(codes.Internal, "Fail to add model")
			}
			tags = append(tags, modelType.TopicName)
		} else {
			modelGroup, err := api.MysqlClient.GetModelGroup(modelType.Id)
			modelIds := append(modelGroup.ModelIds, modelId)
			newModelGroup := &spb.ModelGroup{
				Id:        modelGroup.Id,
				TopicName: modelGroup.TopicName,
				ModelIds:  modelIds,
			}
			time := time.Now().Unix()
			err = api.MysqlClient.UpdateModelGroup(newModelGroup, time, tx)
			if err != nil {
				glog.Errorf("Fail to update model group in the database \n%v", err)
				tx.Rollback()
				return nil, status.Errorf(codes.Internal, "Fail to add model")
			}
			tags = append(tags, modelGroup.TopicName)
		}
	}
	req.Model.Tags = tags
	time := time.Now().Unix()
	err = api.MysqlClient.InsertModel(req.Model, time, tx)
	if err != nil {
		glog.Errorf("Fail to insert model in the database \n%v", err)
		tx.Rollback()
		return nil, status.Errorf(codes.Internal, "Fail to add model")
	}
	err = tx.Commit()
	if err != nil {
		glog.Errorf("Fail to commit tx \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to add model")
	}
	return &apb.AddModelResponse{
		ModelId: modelId,
	}, nil
}

func (api *ApiServer) ListModels(ctx context.Context, req *apb.ListModelsRequest) (
	*apb.ListModelsResponse, error) {
	models, err := api.MysqlClient.GetModels()
	if err != nil {
		glog.Errorf("Fail to read model list in the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to read model list in the database")
	}
	return &apb.ListModelsResponse{
		Models: models,
	}, nil
}

func (api *ApiServer) ListModelGroups(ctx context.Context, req *apb.ListModelGroupsRequest) (
	*apb.ListModelGroupsResponse, error) {
	modelGroups, err := api.MysqlClient.GetModelGroups()
	if err != nil {
		glog.Errorf("Fail to read model type list in the database \n%v", err)
		return nil, status.Errorf(codes.Internal,
			"Fail to read model type list in the database")
	}
	return &apb.ListModelGroupsResponse{
		ModelTypes: modelGroups,
	}, nil
}

func (api *ApiServer) UpdateModelTypes(ctx context.Context, req *apb.UpdateModelTypesRequest) (
	*apb.UpdateModelTypesResponse, error) {
	if req == nil || req.ModelTypes == nil || len(req.ModelTypes) == 0 || req.ModelId == "" {
		glog.Error("The request for updating model type is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for updating model type is invalid!")
	}
	model, err := api.MysqlClient.GetModel(req.ModelId)
	if err != nil {
		glog.Error("Fail to get model from database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to update model type")
	}
	tx, err := api.MysqlClient.Transaction()
	if err != nil {
		glog.Error("Fail to get Tx from database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to update model type")
	}
	var tags []string
	for _, modelGroup := range req.ModelTypes {
		tags = append(tags, modelGroup.TopicName)
		time := time.Now().Unix()
		err = api.MysqlClient.UpdateModelGroup(modelGroup, time, tx)
		if err != nil {
			glog.Error("Fail to update model group to the database \n%v", err)
			return nil, status.Errorf(codes.Internal, "Fail to update model type")
		}
	}
	model.Tags = tags
	time := time.Now().Unix()
	err = api.MysqlClient.UpdateModel(model, time, tx)
	if err != nil {
		glog.Error("Fail to update model to the database \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to update model type")
	}
	err = tx.Commit()
	if err != nil {
		glog.Error("Fail to commit tx \n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to update model type")
	}
	return &apb.UpdateModelTypesResponse{}, nil
}

func (api *ApiServer) GetModel(ctx context.Context, req *apb.GetModelRequest) (
	*apb.GetModelResponse, error) {
	if req == nil || req.ModelId == "" {
		glog.Error("The request for getting model info is invalid!")
		return nil, status.Errorf(codes.InvalidArgument, "The request for getting model info is invalid!")
	}
	model, err := api.MysqlClient.GetModel(req.ModelId)
	if err != nil {
		glog.Errorf("Fail to get model info in the database\n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to get model info")
	}
	return &apb.GetModelResponse{
		Model: model,
	}, nil
}

func (api *ApiServer) StartAllModelGroups(ctx context.Context, req *apb.StartAllModelGroupsRequest) (
	*apb.StartAllModelGroupsResponse, error) {
	modelGroups, err := api.MysqlClient.GetModelGroups()
	if err != nil {
		glog.Errorf("Fail to get all model from database\n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to start all model groups")
	}
	models, err := api.MysqlClient.GetModels()
	if err != nil {
		glog.Errorf("Fail to get all model from database\n%v", err)
		return nil, status.Errorf(codes.Internal, "Fail to start all models")
	}
	glog.Info(modelGroups, models)
	return &apb.StartAllModelGroupsResponse{}, nil
}
