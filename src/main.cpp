#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "renderer.h"
#include "scene.h"

namespace {

void print_help(const char* argv0) {
    std::cout << R"(ParallelRayTracer - C++17/OpenMP/BVH 并行光线追踪渲染器

用法:
  )" << argv0 << R"( [options]

常用参数:
  --scene <path>       场景文件，默认 scenes/spheres.scene
  --output <path>      输出 PPM 图片，默认 results/output.ppm
  --width <int>        图像宽度，默认 800
  --height <int>       图像高度，默认 450
  --spp <int>          每像素采样数，默认 16
  --depth <int>        最大反弹深度，默认 8
  --threads <int>      OpenMP 线程数，0 表示运行库默认，默认 0
  --tile <int>         tile 大小，默认 16
  --scheduler <name>   OpenMP 调度策略: static/dynamic/guided，默认 dynamic
  --leaf-size <int>    BVH 叶子节点最大图元数，默认 4
  --no-bvh             关闭 BVH，使用暴力求交
  --seed <int>         随机种子，默认 20260701
  --progress           显示渲染进度
  --help               显示帮助

示例:
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build -j
  ./build/raytracer --scene scenes/spheres.scene --output results/spheres.ppm --width 800 --height 450 --spp 32 --threads 8
  ./build/raytracer --scene scenes/many_spheres.scene --output results/no_bvh.ppm --no-bvh --spp 4 --threads 1
)";
}

bool require_value(int i, int argc, const std::string& opt) {
    if (i + 1 >= argc) {
        std::cerr << "参数 " << opt << " 缺少取值\n";
        return false;
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::string scene_path = "scenes/spheres.scene";
    std::string output_path = "results/output.ppm";
    RenderSettings settings;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            return 0;
        } else if (arg == "--scene") {
            if (!require_value(i, argc, arg)) return 1;
            scene_path = argv[++i];
        } else if (arg == "--output") {
            if (!require_value(i, argc, arg)) return 1;
            output_path = argv[++i];
        } else if (arg == "--width") {
            if (!require_value(i, argc, arg)) return 1;
            settings.width = std::atoi(argv[++i]);
        } else if (arg == "--height") {
            if (!require_value(i, argc, arg)) return 1;
            settings.height = std::atoi(argv[++i]);
        } else if (arg == "--spp") {
            if (!require_value(i, argc, arg)) return 1;
            settings.samples_per_pixel = std::atoi(argv[++i]);
        } else if (arg == "--depth") {
            if (!require_value(i, argc, arg)) return 1;
            settings.max_depth = std::atoi(argv[++i]);
        } else if (arg == "--threads") {
            if (!require_value(i, argc, arg)) return 1;
            settings.num_threads = std::atoi(argv[++i]);
        } else if (arg == "--tile") {
            if (!require_value(i, argc, arg)) return 1;
            settings.tile_size = std::atoi(argv[++i]);
        } else if (arg == "--scheduler") {
            if (!require_value(i, argc, arg)) return 1;
            settings.scheduler = argv[++i];
        } else if (arg == "--leaf-size") {
            if (!require_value(i, argc, arg)) return 1;
            settings.bvh_leaf_size = std::atoi(argv[++i]);
        } else if (arg == "--no-bvh") {
            settings.use_bvh = false;
        } else if (arg == "--seed") {
            if (!require_value(i, argc, arg)) return 1;
            settings.seed = static_cast<std::uint64_t>(std::strtoull(argv[++i], nullptr, 10));
        } else if (arg == "--progress") {
            settings.show_progress = true;
        } else {
            std::cerr << "未知参数: " << arg << "\n";
            print_help(argv[0]);
            return 1;
        }
    }

    if (settings.width <= 1 || settings.height <= 1 || settings.samples_per_pixel <= 0 ||
        settings.max_depth <= 0) {
        std::cerr << "width/height/spp/depth 参数非法\n";
        return 1;
    }

    Scene scene;
    std::string error;
    double aspect_ratio = static_cast<double>(settings.width) / settings.height;
    if (!load_scene_file(scene_path, aspect_ratio, scene, error)) {
        std::cerr << "加载场景失败: " << error << "\n";
        return 1;
    }

    RenderStats stats;
    Image image = render_scene(scene, settings, stats);
    if (!image.write_ppm(output_path, settings.samples_per_pixel)) {
        std::cerr << "写入输出文件失败: " << output_path << "\n";
        return 1;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "output=" << output_path << "\n";
    std::cout << "resolution=" << settings.width << "x" << settings.height << "\n";
    std::cout << "spp=" << settings.samples_per_pixel << " depth=" << settings.max_depth << "\n";
    std::cout << "threads=" << stats.actual_threads << " scheduler=" << settings.scheduler
              << " tile=" << settings.tile_size << "\n";
    std::cout << "primitives=" << stats.primitive_count
              << " use_bvh=" << (settings.use_bvh ? "yes" : "no")
              << " bvh_nodes=" << stats.bvh_node_count << "\n";
    std::cout << "bvh_build_seconds=" << stats.bvh_build_seconds << "\n";
    std::cout << "render_seconds=" << stats.render_seconds << "\n";
    return 0;
}
