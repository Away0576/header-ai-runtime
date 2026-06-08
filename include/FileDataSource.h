#pragma once

#include "IDataSource.h"

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

namespace header_ai {

class FileDataSource : public IDataSource {
public:
    FileDataSource(const std::string& path, std::size_t feature_dim);

    bool readSample(std::vector<double>& sample) override;
    std::size_t lineNumber() const;

private:
    std::vector<double> parseLine(const std::string& line) const;

    std::ifstream input_;
    std::string path_;
    std::size_t feature_dim_;
    std::size_t line_number_{};
    bool skipped_header_{};
    bool read_sample_{};
};

}  // namespace header_ai
