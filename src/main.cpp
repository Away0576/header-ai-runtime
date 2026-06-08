#include "MetaConfig.h"

#ifdef HEADER_AI_ENABLE_ONNX
#include "OnnxAutoEncoder.h"
#endif

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

void printUsage()
{
    std::cout << "header_ai_detector v0.5.0\n"
              << "Usage:\n"
              << "  header_ai_detector --version\n"
              << "  header_ai_detector --meta <meta.json>\n"
              << "  header_ai_detector --model <model.onnx> --meta <meta.json> --probe-onnx\n";
}

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
                      << "threshold=" << config.threshold << "\n";
            return 0;
        }

        if (argc == 6 && std::string(argv[1]) == "--model" && std::string(argv[3]) == "--meta" && std::string(argv[5]) == "--probe-onnx") {
#ifdef HEADER_AI_ENABLE_ONNX
            const auto config = header_ai::loadMetaConfigFromFile(argv[4]);
            header_ai::OnnxAutoEncoder model(argv[2], config);
            const std::vector<double> zero_input(config.input_dim, 0.0);
            const auto reconstruction = model.reconstruct(zero_input);
            std::cout << "ONNX probe completed\n"
                      << "input_dim=" << config.input_dim << "\n"
                      << "output_dim=" << reconstruction.size() << "\n";
            return reconstruction.size() == config.input_dim ? 0 : 1;
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
