/*
 * @Author: chengjun 1@a.com
 * @Date: 2025-09-18 11:03:51
 * @LastEditors: chengjun 1@a.com
 * @LastEditTime: 2025-09-22 16:09:45
 * @FilePath: \SensorMonitorApp\include\UI\SystemControl.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <string>

// 系统状态枚举
enum class SystemStatus {
    STOPPED,
    RUNNING
};

// 系统参数结构体
struct SystemParameters {
    int windowLength = 1024;      // 窗口长度
    int stepLength = 512;         // 步进长度
    double samplingRate = 22500.0; // 采样率
    SystemStatus status = SystemStatus::STOPPED; // 系统状态
};

class SystemControl {
public:
    SystemControl();
    ~SystemControl();

    // 绘制系统控制按钮（右上角的三个按钮）
    void drawControlButtons();

    // 绘制系统参数配置窗口
    void drawSystemConfigWindow();

    // 获取当前系统参数
    const SystemParameters& getSystemParameters() const;

    // 设置系统参数
    void setSystemParameters(const SystemParameters& params);

    // 获取系统状态
    SystemStatus getSystemStatus() const;

    // 设置系统状态
    void setSystemStatus(SystemStatus status);

private:
    // 保存参数到main.h文件
    void saveParametersToFile();

    // 从main.h文件加载参数
    void loadParametersFromFile();

    // 开始按钮回调
    void onStartClicked();

    // 关闭按钮回调
    void onStopClicked();

    // 系统按钮回调
    void onSystemClicked();

    // 确认按钮回调（系统配置窗口）
    void onConfirmClicked();

    // 取消按钮回调（系统配置窗口）
    void onCancelClicked();

private:
    SystemParameters m_currentParams;    // 当前系统参数
    SystemParameters m_tempParams;       // 临时参数（用于系统配置窗口）
    bool m_showSystemWindow = false;     // 是否显示系统配置窗口

    // ImGui输入缓冲区
    char m_windowLengthBuffer[32] = "1024";
    char m_stepLengthBuffer[32] = "512";
    char m_samplingRateBuffer[32] = "22500.0";
};