#include "scene.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace {

std::string strip_comment(const std::string& line) {
    auto pos = line.find('#');
    return pos == std::string::npos ? line : line.substr(0, pos);
}

bool read_face_index(const std::string& token, int vertex_count, int& idx) {
    auto slash = token.find('/');
    std::string head = slash == std::string::npos ? token : token.substr(0, slash);
    if (head.empty()) return false;
    int raw = std::stoi(head);
    if (raw > 0) idx = raw - 1;
    else if (raw < 0) idx = vertex_count + raw;
    else return false;
    return idx >= 0 && idx < vertex_count;
}

bool load_obj_triangles(const std::filesystem::path& path, int material_id, double scale,
                        const Vec3& translate, std::vector<Primitive>& primitives,
                        std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "无法打开 OBJ 文件: " + path.string();
        return false;
    }

    std::vector<Point3> vertices;
    std::string line;
    int line_no = 0;

    while (std::getline(in, line)) {
        ++line_no;
        std::string clean = strip_comment(line);
        std::istringstream iss(clean);
        std::string tag;
        if (!(iss >> tag)) continue;

        if (tag == "v") {
            double x, y, z;
            if (!(iss >> x >> y >> z)) {
                error = "OBJ 顶点格式错误，行 " + std::to_string(line_no) + ": " + path.string();
                return false;
            }
            vertices.emplace_back(scale * Point3(x, y, z) + translate);
        } else if (tag == "f") {
            std::vector<int> ids;
            std::string tok;
            while (iss >> tok) {
                int idx = -1;
                if (!read_face_index(tok, static_cast<int>(vertices.size()), idx)) {
                    error = "OBJ 面索引格式错误，行 " + std::to_string(line_no) + ": " + path.string();
                    return false;
                }
                ids.push_back(idx);
            }
            if (ids.size() < 3) continue;
            // Fan triangulation supports quads and simple polygons.
            for (size_t i = 1; i + 1 < ids.size(); ++i) {
                primitives.push_back(Primitive::make_triangle(vertices[ids[0]], vertices[ids[i]], vertices[ids[i + 1]], material_id));
            }
        }
    }
    return true;
}

} // namespace

bool load_scene_file(const std::string& filename, double aspect_ratio, Scene& scene, std::string& error) {
    std::ifstream in(filename);
    if (!in) {
        error = "无法打开场景文件: " + filename;
        return false;
    }

    scene = Scene{};
    std::filesystem::path scene_path(filename);
    std::filesystem::path scene_dir = scene_path.parent_path();

    Material default_mat;
    default_mat.name = "default";
    default_mat.type = Material::Type::Lambertian;
    default_mat.albedo = Color(0.8, 0.8, 0.8);
    scene.add_material(default_mat);

    std::string line;
    int line_no = 0;
    while (std::getline(in, line)) {
        ++line_no;
        std::string clean = strip_comment(line);
        std::istringstream iss(clean);
        std::string tag;
        if (!(iss >> tag)) continue;

        try {
            if (tag == "camera") {
                double lx, ly, lz, ax, ay, az, ux, uy, uz, vfov, aperture, focus_dist;
                if (!(iss >> lx >> ly >> lz >> ax >> ay >> az >> ux >> uy >> uz >> vfov >> aperture >> focus_dist)) {
                    error = "camera 格式: camera lookfrom(3) lookat(3) vup(3) vfov aperture focus_dist，行 " + std::to_string(line_no);
                    return false;
                }
                scene.camera.configure(Point3(lx, ly, lz), Point3(ax, ay, az), Vec3(ux, uy, uz), vfov,
                                       aspect_ratio, aperture, focus_dist);
            } else if (tag == "background") {
                double r, g, b;
                if (!(iss >> r >> g >> b)) {
                    error = "background 格式: background r g b，行 " + std::to_string(line_no);
                    return false;
                }
                scene.background = Color(r, g, b);
            } else if (tag == "material") {
                std::string name, type;
                if (!(iss >> name >> type)) {
                    error = "material 格式错误，行 " + std::to_string(line_no);
                    return false;
                }
                Material mat;
                mat.name = name;
                if (type == "lambertian" || type == "diffuse") {
                    double r, g, b;
                    if (!(iss >> r >> g >> b)) {
                        error = "lambertian 材质格式: material name lambertian r g b，行 " + std::to_string(line_no);
                        return false;
                    }
                    mat.type = Material::Type::Lambertian;
                    mat.albedo = Color(r, g, b);
                } else if (type == "metal") {
                    double r, g, b, fuzz;
                    if (!(iss >> r >> g >> b >> fuzz)) {
                        error = "metal 材质格式: material name metal r g b fuzz，行 " + std::to_string(line_no);
                        return false;
                    }
                    mat.type = Material::Type::Metal;
                    mat.albedo = Color(r, g, b);
                    mat.fuzz = std::min(fuzz, 1.0);
                } else if (type == "dielectric" || type == "glass") {
                    double ir;
                    if (!(iss >> ir)) {
                        error = "dielectric 材质格式: material name dielectric refractive_index，行 " + std::to_string(line_no);
                        return false;
                    }
                    mat.type = Material::Type::Dielectric;
                    mat.ref_idx = ir;
                } else if (type == "light") {
                    double r, g, b, intensity;
                    if (!(iss >> r >> g >> b >> intensity)) {
                        error = "light 材质格式: material name light r g b intensity，行 " + std::to_string(line_no);
                        return false;
                    }
                    mat.type = Material::Type::Light;
                    mat.emission = intensity * Color(r, g, b);
                    mat.albedo = Color(0, 0, 0);
                } else {
                    error = "未知材质类型: " + type + "，行 " + std::to_string(line_no);
                    return false;
                }
                scene.add_material(mat);
            } else if (tag == "sphere") {
                std::string mat_name;
                double x, y, z, radius;
                if (!(iss >> mat_name >> x >> y >> z >> radius)) {
                    error = "sphere 格式: sphere material cx cy cz radius，行 " + std::to_string(line_no);
                    return false;
                }
                int mat_id = scene.find_material(mat_name);
                if (mat_id < 0) {
                    error = "未定义材质: " + mat_name + "，行 " + std::to_string(line_no);
                    return false;
                }
                scene.primitives.push_back(Primitive::make_sphere(Point3(x, y, z), radius, mat_id));
            } else if (tag == "triangle") {
                std::string mat_name;
                double ax, ay, az, bx, by, bz, cx, cy, cz;
                if (!(iss >> mat_name >> ax >> ay >> az >> bx >> by >> bz >> cx >> cy >> cz)) {
                    error = "triangle 格式: triangle material ax ay az bx by bz cx cy cz，行 " + std::to_string(line_no);
                    return false;
                }
                int mat_id = scene.find_material(mat_name);
                if (mat_id < 0) {
                    error = "未定义材质: " + mat_name + "，行 " + std::to_string(line_no);
                    return false;
                }
                scene.primitives.push_back(Primitive::make_triangle(Point3(ax, ay, az), Point3(bx, by, bz), Point3(cx, cy, cz), mat_id));
            } else if (tag == "obj") {
                std::string mat_name, obj_path;
                double scale = 1.0, tx = 0.0, ty = 0.0, tz = 0.0;
                if (!(iss >> mat_name >> obj_path >> scale >> tx >> ty >> tz)) {
                    error = "obj 格式: obj material path scale tx ty tz，行 " + std::to_string(line_no);
                    return false;
                }
                int mat_id = scene.find_material(mat_name);
                if (mat_id < 0) {
                    error = "未定义材质: " + mat_name + "，行 " + std::to_string(line_no);
                    return false;
                }
                std::filesystem::path full_path = std::filesystem::path(obj_path);
                if (full_path.is_relative()) full_path = scene_dir / full_path;
                if (!load_obj_triangles(full_path, mat_id, scale, Vec3(tx, ty, tz), scene.primitives, error)) return false;
            } else {
                error = "未知场景命令: " + tag + "，行 " + std::to_string(line_no);
                return false;
            }
        } catch (const std::exception& ex) {
            error = "解析异常，行 " + std::to_string(line_no) + ": " + ex.what();
            return false;
        }
    }

    if (scene.primitives.empty()) {
        error = "场景中没有任何几何体";
        return false;
    }
    return true;
}
