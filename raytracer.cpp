#include "parser.h"
#include "ppm.h"
#include <iostream>
#include <math.h>

using namespace parser;

typedef unsigned char RGB[3];

struct Ray {
    Vec3f start, dir;
};

struct Hit {
    bool hitOccur;
    int materialID;
    Vec3f intersectPoint, normal;
};

double ray_triangle_intersect(Ray& ray, Face& triangle, Scene& scene)
{
#define e (ray.start)
#define d (ray.dir)
#define a (scene.vertex_data[triangle.v0_id - 1])
#define b (scene.vertex_data[triangle.v1_id - 1])
#define c (scene.vertex_data[triangle.v2_id - 1])
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
#define c (scene.vertex_data[sphere.center_vertex_id - 1])
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

Hit ClosestHit(Ray& ray, Scene& scene)
{
    Hit ret;
    double t, tmin = __DBL_MAX__;
    ret.hitOccur = false;
    // Intersection tests
    //  Mesh intersect
    for (int meshID = 0; meshID < scene.meshes.size(); meshID++) {
        Mesh& mesh = scene.meshes[meshID];
        for (int faceID = 0; faceID < mesh.faces.size(); faceID++) {
            Face& triangle = mesh.faces[faceID];
            t = ray_triangle_intersect(ray, triangle, scene);
            if (t >= 0 && t < tmin) {
                tmin = t;
                ret.intersectPoint = ray.start + ray.dir * t;
                ret.normal = triangle.normal;
                ret.materialID = mesh.material_id;
                ret.hitOccur = true;
            }
        }
    }
    //  Triangle intersect
    for (int triangleID = 0; triangleID < scene.triangles.size(); triangleID++) {
        Face& triangle = scene.triangles[triangleID].indices;
        t = ray_triangle_intersect(ray, triangle, scene);
        if (t >= 0 && t < tmin) {
            tmin = t;
            ret.intersectPoint = ray.start + ray.dir * t;
            ret.normal = triangle.normal;
            ret.materialID = scene.triangles[triangleID].material_id;
            ret.hitOccur = true;
        }
    }
    //  Sphere intersect
    for (int sphereID = 0; sphereID < scene.spheres.size(); sphereID++) {
        Sphere& sphere = scene.spheres[sphereID];
        t = ray_sphere_intersect(ray, sphere, scene);
        if (t >= 0 && t < tmin) {
            tmin = t;
            ret.intersectPoint = ray.start + ray.dir * t;
            ret.normal = (ret.intersectPoint - scene.vertex_data[sphere.center_vertex_id - 1]).normalize();
            ret.materialID = sphere.material_id;
            ret.hitOccur = true;
        }
    }
    return ret;
}

unsigned char* Specular(Ray& ray, Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toSource, halfWay, toLight;
    toSource = (ray.start - hit.intersectPoint).normalize();
    toLight = (light.position - hit.intersectPoint);
    double dSquare = toLight.dot(toLight);
    toLight = toLight.normalize();
    halfWay = (toSource + toLight).normalize();
    unsigned char* ret = new unsigned char[3];
    ret[0] = scene.materials[hit.materialID - 1].specular.x * pow(halfWay.dot(hit.normal), scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.x / dSquare;
    ret[1] = scene.materials[hit.materialID - 1].specular.y * pow(halfWay.dot(hit.normal), scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.y / dSquare;
    ret[2] = scene.materials[hit.materialID - 1].specular.z * pow(halfWay.dot(hit.normal), scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.z / dSquare;
    return ret;
}

unsigned char* Diffuse(Ray& ray, Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toSource, toLight;
    toSource = (ray.start - hit.intersectPoint).normalize();
    toLight = (light.position - hit.intersectPoint);
    double dSquare = toLight.dot(toLight);
    toLight = toLight.normalize();
    unsigned char* ret = new unsigned char[3];
    ret[0] = scene.materials[hit.materialID - 1].diffuse.x * (toLight.dot(hit.normal)) * light.intensity.x / dSquare;
    ret[1] = scene.materials[hit.materialID - 1].diffuse.y * (toLight.dot(hit.normal)) * light.intensity.y / dSquare;
    ret[2] = scene.materials[hit.materialID - 1].diffuse.z * (toLight.dot(hit.normal)) * light.intensity.z / dSquare;

    return ret;
}

bool isShadow(Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toLight;
    Vec3f toShadow;
    Ray newRay;
    toLight = (light.position - hit.intersectPoint);
    newRay.dir = toLight;
    newRay.start = hit.intersectPoint + newRay.dir.operator*(0.0001);
    double d = toLight.dot(toLight);
    Hit hitsh = ClosestHit(newRay, scene);
    if (hitsh.hitOccur) {
        toShadow = (hitsh.intersectPoint - hit.intersectPoint);
        double ds = toShadow.dot(toShadow);
        if (ds < d)
            return true;
        else
            return false;
    } else
        return false;
}

unsigned char* CalculateColor(Ray& ray, int iterationCount, Scene& scene)
{
    Vec3f color = { 0, 0, 0 };
    unsigned char* ret = new unsigned char[3];
    ret[0] = ret[1] = ret[2] = 0;
    if (iterationCount < 0)
        return ret;
    Hit hit = ClosestHit(ray, scene);
    if (!hit.hitOccur) {
        color.x = scene.background_color.x;
        color.y = scene.background_color.y;
        color.z = scene.background_color.z;
        // Rounding and clipping
        ret[0] = MIN(round(color.x), 255);
        ret[1] = MIN(round(color.y), 255);
        ret[2] = MIN(round(color.z), 255);
        return ret;
    }
    // Ambient color
    color.x += scene.materials[hit.materialID - 1].ambient.x * scene.ambient_light.x;
    color.y += scene.materials[hit.materialID - 1].ambient.y * scene.ambient_light.y;
    color.z += scene.materials[hit.materialID - 1].ambient.z * scene.ambient_light.z;

    // Calculate shadow for all light
    for (int lightNo = 0; lightNo < scene.point_lights.size(); lightNo++) {
        PointLight& currentLight = scene.point_lights[lightNo];

        if (isShadow(hit, currentLight, scene))
            continue;

        // Diffuse and Specular if not in shadow

        unsigned char* specular = Specular(ray, hit, currentLight, scene);
        color.x += specular[0];
        color.y += specular[1];
        color.z += specular[2];
        delete[] specular;

        unsigned char* diffuse = Diffuse(ray, hit, currentLight, scene);
        color.x += diffuse[0];
        color.y += diffuse[1];
        color.z += diffuse[2];
        delete[] diffuse;
    }

    // Reflected component
    unsigned char* mirrorness;
    if (scene.materials[hit.materialID - 1].mirror.x || scene.materials[hit.materialID - 1].mirror.y || scene.materials[hit.materialID - 1].mirror.z) {
        Ray newRay, toSource;
        toSource.dir = (ray.start - hit.intersectPoint).normalize();
        newRay.dir = toSource.dir * 2 * hit.normal.dot(toSource.dir) - toSource.dir;
        newRay.start = hit.intersectPoint + newRay.dir * (scene.shadow_ray_epsilon / sqrt(newRay.dir.dot(newRay.dir)));
        mirrorness = CalculateColor(newRay, iterationCount - 1, scene);

        color.x += mirrorness[0] * scene.materials[hit.materialID - 1].mirror.x;
        color.y += mirrorness[1] * scene.materials[hit.materialID - 1].mirror.y;
        color.z += mirrorness[2] * scene.materials[hit.materialID - 1].mirror.z;
        delete[] mirrorness;
    }

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
        Ray currentRay;
        for (int i = 0; i < camera.image_height; i++) {
            for (int j = 0; j < camera.image_width; j++) {
                currentRay = Generate(camera, i, j);
                unsigned char* color = CalculateColor(currentRay, scene.max_recursion_depth, scene);
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
