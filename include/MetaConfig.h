#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace header_ai {

struct NormalizationConfig {
    std::string type;
    std::vector<double> mean;
    std::vector<double> std;
};

struct AlarmConfig {
    std::size_t consecutive_count{};
    std::size_t clear_count{};
};

struct OnnxConfig {
    int opset{};
};

struct MetaConfig {
    std::string schema_version;
    std::string model_type;
    std::string input_name;
    std::string output_name;
    std::size_t window_size{};
    std::size_t feature_dim{};
    std::size_t input_dim{};
    std::string flatten_order;
    double threshold{};
    double threshold_percentile{};
    NormalizationConfig normalization;
    AlarmConfig alarm;
    OnnxConfig onnx;
};

MetaConfig loadMetaConfigFromFile(const std::string& path);
MetaConfig parseMetaConfig(const std::string& json_text);
void validateMetaConfig(const MetaConfig& config);

}  // namespace header_ai
