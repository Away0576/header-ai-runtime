#include "Normalizer.h"

#include <stdexcept>
#include <utility>

namespace header_ai {

Normalizer::Normalizer(std::vector<double> mean, std::vector<double> std)
    : mean_(std::move(mean)), std_(std::move(std))
{
    if (mean_.empty() || mean_.size() != std_.size()) {
        throw std::runtime_error("Normalizer requires mean/std with the same non-zero length");
    }
    for (double std_value : std_) {
        if (std_value == 0.0) {
            throw std::runtime_error("Normalizer std must not contain 0");
        }
    }
}

std::vector<double> Normalizer::normalizeFlattenedTimeMajor(const std::vector<double>& values,
                                                           std::size_t feature_dim) const
{
    if (feature_dim == 0) {
        throw std::runtime_error("feature_dim must be greater than 0");
    }
    if (feature_dim != mean_.size()) {
        throw std::runtime_error("feature_dim must match normalizer mean/std length");
    }
    if (values.size() % feature_dim != 0) {
        throw std::runtime_error("flattened input length must be divisible by feature_dim");
    }

    std::vector<double> normalized;
    normalized.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        const std::size_t feature_index = i % feature_dim;
        normalized.push_back((values[i] - mean_[feature_index]) / std_[feature_index]);
    }
    return normalized;
}

}  // namespace header_ai
