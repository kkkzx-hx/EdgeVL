cmake_minimum_required(VERSION 3.10)
project(qwen_vl VERSION 1.0.0)

# ============================================
# C++ 标准
# ============================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -pthread -O3")

# ============================================
# 头文件路径
# ============================================
# 项目自己的头文件
include_directories(${CMAKE_SOURCE_DIR}/include)

# OpenCV 头文件（项目内副本，避免工具链路径拦截）
include_directories(BEFORE ${CMAKE_SOURCE_DIR}/3rdparty/opencv/include)

# ============================================
# 库文件路径
# ============================================
link_directories(${CMAKE_SOURCE_DIR}/lib)

# ============================================
# 源文件
# ============================================
set(SOURCES
    src/main.cpp
    src/RK35llm.cpp
)

# ============================================
# 可执行文件
# ============================================
add_executable(VLM_NPU ${SOURCES})

# ============================================
# 链接依赖
# ============================================
# OpenCV（来自板子，aarch64 .so）
# rkllm / rknn 运行时库
# 系统库
target_link_libraries(VLM_NPU
    # OpenCV
    ${CMAKE_SOURCE_DIR}/lib/libopencv_core.so
    ${CMAKE_SOURCE_DIR}/lib/libopencv_imgcodecs.so
    ${CMAKE_SOURCE_DIR}/lib/libopencv_imgproc.so
    # Rockchip 运行时
    ${CMAKE_SOURCE_DIR}/lib/librknnrt.so
    ${CMAKE_SOURCE_DIR}/lib/librkllmrt.so
    # 辅助库
    ${CMAKE_SOURCE_DIR}/lib/libpng16.so.16
    ${CMAKE_SOURCE_DIR}/lib/libz.so.1
    # 系统
    ${CMAKE_SOURCE_DIR}/lib/libgomp.so.1
    -ldl -lpthread
)

# ============================================
# 输出路径
# ============================================
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR})

# ============================================
# 安装规则（安装到 test/ 目录）
# ============================================
install(TARGETS VLM_NPU
    RUNTIME DESTINATION bin
)

install(DIRECTORY lib/
    DESTINATION lib
    FILES_MATCHING PATTERN "*.so*"
)

install(DIRECTORY models/
    DESTINATION models
)

install(DIRECTORY images/
    DESTINATION images
)
