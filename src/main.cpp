#include "AnomalyDetector.h"
#include "FileDataSource.h"
#include "MetaConfig.h"
#include "Normalizer.h"
#include "ReconstructionError.h"
#include "SlidingWindow.h"

#ifdef HEADER_AI_ENABLE_ONNX
#include "OnnxAutoEncoder.h"
#endif

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage()
{
    std::cout << "header_ai_detector v0.10.0\n"
              << "Usage:\n"
              << "  header_ai_detector --version\n"
              << "  header_ai_detector --meta <meta.json>\n"
              << "  header_ai_detector --model <model.onnx> --meta <meta.json> --probe-onnx\n"
              << "  header_ai_detector --model <model.onnx> --meta <meta.json> --replay <samples.csv>\n";
}

#ifdef HEADER_AI_ENABLE_ONNX
int runOnnxProbe(const std::string& model_path, const std::string& meta_path)
{
    const auto config = header_ai::loadMetaConfigFromFile(meta_path);
    header_ai::OnnxAutoEncoder model(model_path, config);
    const std::vector<double> normalized_input(config.input_dim, 0.0);
    const auto reconstruction = model.reconstruct(normalized_input);
    const double mse = header_ai::calculateMeanSquaredError(normalized_input, reconstruction);
    const bool anomaly = header_ai::isAnomaly(mse, config.threshold);
    header_ai::AnomalyDetector detector(config.alarm.consecutive_count, config.alarm.clear_count);
    const auto state = detector.update(anomaly);

    std::cout << "ONNX probe completed\n"
              << "input_dim=" << config.input_dim << "\n"
              << "output_dim=" << reconstruction.size() << "\n"
              << "mse=" << mse << "\n"
              << "threshold=" << config.threshold << "\n"
              << "is_anomaly=" << (anomaly ? "true" : "false") << "\n"
              << "state=" << header_ai::toString(state) << "\n";
    return reconstruction.size() == config.input_dim ? 0 : 1;
}

int runReplay(const std::string& model_path, const std::string& meta_path, const std::string& replay_path)
{
    const auto config = header_ai::loadMetaConfigFromFile(meta_path);
    header_ai::OnnxAutoEncoder model(model_path, config);
    header_ai::FileDataSource source(replay_path, config.feature_dim);
    header_ai::SlidingWindow window(config.window_size, config.feature_dim);
    header_ai::Normalizer normalizer(config.normalization.mean, config.normalization.std);
    header_ai::AnomalyDetector detector(config.alarm.consecutive_count, config.alarm.clear_count);

    std::vector<double> sample;
    std::size_t sample_index = 0;
    std::size_t window_index = 0;

    std::cout << "window_index,end_sample_index,mse,threshold,is_anomaly,state\n";
    while (source.readSample(sample)) {
        window.addSample(sample);
        if (window.ready()) {
            const auto flattened = window.flattenTimeMajor();
            const auto normalized_input = normalizer.normalizeFlattenedTimeMajor(flattened, config.feature_dim);
            const auto reconstruction = model.reconstruct(normalized_input);
            const double mse = header_ai::calculateMeanSquaredError(normalized_input, reconstruction);
            const bool anomaly = header_ai::isAnomaly(mse, config.threshold);
            const auto state = detector.update(anomaly);

            std::cout << window_index << ','
                      << sample_index << ','
                      << std::setprecision(12) << mse << ','
                      << std::setprecision(12) << config.threshold << ','
                      << (anomaly ? "true" : "false") << ','
                      << header_ai::toString(state) << "\n";
            ++window_index;
        }
        ++sample_index;
    }

    std::cout << "total_samples=" << sample_index << "\n"
              << "total_windows=" << window_index << "\n";
    return window_index > 0 ? 0 : 1;
}
#endif

}  // namespace

int main(int argc, char* argv[])
{
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--version")) {
            printUsage();
            return 0;
        }

        if (argc == 3 && std::string(argv[1]) == "--meta") {
            const auto config = header_ai::loadMetaConfigFromFile(argv[2]);
            std::cout << "Loaded meta.json\n"
                      << "schema_version=" << config.schema_version << "\n"
                      << "model_type=" << config.model_type << "\n"
                      << "window_size=" << config.window_size << "\n"
                      << "feature_dim=" << config.feature_dim << "\n"
                      << "input_dim=" << config.input_dim << "\n"
                      << "threshold=" << config.threshold << "\n"
                      << "consecutive_count=" << config.alarm.consecutive_count << "\n"
                      << "clear_count=" << config.alarm.clear_count << "\n";
            return 0;
        }

        if (argc == 6 && std::string(argv[1]) == "--model" && std::string(argv[3]) == "--meta" && std::string(argv[5]) == "--probe-onnx") {
#ifdef HEADER_AI_ENABLE_ONNX
            return runOnnxProbe(argv[2], argv[4]);
#else
            std::cerr << "ONNX Runtime support is not enabled in this build\n";
            return 1;
#endif
        }

        if (argc == 7 && std::string(argv[1]) == "--model" && std::string(argv[3]) == "--meta" && std::string(argv[5]) == "--replay") {
#ifdef HEADER_AI_ENABLE_ONNX
            return runReplay(argv[2], argv[4], argv[6]);
#else
            std::cerr << "ONNX Runtime support is not enabled in this build\n";
            return 1;
#endif
        }

        printUsage();
        return 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
