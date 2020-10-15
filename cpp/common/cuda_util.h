#ifndef COMMON_CUDA_UTIL_H
#define COMMON_CUDA_UTIL_H
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <cuda_runtime.h>
#include "cublas_v2.h"

#define CUDA_FATAL_ERROR(s) {                                               \
    std::stringstream _where, _message;                                \
    _where << __FILE__ << ':' << __LINE__;                             \
    _message << std::string(s) + "\n" << __FILE__ << ':' << __LINE__;  \
    std::cerr << _message.str() << "\nAborting...\n";                  \
    cudaDeviceReset();                                                 \
    exit(EXIT_FAILURE);                                                \
}


#define CHECK_CUDA_ERROR(status) {                                       \
    std::stringstream _error;                                            \
    if (status != 0) {                                                   \
        _error << "Cuda failure\nError: " << cudaGetErrorString(status); \
        CUDA_FATAL_ERROR(_error.str());                                       \
    }                                                                    \
}

#define CHECK_CUBLAS_ERROR(status) {                                   \
    std::stringstream _error;                                          \
    if (status != 0) {                                                 \
        _error << "Cublas failure\nError code " << status;             \
        CUDA_FATAL_ERROR(_error.str());                                     \
    }                                                                  \
}

#endif //COMMON_CUDA_UTIL_H
