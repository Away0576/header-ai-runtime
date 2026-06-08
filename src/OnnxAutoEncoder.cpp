#include "OnnxAutoEncoder.h"

#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

namespace header_ai {
namespace {

std::string getInputName(Ort::Session& session, Ort::AllocatorWithDefaultOptions& allocator)
{
    if (session.GetInputCount() != 1) {
        throw std::runtime_error("ONNX model must have exactly one input");
    }
    auto name = session.GetInputNameAllocated(0, allocator);
    return name.get();
}

std::string getOutputName(Ort::Session& session, Ort::AllocatorWithDefaultOptions& allocator)
{
    if (session.GetOutputCount() != 1) {
        throw std::runtime_error("ONNX model must have exactly one output");
    }
    auto name = session.GetOutputNameAllocated(0, allocator);
    return name.get();
}

void validateTensorShape(const std::vector<int64_t>& shape, std::size_t input_dim, const std::string& label)
{
    if (shape.size() != 2) {
        throw std::runtime_error("ONNX " + label + " shape must be [1, input_dim]");
    }
    if (shape[1] != static_cast<int64_t>(input_dim)) {
        throw std::runtime_error("ONNX " + label + " second dimension must equal input_dim");
    }
}

void validateNumericTensorType(ONNXTensorElementDataType type, const std::string& label)
{
    if (type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT && type != ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
        std::ostringstream message;
    message << "ONNX " << label << " tensor type must be float32 or float64, actual type=" << static_cast<int>(type);
    throw std::runtime_error(message.str());
    }
}

}  // namespace

class OnnxAutoEncoder::Impl {
public:
    Impl(const std::string& model_path, const MetaConfig& config)
        : env_(ORT_LOGGING_LEVEL_WARNING, "header_ai_runtime"), session_options_{}, session_(nullptr), input_dim_(config.input_dim)
    {
        session_options_.SetIntraOpNumThreads(1);
        session_options_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
        session_ = Ort::Session(env_, model_path.c_str(), session_options_);

        Ort::AllocatorWithDefaultOptions allocator;
        input_name_ = getInputName(session_, allocator);
        output_name_ = getOutputName(session_, allocator);
        if (input_name_ != config.input_name) {
            throw std::runtime_error("ONNX input name mismatch: model has '" + input_name_ + "', meta.json expects '" + config.input_name + "'");
        }
        if (output_name_ != config.output_name) {
            throw std::runtime_error("ONNX output name mismatch: model has '" + output_name_ + "', meta.json expects '" + config.output_name + "'");
        }

        const auto input_type_info = session_.GetInputTypeInfo(0);
        const auto input_info = input_type_info.GetTensorTypeAndShapeInfo();
        input_type_ = input_info.GetElementType();
        validateNumericTensorType(input_type_, "input");
        validateTensorShape(input_info.GetShape(), input_dim_, "input");

        const auto output_type_info = session_.GetOutputTypeInfo(0);
        const auto output_info = output_type_info.GetTensorTypeAndShapeInfo();
        output_type_ = output_info.GetElementType();
        validateNumericTensorType(output_type_, "output");
        validateTensorShape(output_info.GetShape(), input_dim_, "output");
    }

    std::vector<double> reconstruct(const std::vector<double>& normalized_input)
    {
        if (normalized_input.size() != input_dim_) {
            throw std::runtime_error("ONNX input vector length must equal input_dim");
        }

        if (input_type_ == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
            auto double_input = normalized_input;
            return run<double>(double_input);
        }

        std::vector<float> float_input;
        float_input.reserve(normalized_input.size());
        std::transform(normalized_input.begin(), normalized_input.end(), std::back_inserter(float_input), [](double value) {
            return static_cast<float>(value);
        });
        return run<float>(float_input);
    }

private:
    template <typename InputT>
    std::vector<double> run(std::vector<InputT>& input)
    {
        std::array<int64_t, 2> shape{1, static_cast<int64_t>(input_dim_)};
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        auto input_tensor = Ort::Value::CreateTensor<InputT>(memory_info, input.data(), input.size(), shape.data(), shape.size());

        const char* input_names[] = {input_name_.c_str()};
        const char* output_names[] = {output_name_.c_str()};
        auto outputs = session_.Run(Ort::RunOptions{nullptr}, input_names, &input_tensor, 1, output_names, 1);
        if (outputs.size() != 1 || !outputs[0].IsTensor()) {
            throw std::runtime_error("ONNX inference did not return exactly one tensor output");
        }

        const auto output_info = outputs[0].GetTensorTypeAndShapeInfo();
        if (static_cast<std::size_t>(output_info.GetElementCount()) != input_dim_) {
            throw std::runtime_error("ONNX output length must equal input_dim");
        }

        if (output_type_ == ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE) {
            const double* output_data = outputs[0].template GetTensorData<double>();
            return std::vector<double>(output_data, output_data + input_dim_);
        }

        const float* output_data = outputs[0].template GetTensorData<float>();
        std::vector<double> reconstruction;
        reconstruction.reserve(input_dim_);
        for (std::size_t i = 0; i < input_dim_; ++i) {
            reconstruction.push_back(static_cast<double>(output_data[i]));
        }
        return reconstruction;
    }

    Ort::Env env_;
    Ort::SessionOptions session_options_;
    Ort::Session session_;
    std::string input_name_;
    std::string output_name_;
    std::size_t input_dim_;
    ONNXTensorElementDataType input_type_{};
    ONNXTensorElementDataType output_type_{};
};

OnnxAutoEncoder::OnnxAutoEncoder(const std::string& model_path, const MetaConfig& config)
    : impl_(std::make_unique<Impl>(model_path, config))
{
}

OnnxAutoEncoder::~OnnxAutoEncoder() = default;
OnnxAutoEncoder::OnnxAutoEncoder(OnnxAutoEncoder&&) noexcept = default;
OnnxAutoEncoder& OnnxAutoEncoder::operator=(OnnxAutoEncoder&&) noexcept = default;

std::vector<double> OnnxAutoEncoder::reconstruct(const std::vector<double>& normalized_input)
{
    return impl_->reconstruct(normalized_input);
}

}  // namespace header_ai
