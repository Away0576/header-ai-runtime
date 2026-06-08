#include "ReconstructionError.h"

#include <cstddef>
#include <stdexcept>

namespace header_ai {

double calculateMeanSquaredError(const std::vector<double>& expected, const std::vector<double>& actual)
{
    if (expected.empty()) {
        throw std::runtime_error("MSE input must not be empty");
    }
    if (expected.size() != actual.size()) {
        throw std::runtime_error("MSE input vectors must have the same length");
    }

    double squared_error_sum = 0.0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const double error = actual[i] - expected[i];
        squared_error_sum += error * error;
    }
    return squared_error_sum / static_cast<double>(expected.size());
}

bool isAnomaly(double mse, double threshold)
{
    if (threshold < 0.0) {
        throw std::runtime_error("threshold must be greater than or equal to 0");
    }
    return mse > threshold;
}

}  // namespace header_ai
