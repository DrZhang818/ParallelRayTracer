# ParallelRayTracer

基于 C++17 实现的 CPU 并行光线追踪渲染器。

项目实现了基础路径追踪、BVH 加速结构、OpenMP 图像块并行渲染以及简单场景文件解析，可用于光线追踪算法验证和高性能计算课程实验。

## 功能特性

- 基础路径追踪渲染流程
  - 透视相机
  - 多采样抗锯齿
  - 递归光线反弹
  - 可配置最大反弹深度
- 几何体支持
  - 球体
  - 三角形
  - 简单 OBJ 网格模型
- 材质支持
  - Lambertian 漫反射材质
  - Metal 金属材质
  - Dielectric 玻璃 / 折射材质
  - Light 自发光材质
- 加速结构
  - AABB 包围盒
  - BVH 构建
  - 迭代式 BVH 遍历
  - 支持关闭 BVH 进行暴力求交对比
- 并行计算
  - 基于 OpenMP 的图像块并行渲染
  - 支持指定线程数
  - 支持 `static`、`dynamic`、`guided` 调度策略
- 性能测试
  - 支持不同线程数对比
  - 支持 BVH / 非 BVH 对比
  - 自动生成 CSV 测试结果

## 目录结构

```text
ParallelRayTracer/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── include/              # 头文件
├── src/                  # 源代码
├── scenes/               # 示例场景与 OBJ 文件
├── scripts/              # 性能测试脚本
└── results/              # 渲染结果与测试输出
```

## 环境要求

- CMake 3.12 或更高版本
- 支持 C++17 的编译器
- OpenMP，可选但推荐开启
- Python 3，用于运行性能测试脚本

Ubuntu / Debian 环境下可使用：

```bash
sudo apt update
sudo apt install -y build-essential cmake python3
```

如需将 PPM 图片转换为 PNG，可安装 ImageMagick：

```bash
sudo apt install -y imagemagick
```

## 编译方法

在项目根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

如果系统支持 OpenMP，CMake 会自动启用多线程渲染。

如需关闭 OpenMP：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_OPENMP=OFF
cmake --build build -j
```

## 快速运行

渲染基础球体场景：

```bash
./build/raytracer \
  --scene scenes/spheres.scene \
  --output results/spheres.ppm \
  --width 800 \
  --height 450 \
  --spp 32 \
  --depth 8 \
  --threads 8 \
  --scheduler dynamic \
  --progress
```

输出文件：

```text
results/spheres.ppm
```

转换为 PNG：

```bash
convert results/spheres.ppm results/spheres.png
```

## 示例场景

### 球体场景

```bash
./build/raytracer \
  --scene scenes/spheres.scene \
  --output results/spheres.ppm \
  --width 800 \
  --height 450 \
  --spp 32 \
  --depth 8 \
  --threads 8
```

### Cornell Box 场景

```bash
./build/raytracer \
  --scene scenes/cornell.scene \
  --output results/cornell.ppm \
  --width 800 \
  --height 600 \
  --spp 64 \
  --depth 10 \
  --threads 8
```

### OBJ 网格场景

```bash
./build/raytracer \
  --scene scenes/mesh_demo.scene \
  --output results/mesh_demo.ppm \
  --width 800 \
  --height 450 \
  --spp 32 \
  --depth 8 \
  --threads 8
```

## 命令行参数

| 参数 | 说明 | 默认值 |
|---|---|---|
| `--scene <path>` | 输入场景文件 | `scenes/spheres.scene` |
| `--output <path>` | 输出 PPM 图片路径 | `results/output.ppm` |
| `--width <int>` | 图像宽度 | `800` |
| `--height <int>` | 图像高度 | `450` |
| `--spp <int>` | 每像素采样数 | `16` |
| `--depth <int>` | 最大光线反弹深度 | `8` |
| `--threads <int>` | OpenMP 线程数，`0` 表示使用运行库默认值 | `0` |
| `--tile <int>` | 图像块大小 | `16` |
| `--scheduler <name>` | OpenMP 调度策略：`static`、`dynamic`、`guided` | `dynamic` |
| `--leaf-size <int>` | BVH 叶子节点最大图元数 | `4` |
| `--no-bvh` | 关闭 BVH，使用暴力求交 | 默认启用 BVH |
| `--seed <int>` | 随机种子 | `20260701` |
| `--progress` | 显示渲染进度 | 默认关闭 |
| `--help` | 显示帮助信息 | - |

## 功能验证

| 功能 | 验证方式 |
|---|---|
| 球体、金属、玻璃、自发光材质 | 运行 `scenes/spheres.scene` |
| 三角形、面光源、Cornell Box | 运行 `scenes/cornell.scene` |
| OBJ 网格模型 | 运行 `scenes/mesh_demo.scene` |
| 多采样抗锯齿 | 对比 `--spp 1` 与 `--spp 64` 的输出图像 |
| 最大反弹深度 | 对比 `--depth 1` 与 `--depth 10` 的输出图像 |
| BVH 加速 | 对比默认运行与 `--no-bvh` 的 `render_seconds` |
| OpenMP 并行 | 对比 `--threads 1,2,4,8` 的 `render_seconds` |
| OpenMP 调度策略 | 对比 `--scheduler static/dynamic/guided` 的运行时间 |

## 性能测试

运行性能测试脚本：

```bash
python3 scripts/benchmark.py \
  --exe build/raytracer \
  --scene scenes/many_spheres.scene \
  --width 400 \
  --height 225 \
  --spp 4 \
  --depth 4 \
  --threads 1,2,4,8 \
  --schedulers dynamic
```

测试结果会输出到：

```text
results/bench/benchmark.csv
```

CSV 文件中包含以下信息：

- 线程数
- 是否启用 BVH
- OpenMP 调度策略
- 图像分辨率
- 每像素采样数
- 最大反弹深度
- 图元数量
- BVH 构建时间
- 渲染时间
- 输出文件路径

## BVH 对比实验

启用 BVH：

```bash
./build/raytracer \
  --scene scenes/many_spheres.scene \
  --output results/bvh.ppm \
  --width 400 \
  --height 225 \
  --spp 4 \
  --depth 4 \
  --threads 8
```

关闭 BVH：

```bash
./build/raytracer \
  --scene scenes/many_spheres.scene \
  --output results/no_bvh.ppm \
  --width 400 \
  --height 225 \
  --spp 4 \
  --depth 4 \
  --threads 8 \
  --no-bvh
```

通过比较两次运行的 `render_seconds`，可以分析 BVH 加速结构对求交性能的影响。

## 场景文件格式

场景文件为纯文本格式。

### 相机

```text
camera lookfrom_x lookfrom_y lookfrom_z  lookat_x lookat_y lookat_z  vup_x vup_y vup_z  vfov aperture focus_dist
```

### 背景颜色

```text
background r g b
```

### 材质

```text
material name lambertian r g b
material name metal r g b fuzz
material name dielectric refractive_index
material name light r g b intensity
```

### 几何体

```text
sphere material cx cy cz radius
triangle material ax ay az bx by bz cx cy cz
obj material path scale tx ty tz
```

其中，`obj` 的路径相对于当前 `.scene` 文件所在目录。

## 实现说明

程序启动后会先解析场景文件，生成相机、材质和图元数据。渲染开始前，程序会根据场景中的图元构建 BVH 加速结构。渲染阶段，每条相机光线通过 BVH 查找最近交点，并根据命中的材质继续进行反射、折射或漫反射采样。

并行渲染采用图像块划分方式。程序将整张图像切分为多个 tile，再使用 OpenMP 将这些 tile 分配给不同线程计算。由于不同像素之间不存在数据依赖，这种方式可以较好地利用多核 CPU 资源。

## 输出格式

渲染结果默认保存为 ASCII PPM 格式。  
PPM 格式实现简单，不依赖额外图像库，适合课程实验和跨平台运行。

转换为 PNG 示例：

```bash
convert results/spheres.ppm results/spheres.png
```

## 许可证

本项目使用 MIT License。