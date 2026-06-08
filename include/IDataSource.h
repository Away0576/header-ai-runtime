#pragma once

#include <vector>

namespace header_ai {

class IDataSource {
public:
    virtual ~IDataSource() = default;
    virtual bool readSample(std::vector<double>& sample) = 0;
};

}  // namespace header_ai
