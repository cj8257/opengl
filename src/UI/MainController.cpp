
#include "UI/MainController.h"  // 包含主控制器头文件
#include <nlohmann/json.hpp>  // 包含JSON库头文件
#include <imgui.h>  // 包含ImGui GUI库头文件

using json = nlohmann::json;  // 定义JSON类型别名

MainController::MainController(const std::string& endpoint) : subscriber(endpoint) {}  // 构造函数，初始化ZeroMQ订阅器

MainController::~MainController() {
    subscriber.stop();  // 停止ZeroMQ订阅服务
}

void MainController::toggle() {  // 切换运行状态的方法
    if (running) {  // 如果正在运行
        subscriber.stop();  // 停止订阅服务
        running = false;  // 设置运行状态为false
    } else {  // 如果未运行
        subscriber.start([this](const std::string& msg) {  // 启动订阅服务并设置回调函数
            try {  // 尝试解析JSON数据
                auto j = json::parse(msg);  // 解析JSON字符串
                DataPoint point;  // 创建数据点对象
                point.timestamp = j["timestamp"];  // 设置时间戳
                point.sensor_a = j["sensor_a"];  // 设置传感器A的值
                point.sensor_b = j["sensor_b"];  // 设置传感器B的值
                dataManager.addData(point);  // 将数据点添加到管理器
            } catch (...) {}  // 捕获所有异常并忽略
        });  // 结束回调函数
        running = true;  // 设置运行状态为true
    }  // 结束else分支
}

void MainController::clear() {  // 清除数据的方法
    dataManager.clear();  // 清除所有数据
}

void MainController::drawUI() {  // 绘制用户界面的方法
    if (ImGui::Button(running ? "暂停" : "开始")) toggle();  // 绘制开始/暂停按钮
    ImGui::SameLine();  // 在同一行绘制下一个控件
    if (ImGui::Button("清除")) clear();  // 绘制清除按钮

    auto data = dataManager.getData();  // 获取所有数据
    std::vector<float> a, b;  // 创建两个传感器数据的向量
    for (const auto& d : data) {  // 遍历所有数据点
        a.push_back(static_cast<float>(d.sensor_a));  // 添加传感器A的值
        b.push_back(static_cast<float>(d.sensor_b));  // 添加传感器B的值
    }
    if (!a.empty()) ImGui::PlotLines("Sensor A", a.data(), a.size());  // 绘制传感器A的数据曲线
    if (!b.empty()) ImGui::PlotLines("Sensor B", b.data(), b.size());  // 绘制传感器B的数据曲线
}
