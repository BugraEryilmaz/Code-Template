#include "parser.h"
#include "ppm.h"
#include <iostream>
#include <math.h>

using namespace parser;

typedef unsigned char RGB[3];

struct Ray {
    Vec3f start, dir;
};

double ray_triangle_intersect(Ray& ray, Face& triangle, Scene& scene)
{
#define e (ray.start)
#define d (ray.dir)
#define a (scene.vertex_data[triangle.v0_id])
#define b (scene.vertex_data[triangle.v1_id])
#define c (scene.vertex_data[triangle.v2_id])
    double det, t, beta, gamma;
    det = ((-d.x) * ((b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y)) - (-d.y) * ((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) + (-d.z) * ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)));
    t = ((e.x - a.x) * ((b.y - a.y) * (c.z - a.z) - (b.z - a.z) * (c.y - a.y)) - (e.y - a.y) * ((b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x)) + (e.z - a.z) * ((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x))) / det;
    beta = ((-d.x) * ((e.y - a.y) * (c.z - a.z) - (e.z - a.z) * (c.y - a.y)) - (-d.y) * ((e.x - a.x) * (c.z - a.z) - (e.z - a.z) * (c.x - a.x)) + (-d.z) * ((e.x - a.x) * (c.y - a.y) - (e.y - a.y) * (c.x - a.x))) / det;
    gamma = ((-d.x) * ((b.y - a.y) * (e.z - a.z) - (b.z - a.z) * (e.y - a.y)) - (-d.y) * ((b.x - a.x) * (e.z - a.z) - (b.z - a.z) * (e.x - a.x)) + (-d.z) * ((b.x - a.x) * (e.y - a.y) - (b.y - a.y) * (e.x - a.x))) / det;
#undef e
#undef d
#undef a
#undef b
#undef c

    if (beta >= 0 && gamma >= 0 && beta + gamma <= 1 && t > 0) {
        return t;
    }

    return -1;
}

double ray_sphere_intersect(Ray& ray, Sphere& sphere, Scene& scene)
{

#define r (sphere.radius)
#define c (scene.vertex_data[sphere.center_vertex_id])
#define e (ray.start)
#define d (ray.dir)

    //At^2+Bt+C=0
    double A = d.x * d.x + d.y * d.y + d.z * d.z;
    double B = 2 * ((e.x - c.x) * d.x + (e.y - c.y) * d.y + (e.z - c.z) * d.z);
    double C = (e.x - c.x) * (e.x - c.x) + (e.y - c.y) * (e.y - c.y) + (e.z - c.z) * (e.z - c.z) - r * r;

    double delta = B * B - 4 * A * C;

    if (delta >= 0 && (-B - sqrt(delta)) / (2 * A) >= 0)
        return (-B - sqrt(delta)) / (2 * A);

#undef e
#undef d
#undef r
#undef c

    return -1;
}

Ray Generate(Camera& camera, int i, int j)
{
    Vec3f current;
    Ray ret;

    current = camera.topleft + camera.halfpixelD * (2 * i + 1) + camera.halfpixelR * (2 * j + 1);

    ret.dir = current - camera.position;
    ret.start = camera.position;
    return ret;
}

unsigned char* CalculateColor(Ray& ray, Scene& scene)
{
    Vec3f color = { 0, 0, 0 };
    unsigned char* ret = new unsigned char[3];
    double tmin=__DBL_MAX__, t;
    // Intersection tests
    //  Mesh intersect
    for (int meshID=0; meshID<scene.meshes.size(); meshID++) {
        
    }
    // Ambient color

    // Calculate shadow

    // Diffuse and Specular if not in shadow

    // Reflected component

    // Rounding and clipping
    ret[0] = MIN(round(color.x), 255);
    ret[1] = MIN(round(color.y), 255);
    ret[2] = MIN(round(color.z), 255);
    return ret;
}

int main(int argc, char* argv[])
{
    // Sample usage for reading an XML scene file
    Scene scene;
    scene.loadFromXml(argv[1]);

    // The code below creates a test pattern and writes
    // it to a PPM file to demonstrate the usage of the
    // ppm_write function.
    //
    // Normally, you would be running your ray tracing
    // code here to produce the desired image.

    // test values

    for (int cam = 0; cam < scene.cameras.size(); cam++) {
        Camera& camera = scene.cameras[cam];
        unsigned char* image = new unsigned char[camera.image_width * camera.image_height * 3];
        int index = 0;
        for (int i = 0; i < camera.image_height; i++) {
            for (int j = 0; j < camera.image_width; j++) {
                Ray currentRay = Generate(camera, i, j);
                unsigned char* color = CalculateColor(currentRay, scene);
                image[index++] = color[0];
                image[index++] = color[1];
                image[index++] = color[2];
                delete[] color;
            }
        }
        write_ppm(camera.image_name.c_str(), image, camera.image_width, camera.image_height);
    }

    /*
    const RGB BAR_COLOR[8] =
    {
        { 255, 255, 255 },  // 100% White
        { 255, 255,   0 },  // Yellow
        {   0, 255, 255 },  // Cyan
        {   0, 255,   0 },  // Green
        { 255,   0, 255 },  // Magenta
        { 255,   0,   0 },  // Red
        {   0,   0, 255 },  // Blue
        {   0,   0,   0 },  // Black
    };

    int width = 640, height = 480;
    int columnWidth = width / 8;

    unsigned char* image = new unsigned char [width * height * 3];

    int i = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int colIdx = x / columnWidth;
            image[i++] = BAR_COLOR[colIdx][0];
            image[i++] = BAR_COLOR[colIdx][1];
            image[i++] = BAR_COLOR[colIdx][2];
        }
    }

    write_ppm("test.ppm", image, width, height);*/
    return 0;
}
