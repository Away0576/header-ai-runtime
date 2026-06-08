#pragma once

#include <cstddef>
#include <deque>
#include <vector>

namespace header_ai {

class SlidingWindow {
public:
    SlidingWindow(std::size_t window_size, std::size_t feature_dim);

    void addSample(const std::vector<double>& sample);
    bool ready() const;
    std::vector<double> flattenTimeMajor() const;
    std::size_t size() const;
    std::size_t windowSize() const;
    std::size_t featureDim() const;

private:
    std::size_t window_size_;
    std::size_t feature_dim_;
    std::deque<std::vector<double>> samples_;
};

}  // namespace header_ai
