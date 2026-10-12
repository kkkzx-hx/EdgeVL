# EdgeVL

> 在 RK3588 边缘设备上部署 **Qwen2.5-VL-3B** 多模态大模型，端到端实现「图片 + 提问 → AI 回答」。NPU 本地推理，**完全离线、数据不出设备**。

## ✨ 项目亮点

- **端到端多模态推理**：图片输入 → 视觉编码 → 大模型问答，一条命令跑通全流程
- **W8A8 量化 + FP16 视觉**：将 3B 参数模型压缩部署到资源受限的边缘端，实现模型量化与端侧推理优化
- **NPU 本地推理**：完全离线运行，摆脱云端依赖，隐私安全、低延迟
- **C++17 工程封装**：推理核心类封装，接口简洁，便于二次集成
- **完整的落地踩坑经验**：解决交叉编译、动态库依赖、超大模型文件传输等一系列真实部署难题

## 🎬 效果展示

![EdgeVL 运行效果](images/demo.png)

在 RK3588 板端运行后，输入 `<image>` 标签 + 问题，模型即可基于图片内容作答。示例对话：

```
User: <image>
      描述一下这张图片
AI  : 图中是一轮满月，悬挂在夜空中……
```

## 🏗️ 技术架构

```
┌─────────────┐    ┌──────────────────┐    ┌─────────────────┐
│  图片输入    │───▶│  视觉编码器(ViT)  │───▶│  语言模型(LLM)   │───▶ 文本回答
│  (stb_image)│    │  RKNN / FP16      │    │  RKLLM / W8A8    │
└─────────────┘    └──────────────────┘    └─────────────────┘
        均在 RK3588 NPU 上本地推理，无需云端
```

**部署方案**：Qwen2.5-VL-3B 拆分为视觉编码器与语言模型两部分，视觉部分以 FP16 精度经 RKNN 工具链转换为 NPU 格式，语言部分经 W8A8 量化后通过 RKLLM 运行时在 NPU 上推理。

## 🛠️ 快速开始

### 环境要求

- 硬件：RK3588 开发板（Buildroot 系统）
- 交叉编译器：`aarch64-linux-gnu-g++`（Ubuntu/WSL 安装）
- 运行时：rknpu 驱动 + rkllm-runtime

### 编译（交叉编译为 ARM64 可执行文件）

```bash
aarch64-linux-gnu-g++ -std=c++17 -O3 \
  -I include -I /usr/include/opencv4 \
  src/main.cpp src/RK35llm.cpp \
  ./lib/libopencv_core.so ./lib/libopencv_imgcodecs.so ./lib/libopencv_imgproc.so \
  ./lib/libpng16.so.16 ./lib/libz.so.1 \
  ./lib/librknnrt.so ./lib/librkllmrt.so \
  ./lib/libgomp.so.1 \
  -ldl -lpthread \
  -o VLM_NPU_stb
aarch64-linux-gnu-strip VLM_NPU_stb
```

### 板端运行

```bash
# 一键运行（默认测试图片）
./run_board.sh

# 自定义图片
./run_board.sh /root/你的图片.jpg
```

对话时，图片相关问题需以 `<image>` 标签开头：

```
<image>
描述一下这张图片
```

## 🔬 核心技术方案

### 1. 模型量化（W8A8 + FP16 混合）

- 语言模型采用 **W8A8 对称量化**，权重与激活均压缩为 8bit，大幅降低显存与带宽占用
- 视觉编码器保留 **FP16 精度**，保证图像特征提取质量，避免量化损失影响多模态理解

### 2. NPU 端侧推理

- 基于 RKLLM / RKNN 运行时，模型完整跑在 RK3588 的 NPU 上
- 模型文件放置于板端本地存储，规避 NFS 读大文件的不可中断等待问题

### 3. 图片读取优化

- 板端 OpenCV 缺失 JPEG 解码器，`cv::imread` 无法使用
- 改用 **header-only 的 stb_image** 跨平台读图，零外部依赖，彻底规避解码器缺失

### 4. 大文件传输

- 2GB+ 模型文件经 NFS/SCP 直传易断连，采用 **200MB 分块拆分 + 板端合并** 方案，并校验文件大小

## 📝 部署踩坑记录

| 问题 | 原因 | 解决 |
|------|------|------|
| `rkllm init` 卡死不响应 | NFS 读大文件进入内核不可中断等待 | 模型必须放板子本地 |
| 传大文件 `Connection reset` | 板子网络大流量不稳定 | 200MB 分块传输 |
| `!_src.empty()` in cvtColor | OpenCV 缺少 libjpeg 解码器 | stb_image 替代 cv::imread |
| `invalid rkllm model` | 模型文件下载中断损坏 | 重新下载并校验大小 |
| `error while loading shared libraries` | LD_LIBRARY_PATH 顺序不对 | 运行时库路径置于最前 |

## 📁 项目结构

```
EdgeVL/
├── VLM_NPU_stb            # 编译产物（ARM64 可执行）
├── CMakeLists.txt         # CMake 构建文件
├── build.sh               # 交叉编译脚本
├── run_board.sh           # 板端一键运行脚本
├── src/
│   ├── main.cpp           # 入口（stb_image 读图 + 交互循环）
│   ├── RK35llm.cpp        # 模型推理核心逻辑
│   └── capture.cpp        # 摄像头采集
├── include/
│   ├── RK35llm.h          # 模型封装类
│   ├── rkllm.h            # RKLLM API 头文件
│   └── stb_image.h        # 跨平台图片读取（header-only）
├── lib/                   # 板端运行时 .so 库
├── models/                # 模型文件目录（模型过大，未入仓库）
├── images/                # 测试图片
└── docs/                  # 开发文档
```

> 模型文件（约 4.9GB）未纳入仓库，请参考 `docs/` 中的下载说明获取。

## 📄 许可证

[MIT](LICENSE)
