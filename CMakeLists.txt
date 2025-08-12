cmake_minimum_required(VERSION 3.15)
project(SensorMonitorApp)
set(CMAKE_CXX_STANDARD 17)

# 指定 vcpkg 工具链（如果使用vcpkg）
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# 构建选项：选择使用原始版本还是重构版本
option(USE_REFACTORED_ARCHITECTURE "Use refactored MainController architecture" ON)

# 查找依赖库
find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZeroMQ REQUIRED)

# ImGui 源码路径
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")

# ImPlot 源码路径
set(IMPLOT_DIR "${CMAKE_SOURCE_DIR}/third_party/implot")

# glad 源码路径
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/third_party/glad")

# 收集 ImGui 源码文件
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 收集 ImPlot 源码文件
file(GLOB IMPLOT_SOURCES
    ${IMPLOT_DIR}/*.cpp
)

# glad 源码文件
set(GLAD_SOURCES
    ${GLAD_DIR}/src/glad.c
)

# 根据选项选择主文件和源码文件
if(USE_REFACTORED_ARCHITECTURE)
    message(STATUS "Building with refactored MainController architecture")
    set(MAIN_SOURCE src/main_refactored.cpp)
    # 包含所有模块源码文件
    file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
        src/Core/*.cpp
        src/IO/*.cpp
        src/UI/*.cpp
    )
else()
    message(STATUS "Building with original main.cpp implementation")
    set(MAIN_SOURCE src/main.cpp)
    set(MODULE_SOURCES "") # 原始版本不需要模块源码
endif()

# 可执行文件
add_executable(SensorMonitor
    ${MAIN_SOURCE}
    ${MODULE_SOURCES}
    ${IMGUI_SOURCES}
    ${IMPLOT_SOURCES}
    ${GLAD_SOURCES}
)

# 包含目录
target_include_directories(SensorMonitor PRIVATE
    include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMPLOT_DIR}
    ${GLAD_DIR}/include
)

# 链接库
target_link_libraries(SensorMonitor PRIVATE
    glfw
    nlohmann_json::nlohmann_json
    zmq
)

# C++17 标准
target_compile_features(SensorMonitor PRIVATE cxx_std_17)

# 输出构建信息
if(USE_REFACTORED_ARCHITECTURE)
    message(STATUS "Features enabled:")
    message(STATUS "  - Modular MVC architecture")
    message(STATUS "  - ImPlot professional charts")
    message(STATUS "  - 128-channel binary data support")
    message(STATUS "  - Play/Pause functionality")
    message(STATUS "  - Performance optimizations")
else()
    message(STATUS "Features enabled:")
    message(STATUS "  - Original monolithic implementation")
    message(STATUS "  - Direct ImGui drawing")
    message(STATUS "  - Manual data handling")
endif()
