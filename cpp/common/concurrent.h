#ifndef COMMON_CONCURRENT_H_
#define COMMON_CONCURRENT_H_

#include <fstream>
#include <string>
#include <stdlib.h>     /* srand, rand */
#include <time.h>
#include <memory>
#include <mutex>

namespace novumind {
namespace common {

// A thread safe var class.
template <typename T>
class MultiThreadVar {
 public:
  MultiThreadVar(T init) {
    std::unique_lock<std::mutex> mlock(mutex_);
    value_ = init;
  }

  T value() {
    std::unique_lock<std::mutex> mlock(mutex_);
    return value_;
  }

  void set_value(T value) {
    std::unique_lock<std::mutex> mlock(mutex_);
    value_ = value;
  }

 private:
  T value_;
  std::mutex mutex_;
};

using MultiThreadFlag = MultiThreadVar<bool>;
}  // namespace common
}  // namespace novumind
#endif  // COMMON_CONCURRENT_H_
