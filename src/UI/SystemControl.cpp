// 引入SystemControl类的头文件，该文件定义了SystemControl类的接口。
#include "UI/SystemControl.h"
// 引入Dear ImGui库的头文件，用于创建图形用户界面。
#include "imgui.h"
// 引入C++标准库中的<fstream>头文件，用于文件流操作（读取和写入文件）。
#include <fstream>
// 引入C++标准库中的<iostream>头文件，用于控制台输入输出（如调试信息）。
#include <iostream>
// 引入C++标准库中的<sstream>头文件，用于字符串流操作，方便从字符串中解析数据。
#include <sstream>
// 引入C标准库中的<cstring>头文件，提供C风格字符串处理函数（此处未使用，但可能是为了snprintf的兼容性）。
#include <cstring>

// SystemControl类的构造函数。
SystemControl::SystemControl() {
    // 在构造函数中调用方法，从文件中加载系统参数。
    loadParametersFromFile();

    // 初始化用于UI输入框的字符缓冲区，将加载的数值参数转换为字符串。
    // 使用snprintf安全地格式化字符串，防止缓冲区溢出。
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_currentParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_currentParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_currentParams.samplingRate);
}

// SystemControl类的析构函数。
SystemControl::~SystemControl() {
    // 在析构函数中调用方法，将当前的系统参数保存到文件，以实现持久化。
    saveParametersToFile();
}

// 绘制顶部系统控制按钮栏的函数。
void SystemControl::drawControlButtons() {
    // 获取ImGui的IO对象，从中可以获取显示区域的大小等信息。
    ImGuiIO& io = ImGui::GetIO();
    // 获取主显示窗口的宽度。
    float windowWidth = io.DisplaySize.x;
    // 获取主显示窗口的高度。
    float windowHeight = io.DisplaySize.y;

    // 设置下一个要创建的窗口的位置和大小，使其固定在屏幕左上角并横跨大部分屏幕。
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(windowWidth - 40, 70), ImGuiCond_Always);

    // 开始创建一个无边框、无标题的ImGui窗口作为控制按钮的容器。
    ImGui::Begin("##SystemControl", nullptr,
        ImGuiWindowFlags_NoResize |      // 禁止调整大小。
        ImGuiWindowFlags_NoMove |        // 禁止移动。
        ImGuiWindowFlags_NoCollapse |    // 禁止折叠。
        ImGuiWindowFlags_NoTitleBar |    // 移除标题栏。
        ImGuiWindowFlags_NoScrollbar);   // 移除滚动条。

    // 推入自定义的UI样式变量，以美化控件。
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 8)); // 设置控件（如按钮）的内边距。
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8)); // 设置控件之间的间距。

    // 计算内容垂直居中所需的位置。
    float controlWindowHeight = ImGui::GetWindowHeight(); // 获取当前容器窗口的高度。
    float contentHeight = 35; // 假设按钮的高度是35像素。
    float centerY = (controlWindowHeight - contentHeight) * 0.5f; // 计算Y轴的起始位置。
    ImGui::SetCursorPosY(centerY); // 将光标移动到计算出的垂直居中位置。

    // --- 1. 绘制左侧的系统名称（并增大字体） ---
    ImGui::PushFont(nullptr); // 准备修改字体设置（此处使用默认字体）。
    ImGui::SetWindowFontScale(1.8f); // 将字体放大1.8倍。
    ImGui::Text("信号处理系统");     // 显示系统名称文本。
    ImGui::SetWindowFontScale(1.0f); // 恢复正常的字体大小，以不影响后续控件。
    ImGui::PopFont(); // 结束字体修改。

    // --- 2. 让接下来的按钮组与标题在同一行显示 ---
    ImGui::SameLine();

        // --- 按钮右对齐逻辑 ---
        // 1. 定义按钮和间距的尺寸。
        float buttonWidth = 85.0f; // 单个按钮的宽度。
        float itemSpacingX = ImGui::GetStyle().ItemSpacing.x; // 获取当前样式中项的水平间距。
        int buttonCount = 3; // 按钮的数量。
    
        // 2. 计算所有按钮和它们之间间距的总宽度。
        float totalButtonsWidth = (buttonWidth * buttonCount) + (itemSpacingX * (buttonCount - 1));
    
        // 3. 获取窗口内容区域当前可用的水平宽度。
        float availableWidth = ImGui::GetContentRegionAvail().x;
    
        // 4. 计算右对齐所需的起始X坐标，并设置光标位置。
        float rightMargin = -150.0f; // 设置一个额外的右边距，使按钮组更靠右。
        // 如果可用空间足够容纳按钮组和边距。
        if (availableWidth > totalButtonsWidth + rightMargin) {
            // 将光标的X位置设置为计算出的值，实现右对齐。
            ImGui::SetCursorPosX(availableWidth - totalButtonsWidth - rightMargin);
        }

    // --- 开始按钮 ---
    // 根据系统当前状态决定按钮是可点击还是禁用。
    if (m_currentParams.status == SystemStatus::STOPPED) {
        // 如果系统已停止，则显示一个可点击的蓝色“开始”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.50f, 0.90f, 1.0f));        // 正常状态
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.60f, 1.00f, 1.0f)); // 悬停状态
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.40f, 0.80f, 1.0f));  // 按下状态
        if (ImGui::Button("▶ 开始", ImVec2(buttonWidth, 35))) { // 创建按钮。
            onStartClicked(); // 如果按钮被点击，则调用相应的处理函数。
        }
        ImGui::PopStyleColor(3); // 弹出3个颜色设置。
    } else {
        // 如果系统正在运行，则显示一个灰色的、禁用的“开始”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f)); // 按钮背景色
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));   // 按钮文字颜色
        ImGui::Button("▶ 开始", ImVec2(buttonWidth, 35)); // 绘制按钮（无点击逻辑）。
        ImGui::PopStyleColor(2); // 弹出2个颜色设置。
    }

    ImGui::SameLine(); // 使下一个按钮在同一行。

    // --- 关闭按钮 ---
    if (m_currentParams.status == SystemStatus::RUNNING) {
        // 如果系统正在运行，则显示一个可点击的红色“关闭”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.15f, 0.15f, 1.0f));
        if (ImGui::Button("X 关闭", ImVec2(buttonWidth, 35))) {
            onStopClicked();
        }
        ImGui::PopStyleColor(3);
    } else {
        // 如果系统已停止，则显示一个禁用的“关闭”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
        ImGui::Button("X 关闭", ImVec2(buttonWidth, 35));
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine(); // 使下一个按钮在同一行。

    // --- 系统按钮 ---
    if (m_currentParams.status == SystemStatus::STOPPED) {
        // 如果系统已停止，则显示一个可点击的灰蓝色“系统”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.40f, 0.50f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.50f, 0.60f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.30f, 0.40f, 1.0f));
        if (ImGui::Button("⚙ 系统", ImVec2(buttonWidth, 35))) {
            onSystemClicked();
        }
        ImGui::PopStyleColor(3);
    } else {
        // 如果系统正在运行，则禁用“系统”按钮，因为参数通常在运行时不能修改。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.50f, 0.55f, 1.0f));
        ImGui::Button("⚙ 系统", ImVec2(buttonWidth, 35));
        ImGui::PopStyleColor(2);
    }

    ImGui::PopStyleVar(2); // 弹出之前推入的2个样式变量。
    ImGui::End(); // 结束控制按钮容器窗口的绘制。
}

// 绘制系统参数配置模态窗口的函数。
void SystemControl::drawSystemConfigWindow() {
    // 如果m_showSystemWindow标志为false，则不显示此窗口。
    if (!m_showSystemWindow) {
        return;
    }

    // 打开一个名为"系统参数配置"的ImGui弹出层（Popup）。
    ImGui::OpenPopup("系统参数配置");

    // --- 窗口居中逻辑 ---
    ImGuiIO& io = ImGui::GetIO();
    // 设置下一个窗口的位置为屏幕中心。
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    // 设置下一个窗口的大小。
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always);

    // 开始绘制一个模态弹出窗口。
    if (ImGui::BeginPopupModal("系统参数配置", &m_showSystemWindow, ImGuiWindowFlags_NoResize)) {
        // --- 参数输入字段 ---
        ImGui::Text("窗口长度(s):"); // 显示标签。
        ImGui::SetNextItemWidth(200); // 设置下一个输入框的宽度。
        // 创建一个文本输入框，绑定到字符缓冲区，并限制只能输入十进制数字。
        ImGui::InputText("##windowLength", m_windowLengthBuffer, sizeof(m_windowLengthBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing(); // 添加一些垂直间距。

        ImGui::Text("步进长度(s):");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##stepLength", m_stepLengthBuffer, sizeof(m_stepLengthBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing();

        ImGui::Text("采样率(hz):");
        ImGui::SetNextItemWidth(200);
        ImGui::InputText("##samplingRate", m_samplingRateBuffer, sizeof(m_samplingRateBuffer), ImGuiInputTextFlags_CharsDecimal);
        ImGui::Spacing();

        ImGui::Separator(); // 添加一条分隔线。
        ImGui::Spacing();

        // --- 确认和取消按钮区域 ---
        float buttonWidth = 100; // 按钮宽度。
        float spacing = 20; // 按钮间距。
        float totalWidth = buttonWidth * 2 + spacing; // 计算总宽度。
        float startPos = (ImGui::GetWindowWidth() - totalWidth) * 0.5f; // 计算使按钮组居中的起始X位置。

        ImGui::SetCursorPosX(startPos); // 将光标移动到居中位置。

        // 绘制绿色的“确认”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.6f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.7f, 0.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.5f, 0.0f, 1.0f));
        if (ImGui::Button("确认", ImVec2(buttonWidth, 35))) {
            onConfirmClicked(); // 点击时调用确认处理函数。
        }
        ImGui::PopStyleColor(3); // 弹出颜色设置。

        ImGui::SameLine(); // 同行显示。
        ImGui::SetCursorPosX(startPos + buttonWidth + spacing); // 移动光标到下一个按钮的位置。

        // 绘制灰色的“取消”按钮。
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        if (ImGui::Button("取消", ImVec2(buttonWidth, 35))) {
            onCancelClicked(); // 点击时调用取消处理函数。
        }
        ImGui::PopStyleColor(3);

        ImGui::EndPopup(); // 结束模态弹出窗口的绘制。
    }
}

// 获取当前系统参数的const引用。
const SystemParameters& SystemControl::getSystemParameters() const {
    return m_currentParams;
}

// 设置系统参数，并立即保存到文件。
void SystemControl::setSystemParameters(const SystemParameters& params) {
    m_currentParams = params;
    saveParametersToFile();
}

// 获取当前系统状态。
SystemStatus SystemControl::getSystemStatus() const {
    return m_currentParams.status;
}

// 设置系统状态，并立即保存到文件。
void SystemControl::setSystemStatus(SystemStatus status) {
    m_currentParams.status = status;
    saveParametersToFile();
}

// 将当前系统参数保存到一个C++头文件中。
void SystemControl::saveParametersToFile() {
    // 创建一个输出文件流，准备写入指定的文件。
    std::ofstream file("../include/allParams.h");
    // 检查文件是否成功打开。
    if (file.is_open()) {
        // 写入头文件保护宏，防止重复包含。
        file << "#pragma once\n";
        // 将每个参数作为宏定义写入文件。
        file << "#define WINDOW_LENGTH " << m_currentParams.windowLength << "\n";
        file << "#define STEP_LENGTH " << m_currentParams.stepLength << "\n";
        file << "#define SAMPLING_RATE " << m_currentParams.samplingRate << "\n";
        // 使用三元运算符将系统状态（枚举）转换为字符串"start"或"stop"。
        file << "#define SYSTEM_STATUS \"" << (m_currentParams.status == SystemStatus::RUNNING ? "start" : "stop") << "\"\n\n";
        // 同时写入一个结构体，方便在代码中以类型安全的方式使用这些参数。
        file << "struct SystemConfig {\n";
        file << "    static constexpr int windowLength = " << m_currentParams.windowLength << ";\n";
        file << "    static constexpr int stepLength = " << m_currentParams.stepLength << ";\n";
        file << "    static constexpr double samplingRate = " << m_currentParams.samplingRate << ";\n";
        file << "    static constexpr const char* status = \"" << (m_currentParams.status == SystemStatus::RUNNING ? "start" : "stop") << "\";\n";
        file << "};\n";
        // 关闭文件。
        file.close();
        // 在控制台打印成功信息。
        std::cout << "系统参数已保存到 include/allParams.h" << std::endl;
    } else {
        // 如果文件打开失败，在标准错误流中打印错误信息。
        std::cerr << "无法打开文件 include/allParams.h 进行写入" << std::endl;
    }
}

// 从C++头文件中加载系统参数。
void SystemControl::loadParametersFromFile() {
    // 创建一个输入文件流，准备从指定文件读取。
    std::ifstream file("../include/allParams.h");
    // 检查文件是否成功打开。
    if (file.is_open()) {
        std::string line; // 用于存储文件中的每一行。
        // 逐行读取文件。
        while (std::getline(file, line)) {
            // 检查当前行是否包含特定的宏定义。
            if (line.find("#define WINDOW_LENGTH") != std::string::npos) {
                // 如果是，创建一个字符串流来解析这一行。
                std::istringstream iss(line);
                std::string token; // 用于丢弃不需要的部分（如"#define", "WINDOW_LENGTH"）。
                // 读取并丢弃前两个标记，然后将第三个标记解析为整数并存入参数结构体。
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
                // 对于状态，检查行中是否包含字符串 "start"。
                if (line.find("\"start\"") != std::string::npos) {
                    // 如果是，则设置状态为RUNNING。
                    m_currentParams.status = SystemStatus::RUNNING;
                } else {
                    // 否则，设置状态为STOPPED。
                    m_currentParams.status = SystemStatus::STOPPED;
                }
            }
        }
        // 关闭文件。
        file.close();
    } else {
        // 如果文件不存在，打印提示信息。
        std::cout << "allParams.h 文件不存在，使用默认参数" << std::endl;
        // 并调用保存函数来创建一个包含默认参数的新文件。
        saveParametersToFile();
    }
}

// “开始”按钮的点击事件处理函数。
void SystemControl::onStartClicked() {
    std::cout << "系统开始运行" << std::endl; // 在控制台打印信息。
    m_currentParams.status = SystemStatus::RUNNING; // 将系统状态更新为RUNNING。
    saveParametersToFile(); // 保存更改到文件。
}

// “关闭”按钮的点击事件处理函数。
void SystemControl::onStopClicked() {
    std::cout << "系统停止运行" << std::endl; // 在控制台打印信息。
    m_currentParams.status = SystemStatus::STOPPED; // 将系统状态更新为STOPPED。
    saveParametersToFile(); // 保存更改到文件。
}

// “系统”按钮的点击事件处理函数。
void SystemControl::onSystemClicked() {
    // 将当前的正式参数复制到一个临时参数结构体中，以便在用户取消时可以恢复。
    m_tempParams = m_currentParams;

    // 更新UI输入框的字符缓冲区，使其显示当前的值。
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_tempParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_tempParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_tempParams.samplingRate);

    // 设置标志为true，以在下一帧显示系统配置窗口。
    m_showSystemWindow = true;
}

// 系统配置窗口中“确认”按钮的点击事件处理函数。
void SystemControl::onConfirmClicked() {
    // 使用try-catch块来安全地将字符串缓冲区的内容转换为数值。
    try {
        // 将字符串转换为整数或双精度浮点数。
        m_tempParams.windowLength = std::stoi(m_windowLengthBuffer);
        m_tempParams.stepLength = std::stoi(m_stepLengthBuffer);
        m_tempParams.samplingRate = std::stod(m_samplingRateBuffer);

        // 对转换后的值进行有效性验证。
        if (m_tempParams.windowLength <= 0 || m_tempParams.stepLength <= 0 || m_tempParams.samplingRate <= 0) {
            std::cerr << "参数值必须大于0" << std::endl; // 如果无效，打印错误并返回。
            return;
        }

        // 如果验证通过，则将临时参数应用到当前的正式参数中。
        m_currentParams.windowLength = m_tempParams.windowLength;
        m_currentParams.stepLength = m_tempParams.stepLength;
        m_currentParams.samplingRate = m_tempParams.samplingRate;

        // 将更新后的参数保存到文件。
        saveParametersToFile();

        // 设置标志为false，以关闭配置窗口。
        m_showSystemWindow = false;

        // 在控制台打印更新后的参数信息。
        std::cout << "系统参数已更新：" << std::endl;
        std::cout << "  窗口长度: " << m_currentParams.windowLength << std::endl;
        std::cout << "  步进长度: " << m_currentParams.stepLength << std::endl;
        std::cout << "  采样率: " << m_currentParams.samplingRate << std::endl;

    } catch (const std::exception& e) {
        // 如果字符串转换失败（例如，输入了非数字字符），捕获异常并打印错误。
        std::cerr << "参数解析错误: " << e.what() << std::endl;
    }
}

// 系统配置窗口中“取消”按钮的点击事件处理函数。
void SystemControl::onCancelClicked() {
    // 恢复UI输入框的显示内容为修改前的原始值。
    snprintf(m_windowLengthBuffer, sizeof(m_windowLengthBuffer), "%d", m_currentParams.windowLength);
    snprintf(m_stepLengthBuffer, sizeof(m_stepLengthBuffer), "%d", m_currentParams.stepLength);
    snprintf(m_samplingRateBuffer, sizeof(m_samplingRateBuffer), "%.1f", m_currentParams.samplingRate);

    // 设置标志为false，以关闭配置窗口。
    m_showSystemWindow = false;

    // 在控制台打印取消操作的信息。
    std::cout << "取消参数修改" << std::endl;
}