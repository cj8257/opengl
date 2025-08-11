
#pragma once
#include <mutex>
#include <vector>

struct DataPoint {
    double timestamp;
    double sensor_a;
    double sensor_b;
};

class DataManager {
public:
    void addData(const DataPoint& point);
    void clear();
    std::vector<DataPoint> getData();

private:
    std::vector<DataPoint> buffer;
    std::mutex mutex;
    const size_t maxSize = 1000;
};
