#ifndef __HW1__PARSER__
#define __HW1__PARSER__

#include <math.h>
#include <string>
#include <vector>


#define MIN(a, b) (((a) > (b)) ? (b) : (a))

namespace parser {
//Notice that all the structures are as simple as possible
//so that you are not enforced to adopt any style or design.
struct Vec3f {
    float x, y, z;
    Vec3f operator+(const Vec3f& rhs)
    {
        Vec3f ret;
        ret.x = x + rhs.x;
        ret.y = y + rhs.y;
        ret.z = z + rhs.z;
        return ret;
    }
    Vec3f operator-(const Vec3f& rhs)
    {
        Vec3f ret;
        ret.x = x - rhs.x;
        ret.y = y - rhs.y;
        ret.z = z - rhs.z;
        return ret;
    }
    Vec3f operator*(double rhs)
    {
        Vec3f ret;
        ret.x = x * rhs;
        ret.y = y * rhs;
        ret.z = z * rhs;
        return ret;
    }
    Vec3f operator=(const Vec3f& rhs)
    {
        x = rhs.x;
        y = rhs.y;
        z = rhs.z;
        return *this;
    }
    Vec3f cross(Vec3f& rhs)
    {
        Vec3f ret;
        ret.x = y * rhs.z - z * rhs.y;
        ret.y = z * rhs.x - x * rhs.z;
        ret.z = x * rhs.y - y * rhs.x;
        return ret;
    }
    double dot(Vec3f& rhs)
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }
    Vec3f normalize()
    {
        double len;
        len = std::sqrt(x * x + y * y + z * z);
        Vec3f ret;
        ret.x = x / len;
        ret.y = y / len;
        ret.z = z / len;
        return ret;
    }
};

struct Vec3i {
    int x, y, z;
};

struct Vec4f {
    float x, y, z, w;
};

struct Camera {
    Vec3f position;
    Vec3f gaze;
    Vec3f up;
    Vec3f right;
    Vec3f topleft;
    Vec3f halfpixelR, halfpixelD;
    Vec4f near_plane;
    float near_distance;
    int image_width, image_height;
    std::string image_name;
};

struct PointLight {
    Vec3f position;
    Vec3f intensity;
};

struct Material {
    Vec3f ambient;
    Vec3f diffuse;
    Vec3f specular;
    Vec3f mirror;
    float phong_exponent;
};

struct Face {
    int v0_id;
    int v1_id;
    int v2_id;
    Vec3f normal;
};

struct Mesh {
    int material_id;
    std::vector<Face> faces;
};

struct Triangle {
    int material_id;
    Face indices;
};

struct Sphere {
    int material_id;
    int center_vertex_id;
    float radius;
};

struct Scene {
    //Data
    Vec3i background_color;
    float shadow_ray_epsilon;
    int max_recursion_depth;
    std::vector<Camera> cameras;
    Vec3f ambient_light;
    std::vector<PointLight> point_lights;
    std::vector<Material> materials;
    std::vector<Vec3f> vertex_data;
    std::vector<Mesh> meshes;
    std::vector<Triangle> triangles;
    std::vector<Sphere> spheres;

    //Functions
    void loadFromXml(const std::string& filepath);
};
}

#endif