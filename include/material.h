#pragma once
#include "vec3.h"
#include <string>

struct Material {
    enum class Type { Lambertian, Metal, Dielectric, Light };

    std::string name;
    Type type = Type::Lambertian;
    Color albedo = Color(0.8, 0.8, 0.8);
    Color emission = Color(0.0, 0.0, 0.0);
    double fuzz = 0.0;
    double ref_idx = 1.5;
};
