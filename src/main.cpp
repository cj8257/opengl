#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"
#include <iostream>
#include <fstream>
#include "UI/MainController.h"

// GLFW错误回调函数
static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

// 使用重构后的MainController架构的主函数
int main() {
    // 设置GLFW错误回调函数
    glfwSetErrorCallback(glfw_error_callback);

    // 初始化GLFW库
    if (!glfwInit())
        return -1;

    // 设置OpenGL版本参数，使用3.3核心模式
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建1280x720分辨率的窗口
    GLFWwindow* window = glfwCreateWindow(1280, 720, "SensorMonitorApp - Refactored", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // 开启垂直同步

    // 初始化GLAD库
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize glad" << std::endl;
        return -1;
    }

    // 初始化ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    // 初始化ImPlot（重构后使用ImPlot绘图库）
    ImPlot::CreateContext();
    
    ImGuiIO& io = ImGui::GetIO();
    
    // 尝试加载中文字体（如果可用的话）
    // 注意：这需要系统中存在中文字体文件
    // 如果没有中文字体，ImGui会使用默认字体显示英文
    const char* font_paths[] = {
        "C:/Windows/Fonts/msyh.ttc",  // 微软雅黑 (Windows)
        "C:/Windows/Fonts/simhei.ttf", // 黑体 (Windows)
        "/System/Library/Fonts/Helvetica.ttc", // macOS
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" // Linux
    };
    
    bool font_loaded = false;
    for (const char* font_path : font_paths) {
        if (std::ifstream(font_path).good()) {
            io.Fonts->AddFontFromFileTTF(font_path, 18.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
            font_loaded = true;
            break;
        }
    }
    
    if (!font_loaded) {
        std::cout << "No Chinese font found, using default font (English only)" << std::endl;
    }
    
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    // 创建主控制器实例（使用重构后的架构）
    const std::string ADDRESS = "tcp://*:5555";
    MainController mainController(ADDRESS);
    
    std::cout << "SensorMonitorApp started with refactored architecture" << std::endl;
    std::cout << "Features:" << std::endl;
    std::cout << "- 128 channels @ 22.5kHz sampling rate" << std::endl;
    std::cout << "- Binary data format support" << std::endl;
    std::cout << "- ImPlot-based professional charts" << std::endl;
    std::cout << "- Modular MVC architecture" << std::endl;
    std::cout << "- Play/Pause functionality" << std::endl;
    std::cout << "- Performance optimizations" << std::endl;

    // 主程序循环
    while (!glfwWindowShouldClose(window)) {
        // 处理GLFW事件
        glfwPollEvents();

        // 开始新的ImGui帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 使用MainController绘制UI（模块化架构）
        mainController.drawUI();

        // 渲染
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // 清理资源
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    std::cout << "SensorMonitorApp shutdown completed" << std::endl;
    return 0;
}