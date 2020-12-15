#ifndef __HW1__PARSER__
#define __HW1__PARSER__

#include "jpeg.h"
#include <algorithm>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <string>
#include <vector>

#define MIN(a, b) (((a) > (b)) ? (b) : (a))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ROUND(a) ((((a) - ((int)(a))) > 0.5) ? (((int)(a)) + 1) : ((int)(a)))

#define NEAREST 0 // for interpolation
#define BILINEAR 1
#define REPLACE_KD 2 // for colormode
#define BLEND_KD 3
#define REPLACE_ALL 4
#define NOTEXTURE 5
#define REPEAT 6 // for repeatmode
#define CLAMP 7
#define MESHHIT 8 // for hitType
#define TRIANGLEHIT 9
#define SPHEREHIT 10

#define __DBL_MAX__ double(1.79769313486231570814527423731704357e+308L)

namespace parser {
//Notice that all the structures are as simple as possible
//so that you are not enforced to adopt any style or design.

struct Vec2f {
    double x, y;
};
struct Vec3f {
    double x, y, z;
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
    double x, y, z, w;
};

struct matrix
{
    double translator[4][4];

    matrix() {
        int i, j;
        for (i = 0;i < 4;i++) {
            for (j = 0;j < 4;j++)
                translator[i][j] = 0.0;
        }
    }
    void Print() {
        int i, j;
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 4; j++) {
                std::cout << translator[i][j] << ' ';
            }
            std::cout << std::endl;
        }
    }

    Vec4f MulwithVec(Vec4f& rhs) {
        Vec4f vec;
        int i = 0, j;

        vec.x += translator[0][0] * rhs.x;
        vec.x += translator[0][1] * rhs.y;
        vec.x += translator[0][2] * rhs.z;
        vec.x += translator[0][3] * rhs.w;

        vec.y += translator[1][0] * rhs.x;
        vec.y += translator[1][1] * rhs.y;
        vec.y += translator[1][2] * rhs.z;
        vec.y += translator[1][3] * rhs.w;

        vec.z += translator[2][0] * rhs.x;
        vec.z += translator[2][1] * rhs.y;
        vec.z += translator[2][2] * rhs.z;
        vec.z += translator[2][3] * rhs.w;

        vec.w += 1;

        return vec;
    }

    matrix MulwihMatrix(matrix factor) {
        int i, j, k;
        matrix combine;
        for (i = 0;i < 4;i++) {
            for (j = 0;j < 4;j++) {
                for (k = 0;k < 4;k++) {
                    combine.translator[i][j] += factor.translator[i][k] * translator[k][j];
                }
            }
        }
        return combine;
    }
    matrix Transpose() {
        matrix trans;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                trans.translator[i][j] = translator[j][i];
            }
        }
        return trans;
    }

    void Put(int i, int j, double val) {

        translator[i][j] = val;

    }

    void MakeIdentity() {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                translator[i][j] = i == j ? 1. : .0;
            }
        }
    }
};


struct Vertex {
    Vec3f coordinates;
    double u, v;

    Vec3f operator-(const Vertex& rhs)
    {
        return coordinates - rhs.coordinates;
    }
    Vertex operator=(const Vertex& rhs)
    {
        coordinates = rhs.coordinates;
        u = rhs.u;
        v = rhs.v;
        return *this;
    }
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
    Vertex v0;
    Vertex v1;
    Vertex v2;
    Vec3f normal;
    double max[3];
    double min[3];
};

struct Box {
    Vec3f min, max;
    Box *left, *right;
    int leftindex, rigthindex;
};

struct Mesh {
    int material_id;
    int texture_id;
    std::vector<Face> faces;
    Box* head;
};

struct Triangle {
    int material_id;
    int texture_id;
    Face indices;
};

struct Sphere {
    int material_id;
    int texture_id;
    Vec3f center_vertex;
    Vec3f u, v, w;
    float radius;
};

struct Texture {
    int interpolation;
    int colormode;
    int repeatmode;
    //another element for image itself but do not know its format for now
    unsigned char* image;
    int width, height;
    // texture data is cleared during pushback so delete it at the end of the program
    /*~Texture()
    {
        delete[] image;
    }*/
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
    std::vector<Vertex> vertex_data;
    std::vector<Vec3f> translation;
    std::vector<Vec3f> scaling;
    std::vector<Vec4f> rotation;
    std::vector<Mesh> meshes;
    std::vector<Triangle> triangles;
    std::vector<Sphere> spheres;
    std::vector<Texture> textures;

    //Functions
    void loadFromXml(const std::string& filepath);
};
}

#endif