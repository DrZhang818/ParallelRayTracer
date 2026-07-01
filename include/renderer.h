#pragma once
#include "bvh.h"
#include "image.h"
#include "scene.h"
#include <cstdint>
#include <string>

struct RenderSettings {
    int width = 800;
    int height = 450;
    int samples_per_pixel = 16;
    int max_depth = 8;
    int tile_size = 16;
    int num_threads = 0;
    int bvh_leaf_size = 4;
    bool use_bvh = true;
    bool show_progress = false;
    std::string scheduler = "dynamic";
    std::uint64_t seed = 20260701;
};

struct RenderStats {
    size_t primitive_count = 0;
    size_t bvh_node_count = 0;
    double bvh_build_seconds = 0.0;
    double render_seconds = 0.0;
    int actual_threads = 1;
};

Image render_scene(const Scene& scene, const RenderSettings& settings, RenderStats& stats);
