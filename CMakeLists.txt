cmake_minimum_required(VERSION 3.15)
project(SensorMonitorApp)
set(CMAKE_CXX_STANDARD 17)

# 指定 vcpkg 工具链（如果使用vcpkg）
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")

# 查找依赖库
find_package(glfw3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(ZeroMQ REQUIRED)
# ImGui 源码路径
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")

# glad 源码路径
set(GLAD_DIR "${CMAKE_SOURCE_DIR}/third_party/glad")

# 收集 ImGui 源码文件
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# glad 源码文件
set(GLAD_SOURCES
    ${GLAD_DIR}/src/glad.c
)

# 项目源码文件
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

# 可执行文件
add_executable(SensorMonitor
    ${SOURCES}
    ${IMGUI_SOURCES}
    ${GLAD_SOURCES}
)

# 包含目录
target_include_directories(SensorMonitor PRIVATE
    include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
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
