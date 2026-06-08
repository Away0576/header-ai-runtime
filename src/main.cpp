#include "MetaConfig.h"

#include <exception>
#include <iostream>
#include <string>

namespace {

void printUsage()
{
    std::cout << "header_ai_detector v0.4.0\n"
              << "Usage:\n"
              << "  header_ai_detector --version\n"
              << "  header_ai_detector --meta <meta.json>\n";
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

        printUsage();
        return 2;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
