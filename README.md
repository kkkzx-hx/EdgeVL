# Qwen_vl 开发文档

> 🎯 目标：在 RK3588 开发板上部署 Qwen2.5-VL-3B 多模态大模型，实现"图片+提问→AI回答"

---

## 一、环境概览

| 组件 | 详情 |
|------|------|
| **板子芯片** | RK3588 (Rockchip) |
| **板子系统** | Buildroot 2024.02 (无包管理器) |
| **板子IP** | 192.168.1.x (动态，`arp -a` 查) |
| **板子登录** | root / <密码> |
| **NPU驱动** | rknpu 0.9.8 |
| **运行时** | rkllm-runtime 1.2.3 |
| **模型** | Qwen2.5-VL-3B (W8A8量化 + FP16视觉) |

| 组件 | 详情 |
|------|------|
| **电脑CPU** | Intel i7-13650HX |
| **电脑RAM** | 16GB |
| **WSL** | Ubuntu 24.04 (WSL2) |
| **交叉编译器** | `aarch64-linux-gnu-g++` (Ubuntu apt) |
| **工具链** | buildroot-gcc13.3 (位于 `/home/lhx/toolchains/`) |

---

## 二、文件清单

### 项目位置
```
\\wsl.localhost\Ubuntu-24.04\home\lhx\rk3588_nfs\qwen_vl\
```
放在挂载的目录下面

### 项目结构
```
qwen_vl/
├── VLM_NPU_stb           # ← 最终可用二进制 (195KB, ARM64)
├── CMakeLists.txt         # CMake 构建文件
├── build.sh               # 交叉编译脚本
├── run_board.sh           # 板端一键运行脚本
├── src/
│   ├── main.cpp           # 入口 (stb_image读图 + 交互循环)
│   └── RK35llm.cpp        # 模型推理核心逻辑
├── include/
│   ├── RK35llm.h          # 模型封装类
│   ├── rkllm.h            # RKLLM API 头文件
│   ├── rknn_api.h         # RKNN API 头文件
│   ├── rknn_custom_op.h
│   ├── rknn_matmul_api.h
│   └── stb_image.h        # 跨平台图片读取 (无外部依赖)
├── lib/                   # 运行时 .so 库 (板端加载用)
│   ├── librkllmrt.so      # RKLLM 运行时
│   ├── librknnrt.so       # RKNN 运行时
│   ├── libopencv_*.so     # OpenCV 核心/图像处理
│   ├── libgomp.so.1       # OpenMP (板子 /usr/lib 缺失)
│   └── libpng16.so.16, libz.so.1, etc.
├── models/                # 模型文件 (仅NFS备份，实际从板子本地加载)
├── images/                # 测试图片
├── cmake/
│   └── toolchain-buildroot.cmake
└── 3rdparty/
    └── opencv/include/    # OpenCV 头文件 (编译用)
```

### 板子本地文件
```
/root/qwen_models/
├── qwen2.5-vl-3b-instruct_w8a8_rk3588.rkllm   # 语言模型 (3.5GB)
└── qwen2.5-vl-3b_vision_rk3588.rknn            # 视觉编码器 (1.4GB)
```

### 模型下载地址 (需VPN)
- 语言模型：https://ln5.sync.com/dl/4450f65a0#5xun8r2j-qeg5z6aw-k33ph6s7-eb7dgfbv
- 视觉模型：https://ln5.sync.com/dl/68063c720#jadpjqg3-wgyca793-dfs8tv3u-qp2sgsnq

---

## 三、板端运行

> 每次板子重启后首次运行会自动挂载 NFS，无需手动操作。运行脚本里面有，还有就是，可以给一个最高权限
```
chmod 777 -R ./
```

### 一键运行

```bash
cd /mnt/qwen_vl
./run_board.sh                          # 默认 Moon.jpg
./run_board.sh /root/你的图片.jpg       # 自定义图片
```

### 手动运行

```bash
cd /mnt/qwen_vl
export LD_LIBRARY_PATH=/usr/bin/rkailib/lib:/mnt/qwen_vl/lib:/usr/lib
./VLM_NPU_stb 图片路径 视觉模型.rknn 语言模型.rkllm 512 2048
```

### 对话格式

输入问题前**必须先输入 `<image>` 标签**，模型才知道有图片：

```
<image>
描述一下这张图片
```

纯文本问题（不需要图片的）直接输入即可。

输入 `exit` 退出。

### 换图片

**方法一：重新运行（热启动，几秒就绪）**

```bash
./run_board.sh /root/另一张图片.jpg
```

**方法二：传新图片到板子**

用 Xftp 把图片拖到 `/root/` 或 `/mnt/qwen_vl/images/` 下，然后：

```bash
./run_board.sh /root/你的图片.jpg
# 或
./run_board.sh /mnt/qwen_vl/images/你的图片.jpg
```

---

## 四、编译方法（开发用）

### 前提
```bash
# WSL 中安装交叉编译器
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu libopencv-dev -y
```

### 编译命令
```bash
cd /home/lhx/rk3588_nfs/qwen_vl
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

---

## 五、大文件传输方案

RK3588 板子 NFS/SCP 传输大文件(>2GB)会断开，解决方案：

### 方案：拆分为 200MB 小块传输

**① WSL 端拆分**
```bash
cd /mnt/e/1
mkdir -p /home/lhx/rk3588_nfs/chunks/
split -b 200M 大文件.rkllm /home/lhx/rk3588_nfs/chunks/llm_
```

**② 板子端合并**
```bash
cat /mnt/chunks/llm_* > /root/qwen_models/大文件.rkllm
```

**③ 验证大小一致后清理**
```bash
rm -rf /mnt/chunks/
```

---

## 六、常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| `rkllm init` 卡死不响应 | NFS 读大文件不可中断等待 | 模型必须放板子本地 `/root/` |
| `Connection reset` 传文件断开 | 板子网络大流量不稳定 | 拆成 200MB 小块传输 |
| `!_src.empty()` in cvtColor | OpenCV 找不到 libjpeg | 用 stb_image 代替 cv::imread |
| `invalid rkllm model` | 模型文件损坏 | 重新下载（浏览器+VPN，勿用网盘工具） |
| `error while loading shared libraries` | LD_LIBRARY_PATH 顺序不对 | 确保 `/usr/bin/rkailib/lib` 在最前面 |
| 模型看不到图片 | 没加 `<image>` 标签 | 问题前加 `<image>` 换行 |
| NFS 挂载失败 | IP 变了或权限问题 | `arp -a` 查新 IP，`E:\1\fix_nfs.sh` 修复 |

---

## 七、关键踩坑记录

1. **模型文件必须放板子本地** — NFS 读 2GB+ 文件会进入内核不可中断等待，Ctrl+C 无效，只能硬重启
2. **旧 rkllm 文件只有 2.2GB 是坏的** — 正确大小应为 3.5GB，可能是下载中断导致
3. **LD_LIBRARY_PATH 顺序很重要** — `/usr/bin/rkailib/lib` 必须在 `/usr/lib` 之前（含正确的 librknnrt.so 和 libjpeg.so）
4. **不能用 cv::imread** — 板子 OpenCV 缺少 JPEG 解码器，改用 stb_image (header-only) 读图
5. **编译用 Ubuntu 的 `aarch64-linux-gnu-g++`** — Buildroot 工具链有路径限制且缺少 libgomp

---

## 摄像头
```
cd /user/bin/rkailib && ./run.sh
```

原始画面：rtsp://192.168.1.x:7554/live/0
AI画面： rtsp://  
