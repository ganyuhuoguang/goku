#ifndef COMMON_MATH_UTIL_H_
#define COMMON_MATH_UTIL_H_

#include <cmath>
#include <vector>
#include <algorithm>
#include <utility>

using std::exp;

namespace novumind {
namespace common {

inline static bool PairCompare(const std::pair<float, int>& lhs,
    const std::pair<float, int>& rhs) {
  return lhs.first > rhs.first;
}

/* Return the indices of the top N values of vector v. */
inline static std::vector<int> Argmax(const std::vector<float>& v, int N) {
  std::vector<std::pair<float, int> > pairs;
  for (size_t i = 0; i < v.size(); ++i)
    pairs.push_back(std::make_pair(v[i], i));
  std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

  std::vector<int> result;
  for (int i = 0; i < N; ++i)
    result.push_back(pairs[i].second);
  return result;
}

inline float sigmoid(float p) {
  return 1.0 / (1.0 + exp(-p *1.0));
}

inline float overlap(float x1, float w1, float x2, float w2) {
  float left = std::max(x1 - w1 / 2.0, x2 - w2 / 2.0);
  float right = std::max(x1 + w1 / 2.0, x2 + w2 / 2.0);
  return right - left;
}


inline float softmax_sum(const std::vector<float>& vec, float max_elem) {
  float sum = 0.0;
  for (auto& elem : vec) {
    if (elem > 0) {
      sum += exp(elem - max_elem);
    }
  }
  return sum;
}

}  // namespace common
}  // namespace novumind
#endif  // COMMON_MATH_UTIL_H_
