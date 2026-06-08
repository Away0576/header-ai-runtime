#pragma once

#include <cstddef>
#include <string>

namespace header_ai {

enum class AlarmState {
    Normal,
    Alarm,
};

class AnomalyDetector {
public:
    AnomalyDetector(std::size_t consecutive_count, std::size_t clear_count);

    AlarmState update(bool is_anomaly);
    AlarmState state() const;
    std::size_t anomalyCount() const;
    std::size_t normalCount() const;

private:
    std::size_t consecutive_count_;
    std::size_t clear_count_;
    std::size_t anomaly_count_{};
    std::size_t normal_count_{};
    AlarmState state_{AlarmState::Normal};
};

std::string toString(AlarmState state);

}  // namespace header_ai
