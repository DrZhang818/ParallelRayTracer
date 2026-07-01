# ParallelRayTracer：并行光线追踪渲染器

这是为“高性能计算导论”课程设计准备的 C++17 项目代码。当前压缩包只包含工程代码、场景、运行脚本和说明，不包含 PPT 或实验报告。

## 1. 已实现内容

- 基础路径追踪 / 光线追踪渲染流程
  - 相机模型、抗锯齿多采样、递归反弹
  - 球体求交
  - 三角形求交
  - 简单 OBJ 三角网格载入
- 材质系统
  - Lambertian 漫反射材质
  - Metal 金属反射材质
  - Dielectric 玻璃 / 折射材质
  - Light 自发光材质
- BVH 加速结构
  - AABB 包围盒
  - 基于图元中心的最长轴划分
  - 迭代式 BVH 遍历
  - 支持 `--no-bvh` 关闭 BVH 做性能对照
- OpenMP 并行渲染
  - 图像 tile 分块并行
  - 支持 `static / dynamic / guided` 调度策略
  - 支持指定线程数
- 性能测试脚本
  - 自动测试不同线程数、BVH/非 BVH 的运行时间
  - 输出 `benchmark.csv`

## 2. 目录结构

```text
ParallelRayTracer/
├── CMakeLists.txt
├── README.md
├── include/              # 头文件
├── src/                  # 源代码
├── scenes/               # 示例场景
│   ├── spheres.scene
│   ├── cornell.scene
│   ├── many_spheres.scene
│   ├── mesh_demo.scene
│   └── cube.obj
├── scripts/
│   └── benchmark.py
└── results/              # 输出目录
```

## 3. 编译环境

推荐环境：

- Linux / macOS / Windows + MinGW 或 MSVC
- CMake >= 3.12
- C++17 编译器
- OpenMP，可选但强烈建议开启

Linux 示例：

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

## 4. 编译方法

在项目根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

如果 CMake 找到 OpenMP，会自动启用多线程；如果没有找到，仍可编译串行版本。

## 5. 快速运行

渲染基础球体场景：

```bash
./build/raytracer \
  --scene scenes/spheres.scene \
  --output results/spheres.ppm \
  --width 800 --height 450 \
  --spp 32 --depth 8 \
  --threads 8 \
  --scheduler dynamic
```

渲染简化 Cornell Box：

```bash
./build/raytracer \
  --scene scenes/cornell.scene \
  --output results/cornell.ppm \
  --width 800 --height 600 \
  --spp 64 --depth 10 \
  --threads 8
```

测试 OBJ 三角网格载入：

```bash
./build/raytracer \
  --scene scenes/mesh_demo.scene \
  --output results/mesh_demo.ppm \
  --width 800 --height 450 \
  --spp 32 --depth 8 \
  --threads 8
```

输出格式是 PPM。大多数图片查看器、ImageMagick、GIMP、Photoshop 都可以打开。也可以转换成 PNG：

```bash
convert results/spheres.ppm results/spheres.png
```

## 6. 命令行参数

```text
--scene <path>       场景文件，默认 scenes/spheres.scene
--output <path>      输出 PPM 图片，默认 results/output.ppm
--width <int>        图像宽度，默认 800
--height <int>       图像高度，默认 450
--spp <int>          每像素采样数，默认 16
--depth <int>        最大反弹深度，默认 8
--threads <int>      OpenMP 线程数，0 表示运行库默认，默认 0
--tile <int>         tile 大小，默认 16
--scheduler <name>   OpenMP 调度策略：static / dynamic / guided，默认 dynamic
--leaf-size <int>    BVH 叶子节点最大图元数，默认 4
--no-bvh             关闭 BVH，使用暴力求交
--seed <int>         随机种子，默认 20260701
--progress           显示渲染进度
```

## 7. 性能实验示例

### 7.1 比较 OpenMP 线程数

```bash
python3 scripts/benchmark.py \
  --exe build/raytracer \
  --scene scenes/many_spheres.scene \
  --width 400 --height 225 \
  --spp 4 --depth 4 \
  --threads 1,2,4,8 \
  --schedulers dynamic
```

脚本会输出：

```text
results/bench/benchmark.csv
```

CSV 中包含：

- 线程数
- 是否使用 BVH
- 调度策略
- 图元数量
- BVH 构建时间
- 渲染时间

### 7.2 单独比较 BVH 和暴力求交

使用 BVH：

```bash
./build/raytracer --scene scenes/many_spheres.scene --output results/bvh.ppm --width 400 --height 225 --spp 4 --depth 4 --threads 8
```

关闭 BVH：

```bash
./build/raytracer --scene scenes/many_spheres.scene --output results/no_bvh.ppm --width 400 --height 225 --spp 4 --depth 4 --threads 8 --no-bvh
```

程序会在终端打印类似信息：

```text
output=results/bvh.ppm
resolution=400x225
spp=4 depth=4
threads=8 scheduler=dynamic tile=16
primitives=417 use_bvh=yes bvh_nodes=...
bvh_build_seconds=...
render_seconds=...
```

## 8. 场景文件格式

场景文件是纯文本，支持以下命令。

### 相机

```text
camera lookfrom_x lookfrom_y lookfrom_z  lookat_x lookat_y lookat_z  vup_x vup_y vup_z  vfov aperture focus_dist
```

### 背景

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

`obj` 的路径相对于当前 `.scene` 文件所在目录。

## 9. 后续写报告时可重点分析

虽然本压缩包未包含报告，但代码已经预留了比较实验所需的功能。后续报告可以围绕这些点展开：

1. 光线追踪中最耗时的是光线与大量图元求交。
2. BVH 使用层次包围盒减少无效求交。
3. 像素 / tile 之间相互独立，适合共享内存多线程并行。
4. `dynamic` 调度能缓解不同像素路径深度和求交次数不同造成的负载不均衡。
5. 实验指标建议使用渲染时间、加速比、并行效率、BVH 加速比。

## 10. 备注

- 项目没有依赖外部图形库，方便在课程服务器或普通 Linux 环境上编译。
- 为了降低复杂度，输出使用 ASCII PPM；后续可以自行加入 `stb_image_write` 输出 PNG。
- CUDA / MPI 版本未放入当前压缩包，避免工程复杂度过高；本版本主线是 CPU OpenMP + BVH。
