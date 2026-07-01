#include "renderer.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#ifdef PRT_WITH_OPENMP
#include <omp.h>
#endif

namespace {

struct Tile {
    int x0, y0, x1, y1;
};

std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

std::uint32_t pixel_seed(const RenderSettings& settings, int x, int y) {
    std::uint64_t v = settings.seed;
    v ^= static_cast<std::uint64_t>(x + 1) * 0x9e3779b185ebca87ULL;
    v ^= static_cast<std::uint64_t>(y + 1) * 0xc2b2ae3d27d4eb4fULL;
    return static_cast<std::uint32_t>(splitmix64(v));
}

bool world_hit(const Scene& scene, const BVH& bvh, bool use_bvh, const Ray& r,
               double t_min, double t_max, HitRecord& rec) {
    if (use_bvh) return bvh.hit(r, t_min, t_max, rec);
    return scene.hit_bruteforce(r, t_min, t_max, rec);
}

double reflectance(double cosine, double ref_idx) {
    // Schlick approximation.
    auto r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * std::pow((1 - cosine), 5);
}

bool scatter(const Material& mat, const Ray& r_in, const HitRecord& rec,
             Color& attenuation, Ray& scattered, std::mt19937& rng) {
    if (mat.type == Material::Type::Light) return false;

    if (mat.type == Material::Type::Lambertian) {
        auto scatter_direction = rec.normal + random_unit_vector(rng);
        if (scatter_direction.near_zero()) scatter_direction = rec.normal;
        scattered = Ray(rec.p, scatter_direction);
        attenuation = mat.albedo;
        return true;
    }

    if (mat.type == Material::Type::Metal) {
        Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
        scattered = Ray(rec.p, reflected + mat.fuzz * random_in_unit_sphere(rng));
        attenuation = mat.albedo;
        return dot(scattered.direction(), rec.normal) > 0;
    }

    if (mat.type == Material::Type::Dielectric) {
        attenuation = Color(1.0, 1.0, 1.0);
        double refraction_ratio = rec.front_face ? (1.0 / mat.ref_idx) : mat.ref_idx;

        Vec3 unit_direction = unit_vector(r_in.direction());
        double cos_theta = std::fmin(dot(-unit_direction, rec.normal), 1.0);
        double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        Vec3 direction;
        if (cannot_refract || reflectance(cos_theta, refraction_ratio) > random_double(rng)) {
            direction = reflect(unit_direction, rec.normal);
        } else {
            direction = refract(unit_direction, rec.normal, refraction_ratio);
        }
        scattered = Ray(rec.p, direction);
        return true;
    }

    return false;
}

Color ray_color(const Ray& r, const Scene& scene, const BVH& bvh, bool use_bvh,
                int depth, std::mt19937& rng) {
    if (depth <= 0) return Color(0, 0, 0);

    HitRecord rec;
    if (!world_hit(scene, bvh, use_bvh, r, 0.001, std::numeric_limits<double>::infinity(), rec)) {
        if (scene.background.length_squared() > 0.0) {
            Vec3 unit_direction = unit_vector(r.direction());
            auto t = 0.5 * (unit_direction.y() + 1.0);
            return (1.0 - t) * Color(1.0, 1.0, 1.0) + t * scene.background;
        }
        return Color(0, 0, 0);
    }

    const Material& mat = scene.materials[rec.material_id];
    Color emitted = mat.emission;
    Ray scattered;
    Color attenuation;
    if (!scatter(mat, r, rec, attenuation, scattered, rng)) return emitted;
    return emitted + attenuation * ray_color(scattered, scene, bvh, use_bvh, depth - 1, rng);
}

std::vector<Tile> make_tiles(int width, int height, int tile_size) {
    std::vector<Tile> tiles;
    tile_size = std::max(1, tile_size);
    for (int y = 0; y < height; y += tile_size) {
        for (int x = 0; x < width; x += tile_size) {
            tiles.push_back(Tile{x, y, std::min(x + tile_size, width), std::min(y + tile_size, height)});
        }
    }
    return tiles;
}

void configure_openmp(const RenderSettings& settings, int& actual_threads) {
#ifdef PRT_WITH_OPENMP
    if (settings.num_threads > 0) omp_set_num_threads(settings.num_threads);

    omp_sched_t sched = omp_sched_dynamic;
    if (settings.scheduler == "static") sched = omp_sched_static;
    else if (settings.scheduler == "guided") sched = omp_sched_guided;
    else sched = omp_sched_dynamic;
    omp_set_schedule(sched, 1);

    #pragma omp parallel
    {
        #pragma omp single
        actual_threads = omp_get_num_threads();
    }
#else
    (void)settings;
    actual_threads = 1;
#endif
}

} // namespace

Image render_scene(const Scene& scene, const RenderSettings& settings, RenderStats& stats) {
    stats = RenderStats{};
    stats.primitive_count = scene.primitives.size();

    BVH bvh;
    auto bvh_start = std::chrono::steady_clock::now();
    if (settings.use_bvh) bvh.build(scene.primitives, settings.bvh_leaf_size);
    auto bvh_end = std::chrono::steady_clock::now();
    stats.bvh_build_seconds = std::chrono::duration<double>(bvh_end - bvh_start).count();
    stats.bvh_node_count = settings.use_bvh ? bvh.node_count() : 0;

    Image image(settings.width, settings.height);
    std::vector<Tile> tiles = make_tiles(settings.width, settings.height, settings.tile_size);
    std::atomic<int> done_tiles{0};
    configure_openmp(settings, stats.actual_threads);

    auto render_start = std::chrono::steady_clock::now();

#ifdef PRT_WITH_OPENMP
    #pragma omp parallel for schedule(runtime)
#endif
    for (int ti = 0; ti < static_cast<int>(tiles.size()); ++ti) {
        const Tile& tile = tiles[ti];
        for (int y = tile.y0; y < tile.y1; ++y) {
            for (int x = tile.x0; x < tile.x1; ++x) {
                std::mt19937 rng(pixel_seed(settings, x, y));
                Color pixel_color(0, 0, 0);
                for (int s = 0; s < settings.samples_per_pixel; ++s) {
                    auto u = (x + random_double(rng)) / (settings.width - 1);
                    auto v = (settings.height - 1 - y + random_double(rng)) / (settings.height - 1);
                    Ray r = scene.camera.get_ray(u, v, rng);
                    pixel_color += ray_color(r, scene, bvh, settings.use_bvh, settings.max_depth, rng);
                }
                image.at(x, y) = pixel_color;
            }
        }

        if (settings.show_progress) {
            int current = ++done_tiles;
            if (current % 10 == 0 || current == static_cast<int>(tiles.size())) {
#ifdef PRT_WITH_OPENMP
                #pragma omp critical
#endif
                std::cerr << "\rprogress: " << current << "/" << tiles.size() << " tiles" << std::flush;
            }
        }
    }

    if (settings.show_progress) std::cerr << "\n";

    auto render_end = std::chrono::steady_clock::now();
    stats.render_seconds = std::chrono::duration<double>(render_end - render_start).count();
    return image;
}
