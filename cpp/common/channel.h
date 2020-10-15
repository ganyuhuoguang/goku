#ifndef COMMON_CHANNEL_H_
#define COMMON_CHANNEL_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <chrono>

namespace novumind {
namespace common {

template <typename T>
class Channel {
 public:
  // Construct a channel with given capacity.
  explicit Channel(int capacity) {
    capacity_ = capacity;
    closed_ = false;
  }

  void Put(const T& item) {
    // TODO(jxluo): CHECK closed_.
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.size() >= capacity_) {
      // The channel is full, wait for available slot.
      available_slot_cv_.wait(lock);
    }
  
    queue_.push(item);
    // Notify avaiable item.
    available_item_cv_.notify_one();
    // release the lock.
  }

  bool PutWithTimeout(int timeout, const T& item) {
    // TODO(jxluo): CHECK closed_.
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= capacity_) {
      // The channel is full, wait for available item.
      available_slot_cv_.wait_for(
          lock, std::chrono::milliseconds(timeout),
          [this]{return queue_.size() < capacity_;});
    }
  
    if (queue_.size() < capacity_) {
      queue_.push(item);
      // Notify avaiable item.
      available_item_cv_.notify_one();
      return true;
    }
    return false;
    // release the lock.
  }

  bool Get(T* item) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty() && !closed_) {
      // The channel is empty, wait for available item.
      available_item_cv_.wait(lock);
    }
    if (!queue_.empty()) {
      *item = queue_.front();
      queue_.pop();
      // Notify avaiable item.
      available_slot_cv_.notify_one();
      // release the lock.
      return true;
    }
    return false;
  }

  bool GetWithTimeout(int timeout, bool* got_item, T* item) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty() && !closed_) {
      // The channel is empty, wait for available item.
      available_item_cv_.wait_for(
          lock, std::chrono::milliseconds(timeout),
          [this]{return !queue_.empty();});
    }
  
    if (!queue_.empty()) {
      *got_item = true;
      *item = queue_.front();
      queue_.pop();
      // Notify avaiable item.
      available_slot_cv_.notify_one();
      return true;
    }
    *got_item = false;
    return !closed_;
    // release the lock.
  }

  // Close this channel, no more element will be added into channel.
  void Close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    available_item_cv_.notify_all();
  }

 private:
  bool closed_;
  int capacity_;
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable available_slot_cv_;
  std::condition_variable available_item_cv_;
};

}  // namespace common 
}  // namespace novumind
#endif  // COMMON_CHANNEL_H_
