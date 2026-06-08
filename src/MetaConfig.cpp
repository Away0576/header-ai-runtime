#include "MetaConfig.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace header_ai {
namespace {

class JsonCursor {
public:
    explicit JsonCursor(std::string text) : text_(std::move(text)) {}

    bool consume(char expected)
    {
        skipWhitespace();
        if (pos_ >= text_.size() || text_[pos_] != expected) {
            return false;
        }
        ++pos_;
        return true;
    }

    void expect(char expected, const std::string& context)
    {
        if (!consume(expected)) {
            throw std::runtime_error("Invalid meta.json: expected '" + std::string(1, expected) + "' while parsing " + context);
        }
    }

    std::string readString(const std::string& context)
    {
        skipWhitespace();
        if (pos_ >= text_.size() || text_[pos_] != '"') {
            throw std::runtime_error("Invalid meta.json: expected string while parsing " + context);
        }
        ++pos_;
        std::string value;
        while (pos_ < text_.size()) {
            const char ch = text_[pos_++];
            if (ch == '"') {
                return value;
            }
            if (ch == '\\') {
                if (pos_ >= text_.size()) {
                    throw std::runtime_error("Invalid meta.json: unterminated escape while parsing " + context);
                }
                const char escaped = text_[pos_++];
                switch (escaped) {
                    case '"':
                    case '\\':
                    case '/':
                        value.push_back(escaped);
                        break;
                    case 'b':
                        value.push_back('\b');
                        break;
                    case 'f':
                        value.push_back('\f');
                        break;
                    case 'n':
                        value.push_back('\n');
                        break;
                    case 'r':
                        value.push_back('\r');
                        break;
                    case 't':
                        value.push_back('\t');
                        break;
                    default:
                        throw std::runtime_error("Invalid meta.json: unsupported escape while parsing " + context);
                }
            } else {
                value.push_back(ch);
            }
        }
        throw std::runtime_error("Invalid meta.json: unterminated string while parsing " + context);
    }

    double readNumber(const std::string& context)
    {
        skipWhitespace();
        const std::size_t start = pos_;
        if (pos_ < text_.size() && text_[pos_] == '-') {
            ++pos_;
        }
        readDigits();
        if (pos_ < text_.size() && text_[pos_] == '.') {
            ++pos_;
            readDigits();
        }
        if (pos_ < text_.size() && (text_[pos_] == 'e' || text_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < text_.size() && (text_[pos_] == '+' || text_[pos_] == '-')) {
                ++pos_;
            }
            readDigits();
        }
        if (start == pos_) {
            throw std::runtime_error("Invalid meta.json: expected number while parsing " + context);
        }
        return std::stod(text_.substr(start, pos_ - start));
    }

    std::vector<double> readNumberArray(const std::string& context)
    {
        std::vector<double> values;
        expect('[', context);
        skipWhitespace();
        if (consume(']')) {
            return values;
        }
        while (true) {
            values.push_back(readNumber(context));
            skipWhitespace();
            if (consume(']')) {
                return values;
            }
            expect(',', context);
        }
    }

    bool eof()
    {
        skipWhitespace();
        return pos_ == text_.size();
    }

    void skipValue(const std::string& context)
    {
        skipWhitespace();
        if (pos_ >= text_.size()) {
            throw std::runtime_error("Invalid meta.json: unexpected end while skipping " + context);
        }
        const char ch = text_[pos_];
        if (ch == '"') {
            (void)readString(context);
        } else if (ch == '{') {
            expect('{', context);
            skipWhitespace();
            if (consume('}')) {
                return;
            }
            while (true) {
                (void)readString(context);
                expect(':', context);
                skipValue(context);
                if (consume('}')) {
                    return;
                }
                expect(',', context);
            }
        } else if (ch == '[') {
            expect('[', context);
            skipWhitespace();
            if (consume(']')) {
                return;
            }
            while (true) {
                skipValue(context);
                if (consume(']')) {
                    return;
                }
                expect(',', context);
            }
        } else if (std::isdigit(static_cast<unsigned char>(ch)) || ch == '-') {
            (void)readNumber(context);
        } else if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
        } else if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
        } else if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
        } else {
            throw std::runtime_error("Invalid meta.json: unsupported value while skipping " + context);
        }
    }

private:
    void skipWhitespace()
    {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    void readDigits()
    {
        const std::size_t start = pos_;
        while (pos_ < text_.size() && std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
        if (start == pos_) {
            throw std::runtime_error("Invalid meta.json: expected digit in number");
        }
    }

    std::string text_;
    std::size_t pos_{};
};

std::size_t readSize(double value, const std::string& field)
{
    if (value < 0.0 || value != static_cast<std::size_t>(value)) {
        throw std::runtime_error("Invalid meta.json: " + field + " must be a non-negative integer");
    }
    return static_cast<std::size_t>(value);
}

int readInt(double value, const std::string& field)
{
    if (value != static_cast<int>(value)) {
        throw std::runtime_error("Invalid meta.json: " + field + " must be an integer");
    }
    return static_cast<int>(value);
}

void parseNormalization(JsonCursor& cursor, MetaConfig& config)
{
    bool has_type = false;
    bool has_mean = false;
    bool has_std = false;
    cursor.expect('{', "normalization");
    while (true) {
        const std::string key = cursor.readString("normalization key");
        cursor.expect(':', "normalization");
        if (key == "type") {
            config.normalization.type = cursor.readString("normalization.type");
            has_type = true;
        } else if (key == "mean") {
            config.normalization.mean = cursor.readNumberArray("normalization.mean");
            has_mean = true;
        } else if (key == "std") {
            config.normalization.std = cursor.readNumberArray("normalization.std");
            has_std = true;
        } else {
            cursor.skipValue("normalization." + key);
        }
        if (cursor.consume('}')) {
            break;
        }
        cursor.expect(',', "normalization");
    }
    if (!has_type || !has_mean || !has_std) {
        throw std::runtime_error("Invalid meta.json: normalization requires type, mean, and std");
    }
}

void parseAlarm(JsonCursor& cursor, MetaConfig& config)
{
    bool has_consecutive_count = false;
    bool has_clear_count = false;
    cursor.expect('{', "alarm");
    while (true) {
        const std::string key = cursor.readString("alarm key");
        cursor.expect(':', "alarm");
        if (key == "consecutive_count") {
            config.alarm.consecutive_count = readSize(cursor.readNumber("alarm.consecutive_count"), "alarm.consecutive_count");
            has_consecutive_count = true;
        } else if (key == "clear_count") {
            config.alarm.clear_count = readSize(cursor.readNumber("alarm.clear_count"), "alarm.clear_count");
            has_clear_count = true;
        } else {
            cursor.skipValue("alarm." + key);
        }
        if (cursor.consume('}')) {
            break;
        }
        cursor.expect(',', "alarm");
    }
    if (!has_consecutive_count || !has_clear_count) {
        throw std::runtime_error("Invalid meta.json: alarm requires consecutive_count and clear_count");
    }
}

void parseOnnx(JsonCursor& cursor, MetaConfig& config)
{
    bool has_opset = false;
    cursor.expect('{', "onnx");
    while (true) {
        const std::string key = cursor.readString("onnx key");
        cursor.expect(':', "onnx");
        if (key == "opset") {
            config.onnx.opset = readInt(cursor.readNumber("onnx.opset"), "onnx.opset");
            has_opset = true;
        } else {
            cursor.skipValue("onnx." + key);
        }
        if (cursor.consume('}')) {
            break;
        }
        cursor.expect(',', "onnx");
    }
    if (!has_opset) {
        throw std::runtime_error("Invalid meta.json: onnx requires opset");
    }
}

}  // namespace

MetaConfig loadMetaConfigFromFile(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Failed to open meta.json: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parseMetaConfig(buffer.str());
}

MetaConfig parseMetaConfig(const std::string& json_text)
{
    MetaConfig config;
    bool has_schema_version = false;
    bool has_model_type = false;
    bool has_input_name = false;
    bool has_output_name = false;
    bool has_window_size = false;
    bool has_feature_dim = false;
    bool has_input_dim = false;
    bool has_flatten_order = false;
    bool has_threshold = false;
    bool has_threshold_percentile = false;
    bool has_normalization = false;
    bool has_alarm = false;
    bool has_onnx = false;

    JsonCursor cursor(json_text);
    cursor.expect('{', "root");
    while (true) {
        const std::string key = cursor.readString("root key");
        cursor.expect(':', "root");
        if (key == "schema_version") {
            config.schema_version = cursor.readString(key);
            has_schema_version = true;
        } else if (key == "model_type") {
            config.model_type = cursor.readString(key);
            has_model_type = true;
        } else if (key == "input_name") {
            config.input_name = cursor.readString(key);
            has_input_name = true;
        } else if (key == "output_name") {
            config.output_name = cursor.readString(key);
            has_output_name = true;
        } else if (key == "window_size") {
            config.window_size = readSize(cursor.readNumber(key), key);
            has_window_size = true;
        } else if (key == "feature_dim") {
            config.feature_dim = readSize(cursor.readNumber(key), key);
            has_feature_dim = true;
        } else if (key == "input_dim") {
            config.input_dim = readSize(cursor.readNumber(key), key);
            has_input_dim = true;
        } else if (key == "flatten_order") {
            config.flatten_order = cursor.readString(key);
            has_flatten_order = true;
        } else if (key == "threshold") {
            config.threshold = cursor.readNumber(key);
            has_threshold = true;
        } else if (key == "threshold_percentile") {
            config.threshold_percentile = cursor.readNumber(key);
            has_threshold_percentile = true;
        } else if (key == "normalization") {
            parseNormalization(cursor, config);
            has_normalization = true;
        } else if (key == "alarm") {
            parseAlarm(cursor, config);
            has_alarm = true;
        } else if (key == "onnx") {
            parseOnnx(cursor, config);
            has_onnx = true;
        } else {
            cursor.skipValue(key);
        }
        if (cursor.consume('}')) {
            break;
        }
        cursor.expect(',', "root");
    }
    if (!cursor.eof()) {
        throw std::runtime_error("Invalid meta.json: unexpected content after root object");
    }

    if (!has_schema_version || !has_model_type || !has_input_name || !has_output_name || !has_window_size ||
        !has_feature_dim || !has_input_dim || !has_flatten_order || !has_threshold || !has_threshold_percentile ||
        !has_normalization || !has_alarm || !has_onnx) {
        throw std::runtime_error("Invalid meta.json: missing required root field");
    }
    validateMetaConfig(config);
    return config;
}

void validateMetaConfig(const MetaConfig& config)
{
    if (config.schema_version != "1.0") {
        throw std::runtime_error("Invalid meta.json: unsupported schema_version '" + config.schema_version + "'");
    }
    if (config.model_type != "autoencoder") {
        throw std::runtime_error("Invalid meta.json: model_type must be autoencoder");
    }
    if (config.input_name.empty() || config.output_name.empty()) {
        throw std::runtime_error("Invalid meta.json: input_name and output_name must not be empty");
    }
    if (config.window_size == 0) {
        throw std::runtime_error("Invalid meta.json: window_size must be greater than 0");
    }
    if (config.feature_dim == 0) {
        throw std::runtime_error("Invalid meta.json: feature_dim must be greater than 0");
    }
    if (config.input_dim != config.window_size * config.feature_dim) {
        throw std::runtime_error("Invalid meta.json: input_dim must equal window_size * feature_dim");
    }
    if (config.flatten_order != "time_major") {
        throw std::runtime_error("Invalid meta.json: flatten_order must be time_major");
    }
    if (config.threshold < 0.0) {
        throw std::runtime_error("Invalid meta.json: threshold must be greater than or equal to 0");
    }
    if (config.normalization.type != "standard") {
        throw std::runtime_error("Invalid meta.json: normalization.type must be standard");
    }
    if (config.normalization.mean.size() != config.feature_dim) {
        throw std::runtime_error("Invalid meta.json: normalization.mean length must equal feature_dim");
    }
    if (config.normalization.std.size() != config.feature_dim) {
        throw std::runtime_error("Invalid meta.json: normalization.std length must equal feature_dim");
    }
    for (double std_value : config.normalization.std) {
        if (std_value == 0.0) {
            throw std::runtime_error("Invalid meta.json: normalization.std must not contain 0");
        }
    }
    if (config.alarm.consecutive_count == 0 || config.alarm.clear_count == 0) {
        throw std::runtime_error("Invalid meta.json: alarm consecutive_count and clear_count must be greater than 0");
    }
}

}  // namespace header_ai
