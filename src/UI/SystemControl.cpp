#include "UI/SystemControl.h"
#include "imgui.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>

SystemControl::SystemControl() {
    // 构造函数中加载参数
    loadParametersFromFile();

    // 初始化输入缓冲区
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_currentParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_currentParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_currentParams.samplingRate);
}

SystemControl::~SystemControl() {
    // 析构函数中保存参数
    saveParametersToFile();
}

void SystemControl::drawControlButtons() {
    // 获取窗口大小
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;

    // 设置按钮窗口位置（右上角，匹配设计图）
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth - 40, 70), ImGuiCond_Always);

    // 创建系统控制按钮窗口（无标题栏，现代风格）
    ImGui::Begin("##SystemControl", nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);

    // 设置现代按钮样式
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));

    // 计算垂直居中位置
    float controlWindowHeight = ImGui::GetWindowHeight();
    float contentHeight = 35; // 按钮高度
    float centerY = (controlWindowHeight - contentHeight) * 0.5f;
    ImGui::SetCursorPosY(centerY);

    // --- 1. 绘制左侧的系统名称（增大字体） ---
    ImGui::PushFont(nullptr); // 使用默认字体，但设置缩放
    ImGui::SetWindowFontScale(1.8f); // 增大字体 30%
    ImGui::Text("信号处理系统"); // 字体加载失败时的备用方案
    ImGui::SetWindowFontScale(1.0f); // 恢复正常字体大小
    ImGui::PopFont();
    // --- 2. 让按钮组和标题在同一行 ---
    ImGui::SameLine();
        // 1. 定义按钮和间距的尺寸
        float buttonWidth = 85.0f;
        float itemSpacingX = ImGui::GetStyle().ItemSpacing.x;
        int buttonCount = 3;
    
        // 2. 计算所有按钮和间距的总宽度
        float totalButtonsWidth = (buttonWidth * buttonCount) + (itemSpacingX * (buttonCount - 1));
    
        // 3. 获取窗口内容区域的可用宽度
        float availableWidth = ImGui::GetContentRegionAvail().x;
    
        // 4. 计算右对齐的起始X坐标，并设置光标位置（增加右边距）
        float rightMargin = -150.0f; // 增加右边距
        if (availableWidth > totalButtonsWidth + rightMargin) {
            ImGui::SetCursorPosX(availableWidth - totalButtonsWidth - rightMargin);
        }
    // 开始按钮 - 现代蓝色风格
    if (m_currentParams.status == SystemStatus::STOPPED) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.90f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.60f, 1.00f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.40f, 0.80f, 1.0f));
        if (ImGui::Button("▶ 开始", ImVec2(buttonWidth, 35))) {
            onStartClicked();
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
        ImGui::Button("▶ 开始", ImVec2(buttonWidth, 35)); // 禁用状态
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    // 关闭按钮 - 现代红色风格
    if (m_currentParams.status == SystemStatus::RUNNING) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("X 关闭", ImVec2(buttonWidth, 35))) {
            onStopClicked();
        }
        ImGui::PopStyleColor(3);
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
        ImGui::Button("X 关闭", ImVec2(buttonWidth, 35)); // 禁用状态
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine();

    // 系统按钮 - 现代灰蓝色风格
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.40f, 0.50f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.50f, 0.60f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.30f, 0.40f, 1.0f));
    if (ImGui::Button("⚙ 系统", ImVec2(buttonWidth, 35))) {
        onSystemClicked();
    }
    ImGui::PopStyleColor(3);

    ImGui::PopStyleVar(2);
    ImGui::End();
}

void SystemControl::drawSystemConfigWindow() {
    if (!m_showSystemWindow) {
        return;
    }

    // 设置模态窗口
    ImGui::OpenPopup("系统参数配置");

    // 居中显示窗口
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);

    if (ImGui::BeginPopupModal("系统参数配置", &m_showSystemWindow, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("请配置系统参数：");
        ImGui::Separator();
        ImGui::Spacing();

        // 窗口长度输入
        ImGui::Text("窗口长度(s):");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##windowLength", m_windowLengthBuffer, sizeof(m_windowLengthBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing();

        // 步进长度输入
        ImGui::Text("步进长度(s):");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##stepLength", m_stepLengthBuffer, sizeof(m_stepLengthBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing();

        // 采样率输入
        ImGui::Text("采样率(hz):");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##samplingRate", m_samplingRateBuffer, sizeof(m_samplingRateBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing();

        ImGui::Separator();
        ImGui::Spacing();

        // 按钮区域
        float buttonWidth = 100;
        float spacing = 20;
        float totalWidth = buttonWidth * 2 + spacing;
        float startPos = (ImGui::GetWindowWidth() - totalWidth) * 0.5f;

        ImGui::SetCursorPosX(startPos);

        // 确认按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        if (ImGui::Button("确认", ImVec2(buttonWidth, 35))) {
            onConfirmClicked();
        }
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::SetCursorPosX(startPos + buttonWidth + spacing);

        // 取消按钮
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        if (ImGui::Button("取消", ImVec2(buttonWidth, 35))) {
            onCancelClicked();
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup();
    }
}

const SystemParameters& SystemControl::getSystemParameters() const {
    return m_currentParams;
}

void SystemControl::setSystemParameters(const SystemParameters& params) {
    m_currentParams = params;
    saveParametersToFile();
}

SystemStatus SystemControl::getSystemStatus() const {
    return m_currentParams.status;
}

void SystemControl::setSystemStatus(SystemStatus status) {
    m_currentParams.status = status;
    saveParametersToFile();
}

void SystemControl::saveParametersToFile() {
    std::ofstream file("../include/allParams.h");
    if (file.is_open()) {
        file << "#pragma once\n";
        file << "#define WINDOW_LENGTH " << m_currentParams.windowLength << "\n";
        file << "#define STEP_LENGTH " << m_currentParams.stepLength << "\n";
        file << "#define SAMPLING_RATE " << m_currentParams.samplingRate << "\n";
        file << "#define SYSTEM_STATUS \"" << (m_currentParams.status == SystemStatus::RUNNING ? "start" : "stop") << "\"\n\n";
        file << "struct SystemConfig {\n";
        file << "    static constexpr int windowLength = " << m_currentParams.windowLength << ";\n";
        file << "    static constexpr int stepLength = " << m_currentParams.stepLength << ";\n";
        file << "    static constexpr double samplingRate = " << m_currentParams.samplingRate << ";\n";
        file << "    static constexpr const char* status = \"" << (m_currentParams.status == SystemStatus::RUNNING ? "start" : "stop") << "\";\n";
        file << "};\n";
        file.close();
        std::cout << "系统参数已保存到 include/allParams.h" << std::endl;
    } else {
        std::cerr << "无法打开文件 include/allParams.h 进行写入" << std::endl;
    }
}

void SystemControl::loadParametersFromFile() {
    std::ifstream file("../include/allParams.h");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("#define WINDOW_LENGTH") != std::string::npos) {
                std::istringstream iss(line);
                std::string token;
                iss >> token >> token >> m_currentParams.windowLength;
            } else if (line.find("#define STEP_LENGTH") != std::string::npos) {
                std::istringstream iss(line);
                std::string token;
                iss >> token >> token >> m_currentParams.stepLength;
            } else if (line.find("#define SAMPLING_RATE") != std::string::npos) {
                std::istringstream iss(line);
                std::string token;
                iss >> token >> token >> m_currentParams.samplingRate;
            } else if (line.find("#define SYSTEM_STATUS") != std::string::npos) {
                if (line.find("\"start\"") != std::string::npos) {
                    m_currentParams.status = SystemStatus::RUNNING;
                } else {
                    m_currentParams.status = SystemStatus::STOPPED;
                }
            }
        }
        file.close();
    } else {
        // 如果文件不存在，使用默认参数
        std::cout << "allParams.h 文件不存在，使用默认参数" << std::endl;
        saveParametersToFile();
    }
}

void SystemControl::onStartClicked() {
    std::cout << "系统开始运行" << std::endl;
    m_currentParams.status = SystemStatus::RUNNING;
    saveParametersToFile();
}

void SystemControl::onStopClicked() {
    std::cout << "系统停止运行" << std::endl;
    m_currentParams.status = SystemStatus::STOPPED;
    saveParametersToFile();
}

void SystemControl::onSystemClicked() {
    // 复制当前参数到临时参数
    m_tempParams = m_currentParams;

    // 更新输入缓冲区
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_tempParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_tempParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_tempParams.samplingRate);

    // 显示系统配置窗口
    m_showSystemWindow = true;
}

void SystemControl::onConfirmClicked() {
    // 解析输入值
    try {
        m_tempParams.windowLength = std::stoi(m_windowLengthBuffer);
        m_tempParams.stepLength = std::stoi(m_stepLengthBuffer);
        m_tempParams.samplingRate = std::stod(m_samplingRateBuffer);

        // 验证参数有效性
        if (m_tempParams.windowLength <= 0 || m_tempParams.stepLength <= 0 || m_tempParams.samplingRate <= 0) {
            std::cerr << "参数值必须大于0" << std::endl;
            return;
        }

        // 应用新参数
        m_currentParams.windowLength = m_tempParams.windowLength;
        m_currentParams.stepLength = m_tempParams.stepLength;
        m_currentParams.samplingRate = m_tempParams.samplingRate;

        // 保存到文件
        saveParametersToFile();

        // 关闭窗口
        m_showSystemWindow = false;

        std::cout << "系统参数已更新：" << std::endl;
        std::cout << "  窗口长度: " << m_currentParams.windowLength << std::endl;
        std::cout << "  步进长度: " << m_currentParams.stepLength << std::endl;
        std::cout << "  采样率: " << m_currentParams.samplingRate << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "参数解析错误: " << e.what() << std::endl;
    }
}

void SystemControl::onCancelClicked() {
    // 恢复原始输入值
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_currentParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_currentParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_currentParams.samplingRate);

    // 关闭窗口
    m_showSystemWindow = false;

    std::cout << "取消参数修改" << std::endl;
}