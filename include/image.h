#pragma once
#include "vec3.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

struct Image {
    int width = 0;
    int height = 0;
    std::vector<Color> pixels;

    Image() = default;
    Image(int w, int h) : width(w), height(h), pixels(static_cast<size_t>(w) * h) {}

    Color& at(int x, int y) { return pixels[static_cast<size_t>(y) * width + x]; }
    const Color& at(int x, int y) const { return pixels[static_cast<size_t>(y) * width + x]; }

    bool write_ppm(const std::string& filename, int samples_per_pixel) const {
        std::ofstream out(filename, std::ios::out | std::ios::trunc);
        if (!out) return false;

        out << "P3\n" << width << ' ' << height << "\n255\n";
        const double scale = 1.0 / samples_per_pixel;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                Color c = at(x, y);
                double r = std::sqrt(scale * c.x());
                double g = std::sqrt(scale * c.y());
                double b = std::sqrt(scale * c.z());

                int ir = static_cast<int>(256 * clamp(r, 0.0, 0.999));
                int ig = static_cast<int>(256 * clamp(g, 0.0, 0.999));
                int ib = static_cast<int>(256 * clamp(b, 0.0, 0.999));
                out << ir << ' ' << ig << ' ' << ib << '\n';
            }
        }
        return true;
    }
};
