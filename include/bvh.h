#pragma once
#include "primitive.h"
#include <algorithm>
#include <limits>
#include <numeric>
#include <vector>

struct BVHNode {
    AABB box;
    int left = -1;
    int right = -1;
    int start = 0;
    int count = 0;
    bool is_leaf = false;
};

class BVH {
public:
    void build(const std::vector<Primitive>& primitives, int leaf_size = 4) {
        prims = &primitives;
        indices.resize(primitives.size());
        std::iota(indices.begin(), indices.end(), 0);
        nodes.clear();
        root = -1;
        this->leaf_size = std::max(1, leaf_size);
        if (!primitives.empty()) root = build_recursive(0, static_cast<int>(indices.size()));
    }

    bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        if (root < 0) return false;

        bool hit_anything = false;
        double closest_so_far = t_max;
        HitRecord temp_rec;

        // Iterative traversal avoids recursive overhead in the hot path.
        int stack[128];
        int stack_size = 0;
        stack[stack_size++] = root;

        while (stack_size > 0) {
            int node_id = stack[--stack_size];
            const BVHNode& node = nodes[node_id];
            if (!node.box.hit(r, t_min, closest_so_far)) continue;

            if (node.is_leaf) {
                for (int i = 0; i < node.count; ++i) {
                    const Primitive& p = (*prims)[indices[node.start + i]];
                    if (p.hit(r, t_min, closest_so_far, temp_rec)) {
                        hit_anything = true;
                        closest_so_far = temp_rec.t;
                        rec = temp_rec;
                    }
                }
            } else {
                if (node.left >= 0 && stack_size < 128) stack[stack_size++] = node.left;
                if (node.right >= 0 && stack_size < 128) stack[stack_size++] = node.right;
            }
        }

        return hit_anything;
    }

    size_t node_count() const { return nodes.size(); }
    size_t primitive_count() const { return prims ? prims->size() : 0; }

private:
    const std::vector<Primitive>* prims = nullptr;
    std::vector<int> indices;
    std::vector<BVHNode> nodes;
    int root = -1;
    int leaf_size = 4;

    int build_recursive(int start, int end) {
        int node_id = static_cast<int>(nodes.size());
        nodes.push_back(BVHNode{});

        AABB bounds;
        AABB centroid_bounds;
        for (int i = start; i < end; ++i) {
            const Primitive& p = (*prims)[indices[i]];
            bounds = surrounding_box(bounds, p.bounding_box());
            Point3 c = p.centroid();
            centroid_bounds = surrounding_box(centroid_bounds, AABB(c, c));
        }

        int count = end - start;
        nodes[node_id].box = bounds;
        nodes[node_id].start = start;
        nodes[node_id].count = count;

        if (count <= leaf_size) {
            nodes[node_id].is_leaf = true;
            return node_id;
        }

        Vec3 extent = centroid_bounds.extent();
        int axis = 0;
        if (extent.y() > extent.x() && extent.y() > extent.z()) axis = 1;
        else if (extent.z() > extent.x()) axis = 2;

        int mid = start + count / 2;
        std::nth_element(indices.begin() + start, indices.begin() + mid, indices.begin() + end,
            [&](int a, int b) {
                return (*prims)[a].centroid()[axis] < (*prims)[b].centroid()[axis];
            });

        int left = build_recursive(start, mid);
        int right = build_recursive(mid, end);
        nodes[node_id].left = left;
        nodes[node_id].right = right;
        nodes[node_id].is_leaf = false;
        return node_id;
    }
};
