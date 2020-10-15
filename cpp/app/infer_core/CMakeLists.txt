# check the cmake version
cmake_minimum_required(VERSION 3.5)
if (UNIX)
  add_definitions(-Wno-sign-compare -Wno-enum-compare)
endif(UNIX)
# Project name
project(InferCore)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${InferCore_SOURCE_DIR}/cmake_module ${CMAKE_MODULE_PATH})
list(APPEND CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS}")

option(CPU_ONLY "ON if use caffe, OFF if use tensor rt" ON)
message(${CPU_ONLY})
option(BUILD_WITH_CAFFE "ON if use caffe, OFF if use tensor rt" ON)  #OFF by xiaolong    ON
message(${BUILD_WITH_CAFFE})

option(BUILD_WITH_CUDA "OFF if use CUDA acceleration with caffe" OFF)
option(BUILD_WITH_NOVURT "ON if use novurt, and need to -DBUILD_WITH_CUDA=OFF" OFF)


set(CUDA_ROOT /usr/local/cuda-11.0)
set(REPO_ROOT ${InferCore_SOURCE_DIR}/../../..)
message(${REPO_ROOT})
set(CPP_ROOT ${REPO_ROOT}/cpp)
set(PROTO_GEN_PATH ${REPO_ROOT}/proto/cpp_gen)
set(CAFFE_ROOT /home/xiaolonglu/caffe)
set(NOVURT_ROOT /opt/sdk095/novutensor_sdk-0.9.5/build/distribute)  #${CPP_ROOT}/novu_rt  /opt/sdk095/novutensor_sdk-0.9.5/build/runtime
set(CAFFE_LIBRARIES "-L${CAFFE_ROOT}/build/lib -lcaffe")
set(NOVURT_LIBRARIES "-L${NOVURT_ROOT}/lib -lNovuRT")


#add CUDA lib
if(BUILD_WITH_CUDA)
  add_definitions(-DUSE_GPU)
  find_package(CUDA REQUIRED)
  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS}; -O3 -arch=sm_52)
  include_directories( ${CUDA_INCLUDE_DIRS} )
  link_libraries( ${CUDA_LIBRARIES})
  link_libraries( ${CUDA_CUBLAS_LIBRARIES})
  include_directories(${CUDA_ROOT}/include)
endif()

message("hello caffe")
message(${BUILD_WITH_NOVURT})
if(BUILD_WITH_CAFFE)
  if(BUILD_WITH_NOVURT)
    add_definitions(-DCPU_ONLY)
    add_definitions(-DUSE_NOVURT)
    include_directories(${CAFFE_ROOT}/include)
    include_directories(${CUDA_ROOT}/include)
    link_libraries(${CAFFE_LIBRARIES})
    link_libraries(${NOVURT_LIBRARIES})
  else()
    #add_definitions(-DCPU_ONLY)  #XLL
    add_definitions(-DUSE_CAFFE)
    include_directories(${CAFFE_ROOT}/include)
    link_libraries(${CAFFE_LIBRARIES})
  endif()
  #gflog
  link_libraries( glog )
  find_package(glog REQUIRED)
else()
  message("hello")
  add_definitions(-D_GLIBCXX_USE_C99)
  add_definitions(-DUSE_TENSORRT)
  set(TENSORRT_INCLUDE /mnt/data/TensorRT-7.1.3.4/include)
  include_directories(${TENSORRT_INCLUDE})
  set(CUDA_VERSION cuda-11.0)
  set(CUDA_INSTALL_DIR /usr/local/${CUDA_VERSION})
  include_directories(${CUDA_INSTALL_DIR}/include)
  include_directories(/usr/local/include)
  set(CUDNN_LIB_PATH ${CUDA_INSTALL_DIR}/lib64)
  set(TARGET_LIB_PATH ${CUDA_INSTALL_DIR}/targets/x86_64-linux/lib64)
  link_directories(${CUDNN_LIB_PATH})
  set(SYS_LIB /mnt/data/TensorRT-7.1.3.4/lib)  #/usr/local/lib
  link_directories(${SYS_LIB})
  set(TENSORRT_LIBRARIES "-lnvinfer -lnvparsers -lnvinfer_plugin")
  link_libraries(${TENSORRT_LIBRARIES})
  set(SYS_LIB /usr/local/cuda-11.0/targets/x86_64-linux/lib)  #
  link_directories(${SYS_LIB})
  link_libraries(cudnn)
  link_libraries(cublas)
  link_libraries(cudart_static)
  link_libraries(nvToolsExt)
  link_libraries(cudart)
  link_libraries(rt)
  link_libraries(dl)
endif()

find_package(Threads)
link_libraries(pthread)

#add include folder to the project
include_directories( ${PROTO_GEN_PATH})
include_directories( /opt/sdk095/novutensor_sdk-0.9.5/build/distribute/include)
include_directories( ${CPP_ROOT})

#add Protocol Buffers
find_library(PROTOBUF_LIBRARY protobuf HINTS /usr/local/lib)
link_libraries(${PROTOBUF_LIBRARY})

#add opencv lib
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )
link_libraries( ${OpenCV_LIBS})


link_libraries( ssl )
link_libraries( crypto )
link_libraries( boost_system ) 

#gflags
link_libraries( gflags )
find_package(gflags REQUIRED)


#include(findGRPC/FindGRPC.cmake)
set(_REFLECTION grpc++_reflection)
set(_GRPC_GRPCPP grpc++)
#grpc


find_package(gRPC CONFIG REQUIRED)
#link_libraries( gRPC::grpc )
#link_libraries( gRPC::grpc++ )
#link_libraries( gRPC::grpc++_reflection gRPC::grpc++ protobuf::libprotobuf)

message(STATUS "Using gRPC ${gRPC_VERSION}")

#`pkg-config --libs grpc++`
# Use reflection lib, see: https://grpc.io/grpc/cpp/md_doc_server_reflection_tutorial.html
set(CMAKE_EXE_LINKER_FLAGS "-Wl,--no-as-needed -lboost_thread -L../boost/stage/lib -pthread -lgrpc -lgrpc++ -lgrpc++_reflection -lgpr -Wl,--as-needed,-g")#,-lz   -lboost_system 
 
link_libraries( z )

# For convenience we define the sources as a variable. You can add 
# header files and cpp/c files and CMake will sort them out
file(GLOB_RECURSE SRCS
  ${PROTO_GEN_PATH}/*.cc 
  ${CPP_ROOT}/common/*.cpp 
  )

if(BUILD_WITH_CUDA)
cuda_add_library(InferCoreLib ${SRCS})
cuda_add_executable(InferCore src/main.cpp)
else()
add_library(InferCoreLib ${SRCS})
add_executable(InferCore src/main.cpp)
endif()


target_link_libraries(InferCore  InferCoreLib)




