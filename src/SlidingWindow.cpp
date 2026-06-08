#include "SlidingWindow.h"

#include <stdexcept>

namespace header_ai {

SlidingWindow::SlidingWindow(std::size_t window_size, std::size_t feature_dim)
    : window_size_(window_size), feature_dim_(feature_dim)
{
    if (window_size_ == 0) {
        throw std::runtime_error("window_size must be greater than 0");
    }
    if (feature_dim_ == 0) {
        throw std::runtime_error("feature_dim must be greater than 0");
    }
}

void SlidingWindow::addSample(const std::vector<double>& sample)
{
    if (sample.size() != feature_dim_) {
        throw std::runtime_error("sample feature count must equal feature_dim");
    }
    samples_.push_back(sample);
    if (samples_.size() > window_size_) {
        samples_.pop_front();
    }
}

bool SlidingWindow::ready() const
{
    return samples_.size() == window_size_;
}

std::vector<double> SlidingWindow::flattenTimeMajor() const
{
    if (!ready()) {
        throw std::runtime_error("sliding window is not ready");
    }

    std::vector<double> flattened;
    flattened.reserve(window_size_ * feature_dim_);
    for (const auto& sample : samples_) {
        flattened.insert(flattened.end(), sample.begin(), sample.end());
    }
    return flattened;
}

std::size_t SlidingWindow::size() const
{
    return samples_.size();
}

std::size_t SlidingWindow::windowSize() const
{
    return window_size_;
}

std::size_t SlidingWindow::featureDim() const
{
    return feature_dim_;
}

}  // namespace header_ai
