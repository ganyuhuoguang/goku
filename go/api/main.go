package main

import (
	"flag"
	"github.com/golang/glog"
	"github.com/grpc-ecosystem/grpc-gateway/runtime"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
	"net"
	"net/http"
	"novumind/goku/go/common/db"
	"novumind/goku/go/common/kafka"
	apb "novumind/goku/proto/go/api"
	"time"
)

var (
	httpPort   = flag.String("http_port", "20000", "The port http service listen to.")
	grpcPort   = flag.String("grpc_port", "20001", "The port grpc service listen to.")
	pathPrefix = flag.String("path_prefix", "", "The path prefix, for reverse proxy.")
	brokerAddr = flag.String("borker_addr", "localhost:9092",
		"The address kafka service listen to.")
	mysqlAddr = flag.String("mysql_addr", "localhost:3306",
		"The address mysql service listen to.")
)

func main() {
	flag.Parse()

	// Start http service
	go func() {
		startHttpServer()
	}()

	// Start grpc service
	startGrpcServer()
}

func startHttpServer() {
	gwmux := runtime.NewServeMux(runtime.WithMarshalerOption(
		runtime.MIMEWildcard, &runtime.JSONPb{
			OrigName:     false,
			EmitDefaults: true,
		}))
	opts := []grpc.DialOption{grpc.WithInsecure()}
	err := apb.RegisterApiHandlerFromEndpoint(
		context.Background(), gwmux, "localhost:"+*grpcPort, opts)
	if err != nil {
		glog.Fatal("Fail to registry http service: %v", err)
	}
	glog.Info("Start http service on port %s ...", *httpPort)
	mux := http.NewServeMux()
	mux.Handle("/", gwmux)
	httpServer := &http.Server{
		Addr:           ":" + *httpPort,
		Handler:        http.StripPrefix(*pathPrefix, mux),
		ReadTimeout:    10 * time.Second,
		WriteTimeout:   10 * time.Second,
		MaxHeaderBytes: 1 << 20,
	}
	glog.Fatal(httpServer.ListenAndServe())
}

func startGrpcServer() {
	lis, err := net.Listen("tcp", ":"+*grpcPort)
	if err != nil {
		glog.Fatalf("Failed to listen on grpc port: %v", err)
	}
	var opts []grpc.ServerOption
	s := grpc.NewServer(opts...)
	mysqlClient, err := db.NewMysqlClient(*mysqlAddr)
	if err != nil {
		glog.Fatal("Error to init the mysql database", err)
	}
	defer mysqlClient.Close()
	producer, err := kafka.NewProducer(*brokerAddr, false)
	if err != nil {
		glog.Fatal("Error to init the kafka producer", err)
	}
	defer producer.Close()
	serverInstance := &ApiServer{mysqlClient, producer}
	apb.RegisterApiServer(s, serverInstance)
	reflection.Register(s)
	if err := s.Serve(lis); err != nil {
		glog.Fatalf("Failed to serve grpc: %v", err)
	}
}
