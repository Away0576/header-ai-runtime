#include "FileDataSource.h"

#include <cctype>
#include <stdexcept>

namespace header_ai {
namespace {

std::string trim(const std::string& value)
{
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

std::vector<std::string> splitFields(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    const bool csv = line.find(',') != std::string::npos;

    for (char ch : line) {
        if ((csv && ch == ',') || (!csv && std::isspace(static_cast<unsigned char>(ch)))) {
            if (!field.empty() || csv) {
                fields.push_back(trim(field));
                field.clear();
            }
        } else {
            field.push_back(ch);
        }
    }
    if (!field.empty() || csv) {
        fields.push_back(trim(field));
    }
    return fields;
}

bool parseDouble(const std::string& token, double& value)
{
    const std::string normalized = trim(token);
    if (normalized.empty()) {
        return false;
    }

    std::size_t parsed = 0;
    try {
        value = std::stod(normalized, &parsed);
    } catch (const std::exception&) {
        return false;
    }
    return parsed == normalized.size();
}

}  // namespace

FileDataSource::FileDataSource(const std::string& path, std::size_t feature_dim)
    : input_(path), path_(path), feature_dim_(feature_dim)
{
    if (feature_dim_ == 0) {
        throw std::runtime_error("feature_dim must be greater than 0");
    }
    if (!input_) {
        throw std::runtime_error("Failed to open input file: " + path_);
    }
}

bool FileDataSource::readSample(std::vector<double>& sample)
{
    std::string line;
    while (std::getline(input_, line)) {
        ++line_number_;
        line = trim(line);
        if (line.empty() || line[0] == '#') {
            continue;
        }

        try {
            sample = parseLine(line);
            read_sample_ = true;
            return true;
        } catch (const std::runtime_error&) {
            if (!skipped_header_ && !read_sample_) {
                skipped_header_ = true;
                continue;
            }
            throw;
        }
    }
    return false;
}

std::size_t FileDataSource::lineNumber() const
{
    return line_number_;
}

std::vector<double> FileDataSource::parseLine(const std::string& line) const
{
    const auto fields = splitFields(line);
    std::vector<double> values;
    values.reserve(fields.size());
    for (const auto& field : fields) {
        double value = 0.0;
        if (!parseDouble(field, value)) {
            throw std::runtime_error("Invalid numeric value in input file '" + path_ + "' at line " + std::to_string(line_number_));
        }
        values.push_back(value);
    }

    if (values.size() < feature_dim_) {
        throw std::runtime_error("Input file '" + path_ + "' line " + std::to_string(line_number_) + " has fewer numeric columns than feature_dim");
    }

    return std::vector<double>(values.end() - static_cast<std::ptrdiff_t>(feature_dim_), values.end());
}

}  // namespace header_ai
