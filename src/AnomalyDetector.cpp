#include "AnomalyDetector.h"

#include <stdexcept>

namespace header_ai {

AnomalyDetector::AnomalyDetector(std::size_t consecutive_count, std::size_t clear_count)
    : consecutive_count_(consecutive_count), clear_count_(clear_count)
{
    if (consecutive_count_ == 0) {
        throw std::runtime_error("consecutive_count must be greater than 0");
    }
    if (clear_count_ == 0) {
        throw std::runtime_error("clear_count must be greater than 0");
    }
}

AlarmState AnomalyDetector::update(bool is_anomaly)
{
    if (is_anomaly) {
        ++anomaly_count_;
        normal_count_ = 0;
    } else {
        ++normal_count_;
        anomaly_count_ = 0;
    }

    if (anomaly_count_ >= consecutive_count_) {
        state_ = AlarmState::Alarm;
    }
    if (normal_count_ >= clear_count_) {
        state_ = AlarmState::Normal;
    }
    return state_;
}

AlarmState AnomalyDetector::state() const
{
    return state_;
}

std::size_t AnomalyDetector::anomalyCount() const
{
    return anomaly_count_;
}

std::size_t AnomalyDetector::normalCount() const
{
    return normal_count_;
}

std::string toString(AlarmState state)
{
    switch (state) {
        case AlarmState::Normal:
            return "NORMAL";
        case AlarmState::Alarm:
            return "ALARM";
    }
    throw std::runtime_error("unknown alarm state");
}

}  // namespace header_ai
