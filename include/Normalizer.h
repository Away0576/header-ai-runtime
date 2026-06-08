#pragma once

#include <cstddef>
#include <vector>

namespace header_ai {

class Normalizer {
public:
    Normalizer(std::vector<double> mean, std::vector<double> std);

    std::vector<double> normalizeFlattenedTimeMajor(const std::vector<double>& values,
                                                    std::size_t feature_dim) const;

private:
    std::vector<double> mean_;
    std::vector<double> std_;
};

}  // namespace header_ai
