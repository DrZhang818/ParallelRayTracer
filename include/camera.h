#pragma once
#include "ray.h"
#include <random>

class Camera {
public:
    Point3 origin;
    Point3 lower_left_corner;
    Vec3 horizontal;
    Vec3 vertical;
    Vec3 u, v, w;
    double lens_radius = 0.0;

    Camera() {
        configure(Point3(0, 0, 0), Point3(0, 0, -1), Vec3(0, 1, 0), 40.0, 16.0 / 9.0, 0.0, 1.0);
    }

    Camera(Point3 lookfrom, Point3 lookat, Vec3 vup, double vfov, double aspect_ratio,
           double aperture, double focus_dist) {
        configure(lookfrom, lookat, vup, vfov, aspect_ratio, aperture, focus_dist);
    }

    void configure(Point3 lookfrom, Point3 lookat, Vec3 vup, double vfov, double aspect_ratio,
                   double aperture, double focus_dist) {
        auto theta = degrees_to_radians(vfov);
        auto h = std::tan(theta / 2);
        auto viewport_height = 2.0 * h;
        auto viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        origin = lookfrom;
        horizontal = focus_dist * viewport_width * u;
        vertical = focus_dist * viewport_height * v;
        lower_left_corner = origin - horizontal / 2 - vertical / 2 - focus_dist * w;
        lens_radius = aperture / 2;
    }

    Ray get_ray(double s, double t, std::mt19937& rng) const {
        Vec3 rd = lens_radius * random_in_unit_disk(rng);
        Vec3 offset = u * rd.x() + v * rd.y();
        return Ray(origin + offset, lower_left_corner + s * horizontal + t * vertical - origin - offset);
    }

private:
    static double degrees_to_radians(double degrees) {
        return degrees * 3.1415926535897932385 / 180.0;
    }
};
