#pragma once

#include "MetaConfig.h"

#include <memory>
#include <string>
#include <vector>

namespace header_ai {

class OnnxAutoEncoder {
public:
    OnnxAutoEncoder(const std::string& model_path, const MetaConfig& config);
    ~OnnxAutoEncoder();

    OnnxAutoEncoder(const OnnxAutoEncoder&) = delete;
    OnnxAutoEncoder& operator=(const OnnxAutoEncoder&) = delete;
    OnnxAutoEncoder(OnnxAutoEncoder&&) noexcept;
    OnnxAutoEncoder& operator=(OnnxAutoEncoder&&) noexcept;

    std::vector<double> reconstruct(const std::vector<double>& normalized_input);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace header_ai
