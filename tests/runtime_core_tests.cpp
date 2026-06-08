#include "MetaConfig.h"
#include "Normalizer.h"
#include "ReconstructionError.h"
#include "SlidingWindow.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const char* kValidMeta = R"json(
{
  "schema_version": "1.0",
  "model_type": "autoencoder",
  "input_name": "input",
  "output_name": "reconstruction",
  "window_size": 60,
  "feature_dim": 1,
  "input_dim": 60,
  "flatten_order": "time_major",
  "threshold": 0.009797872975468636,
  "threshold_percentile": 99.9,
  "normalization": {
    "type": "standard",
    "mean": [43.097919775672935],
    "std": [28.07703083103901]
  },
  "alarm": {
    "consecutive_count": 3,
    "clear_count": 5
  },
  "onnx": {
    "opset": 17
  }
}
)json";

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

bool nearlyEqual(double left, double right)
{
    return std::fabs(left - right) < 1e-12;
}

void expectInvalid(const std::string& json, const std::string& label)
{
    try {
        (void)header_ai::parseMetaConfig(json);
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error("Expected invalid meta.json for " + label);
}

void expectRuntimeError(void (*operation)(), const std::string& label)
{
    try {
        operation();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error("Expected runtime error for " + label);
}

std::string replaceFirst(std::string value, const std::string& from, const std::string& to)
{
    const auto pos = value.find(from);
    if (pos == std::string::npos) {
        throw std::runtime_error("test replacement source not found");
    }
    value.replace(pos, from.size(), to);
    return value;
}

void testMetaConfig()
{
    const auto config = header_ai::parseMetaConfig(kValidMeta);
    require(config.schema_version == "1.0", "schema_version parsed");
    require(config.model_type == "autoencoder", "model_type parsed");
    require(config.input_name == "input", "input_name parsed");
    require(config.output_name == "reconstruction", "output_name parsed");
    require(config.window_size == 60, "window_size parsed");
    require(config.feature_dim == 1, "feature_dim parsed");
    require(config.input_dim == 60, "input_dim parsed");
    require(config.flatten_order == "time_major", "flatten_order parsed");
    require(nearlyEqual(config.threshold, 0.009797872975468636), "threshold parsed");
    require(config.normalization.mean.size() == 1, "mean length");
    require(config.normalization.std.size() == 1, "std length");
    require(config.alarm.consecutive_count == 3, "consecutive_count parsed");
    require(config.alarm.clear_count == 5, "clear_count parsed");
    require(config.onnx.opset == 17, "opset parsed");

    expectInvalid(replaceFirst(kValidMeta, R"("schema_version": "1.0",)", ""), "missing schema_version");
    expectInvalid(replaceFirst(kValidMeta, R"("std": [28.07703083103901])", R"("std": [0])"), "std zero");
    expectInvalid(replaceFirst(kValidMeta, R"("input_dim": 60)", R"("input_dim": 61)"), "input_dim mismatch");
}

void testSlidingWindow()
{
    header_ai::SlidingWindow window(60, 1);
    std::size_t ready_count = 0;
    for (int value = 1; value <= 100; ++value) {
        window.addSample({static_cast<double>(value)});
        if (window.ready()) {
            ++ready_count;
            const auto flattened = window.flattenTimeMajor();
            require(flattened.size() == 60, "single-variable flattened length");
            require(nearlyEqual(flattened.front(), static_cast<double>(value - 59)), "window drops oldest sample");
            require(nearlyEqual(flattened.back(), static_cast<double>(value)), "window keeps newest sample");
        }
    }
    require(ready_count == 41, "100 samples with window 60 produce 41 ready windows");

    header_ai::SlidingWindow multivariate(3, 2);
    multivariate.addSample({1.0, 2.0});
    multivariate.addSample({3.0, 4.0});
    multivariate.addSample({5.0, 6.0});
    const auto flattened = multivariate.flattenTimeMajor();
    require((flattened == std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}), "time_major flatten order");
}

void testNormalizer()
{
    header_ai::Normalizer normalizer({10.0, 100.0}, {2.0, 10.0});
    const auto normalized = normalizer.normalizeFlattenedTimeMajor({12.0, 110.0, 8.0, 90.0}, 2);
    require((normalized == std::vector<double>{1.0, 1.0, -1.0, -1.0}), "time_major standard scaling");
}

void testReconstructionError()
{
    const auto mse = header_ai::calculateMeanSquaredError({1.0, 2.0, 3.0}, {1.0, 4.0, 5.0});
    require(nearlyEqual(mse, 8.0 / 3.0), "MSE calculation");
    require(!header_ai::isAnomaly(0.5, 0.5), "threshold uses strict greater-than");
    require(header_ai::isAnomaly(0.5001, 0.5), "score above threshold is anomaly");

    expectRuntimeError([]() { (void)header_ai::calculateMeanSquaredError({}, {}); }, "empty MSE input");
    expectRuntimeError([]() { (void)header_ai::calculateMeanSquaredError({1.0}, {1.0, 2.0}); }, "MSE size mismatch");
    expectRuntimeError([]() { (void)header_ai::isAnomaly(0.1, -1.0); }, "negative threshold");
}

}  // namespace

int main()
{
    try {
        testMetaConfig();
        testSlidingWindow();
        testNormalizer();
        testReconstructionError();
        std::cout << "runtime_core_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
