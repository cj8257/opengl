
#include "Core/DataManager.h"  // 包含数据管理器头文件

void DataManager::addData(const DataPoint& point) {  // 添加数据点的方法
    std::lock_guard<std::mutex> lock(mutex);  // 使用互斥锁保护线程安全
    if (buffer.size() >= maxSize)  // 检查缓冲区是否达到最大容量
        buffer.erase(buffer.begin());  // 移除最旧的数据点
    buffer.push_back(point);  // 将新数据点添加到缓冲区末尾
}

void DataManager::clear() {  // 清空所有数据的方法
    std::lock_guard<std::mutex> lock(mutex);  // 使用互斥锁保护线程安全
    buffer.clear();  // 清空数据缓冲区
}

std::vector<DataPoint> DataManager::getData() {  // 获取所有数据的方法
    std::lock_guard<std::mutex> lock(mutex);  // 使用互斥锁保护线程安全
    return buffer;  // 返回当前所有数据
}
