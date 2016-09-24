#ifndef _SAMPLER_H
#define _SAMPLER_H

#include <math.h>
#include <functional>

class Sampler {
 public:
  Sampler() : num_intervals_(0), samples_(nullptr) { }

  ~Sampler() {
    if (samples_ != nullptr)
      delete[] samples_;
  }

  // Prevent copying and moving.
  Sampler(Sampler&) = delete;
  Sampler(Sampler&&) = delete;

  void Sample(int num_samples, std::function<float(float)> fn) {
    if (num_samples < 1)
      return;
    if (samples_ != nullptr)
      delete[] samples_;
    samples_ = new float[num_samples];
    num_intervals_ = num_samples - 1;
    float dx = (num_intervals_ > 0) ? 1.0f/num_intervals_ : 0.0f;
    for (int i = 0; i < num_samples; i++)
      samples_[i] = fn(static_cast<float>(i)*dx);
  }

  float Evaluate(float x) {
    if (x < 0.0f || x > 1.0f)
      return 0.0f;
    if (num_intervals_ < 1)
      return (samples_ != nullptr) ? samples_[0] : 0.0f;
    float fp_idx = floor(x*num_intervals_);
    int idx = static_cast<int>(fp_idx);
    return (samples_[idx]*(static_cast<float>(idx) + 1.0f - fp_idx) +
            samples_[idx + 1]*(fp_idx - static_cast<float>(idx)));
  }

  float operator()(float x) { return Evaluate(x); }

 private:
  int num_intervals_;
  float* samples_;
};

#endif  /* _SAMPLER_H */
