#ifndef COMMON_TIME_H_
#define COMMON_TIME_H_

#include <time.h>
#include <stdint.h>

namespace novumind {
namespace common {

inline uint64_t GetTimeNano() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return (uint64_t)(t.tv_sec * 1000*1000*1000) + (uint64_t)(t.tv_nsec);
}

inline uint64_t GetTimeMilli() {
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  return (uint64_t)(t.tv_sec * 1000) + (uint64_t)(t.tv_nsec/1000/1000);
}

}  // namespace common
}  // namespace novumind
#endif  // COMMON_TIME_H_

