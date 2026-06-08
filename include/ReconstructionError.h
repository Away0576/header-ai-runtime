#pragma once

#include <vector>

namespace header_ai {

double calculateMeanSquaredError(const std::vector<double>& expected, const std::vector<double>& actual);
bool isAnomaly(double mse, double threshold);

}  // namespace header_ai
