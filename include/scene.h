#pragma once
#include "camera.h"
#include "material.h"
#include "primitive.h"
#include <string>
#include <unordered_map>
#include <vector>

struct Scene {
    Camera camera;
    Color background = Color(0.70, 0.80, 1.00);
    std::vector<Material> materials;
    std::unordered_map<std::string, int> material_ids;
    std::vector<Primitive> primitives;

    int add_material(const Material& mat) {
        int id = static_cast<int>(materials.size());
        materials.push_back(mat);
        material_ids[mat.name] = id;
        return id;
    }

    int find_material(const std::string& name) const {
        auto it = material_ids.find(name);
        return it == material_ids.end() ? -1 : it->second;
    }

    bool hit_bruteforce(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        HitRecord temp_rec;
        bool hit_anything = false;
        auto closest_so_far = t_max;

        for (const auto& p : primitives) {
            if (p.hit(r, t_min, closest_so_far, temp_rec)) {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }
        return hit_anything;
    }
};

bool load_scene_file(const std::string& filename, double aspect_ratio, Scene& scene, std::string& error);
