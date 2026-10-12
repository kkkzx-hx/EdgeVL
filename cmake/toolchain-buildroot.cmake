# 工具链配置文件 - 仿 rk_demo 格式
# 用于交叉编译 aarch64 (RK3588) 目标

# 1. 指定目标操作系统和处理器架构
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 2. 指定工具链的根路径
set(TOOLCHAIN_ROOT "/home/<用户名>/toolchains/buildroot-gcc13.3")
set(TOOLCHAIN_BIN "${TOOLCHAIN_ROOT}/bin")

# 3. 指定编译器
set(CMAKE_C_COMPILER   "${TOOLCHAIN_BIN}/aarch64-linux-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN}/aarch64-linux-g++")

# 4. 指定 Sysroot
set(CMAKE_SYSROOT "${TOOLCHAIN_ROOT}/aarch64-buildroot-linux-gnu/sysroot")

# 5. 配置搜索模式
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
