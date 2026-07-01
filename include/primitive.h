#pragma once
#include "aabb.h"
#include "material.h"
#include <cmath>
#include <limits>

struct HitRecord {
    Point3 p;
    Vec3 normal;
    double t = 0.0;
    int material_id = -1;
    bool front_face = true;

    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

struct Primitive {
    enum class Type { Sphere, Triangle };

    Type type = Type::Sphere;
    int material_id = 0;

    // Sphere
    Point3 center;
    double radius = 1.0;

    // Triangle
    Point3 v0, v1, v2;
    Vec3 tri_normal;

    static Primitive make_sphere(const Point3& c, double r, int mat) {
        Primitive p;
        p.type = Type::Sphere;
        p.center = c;
        p.radius = r;
        p.material_id = mat;
        return p;
    }

    static Primitive make_triangle(const Point3& a, const Point3& b, const Point3& c, int mat) {
        Primitive p;
        p.type = Type::Triangle;
        p.v0 = a;
        p.v1 = b;
        p.v2 = c;
        p.tri_normal = unit_vector(cross(b - a, c - a));
        p.material_id = mat;
        return p;
    }

    AABB bounding_box() const {
        if (type == Type::Sphere) {
            Vec3 r(radius, radius, radius);
            return AABB(center - r, center + r);
        }

        const double eps = 1e-5;
        Point3 small(
            std::fmin(v0.x(), std::fmin(v1.x(), v2.x())) - eps,
            std::fmin(v0.y(), std::fmin(v1.y(), v2.y())) - eps,
            std::fmin(v0.z(), std::fmin(v1.z(), v2.z())) - eps
        );
        Point3 big(
            std::fmax(v0.x(), std::fmax(v1.x(), v2.x())) + eps,
            std::fmax(v0.y(), std::fmax(v1.y(), v2.y())) + eps,
            std::fmax(v0.z(), std::fmax(v1.z(), v2.z())) + eps
        );
        return AABB(small, big);
    }

    Point3 centroid() const {
        if (type == Type::Sphere) return center;
        return (v0 + v1 + v2) / 3.0;
    }

    bool hit(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        if (type == Type::Sphere) return hit_sphere(r, t_min, t_max, rec);
        return hit_triangle(r, t_min, t_max, rec);
    }

private:
    bool hit_sphere(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        Vec3 oc = r.origin() - center;
        auto a = r.direction().length_squared();
        auto half_b = dot(oc, r.direction());
        auto c = oc.length_squared() - radius * radius;

        auto discriminant = half_b * half_b - a * c;
        if (discriminant < 0) return false;
        auto sqrtd = std::sqrt(discriminant);

        auto root = (-half_b - sqrtd) / a;
        if (root < t_min || t_max < root) {
            root = (-half_b + sqrtd) / a;
            if (root < t_min || t_max < root) return false;
        }

        rec.t = root;
        rec.p = r.at(rec.t);
        Vec3 outward_normal = (rec.p - center) / radius;
        rec.set_face_normal(r, outward_normal);
        rec.material_id = material_id;
        return true;
    }

    bool hit_triangle(const Ray& r, double t_min, double t_max, HitRecord& rec) const {
        // Moller-Trumbore intersection.
        const double eps = 1e-9;
        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        Vec3 h = cross(r.direction(), edge2);
        double a = dot(edge1, h);
        if (std::fabs(a) < eps) return false;

        double f = 1.0 / a;
        Vec3 s = r.origin() - v0;
        double u = f * dot(s, h);
        if (u < 0.0 || u > 1.0) return false;

        Vec3 q = cross(s, edge1);
        double v = f * dot(r.direction(), q);
        if (v < 0.0 || u + v > 1.0) return false;

        double t = f * dot(edge2, q);
        if (t < t_min || t > t_max) return false;

        rec.t = t;
        rec.p = r.at(t);
        rec.set_face_normal(r, tri_normal);
        rec.material_id = material_id;
        return true;
    }
};
