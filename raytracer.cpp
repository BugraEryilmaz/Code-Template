#include "parser.h"
#include "ppm.h"
#include <chrono>
#include <iostream>
#include <math.h>

using namespace parser;

typedef unsigned char RGB[3];

#define clip(a) MIN(round(a), 255)

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

    if (delta >= 0) {
        double mindis = MIN((-B - sqrt(delta)) / (2 * A), (-B + sqrt(delta)) / (2 * A));
        double maxdis = MAX((-B - sqrt(delta)) / (2 * A), (-B + sqrt(delta)) / (2 * A));
        return (mindis > 0 ? mindis : maxdis);
    }

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

double* Specular(Ray& ray, Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toSource, halfWay, toLight;
    toSource = (ray.start - hit.intersectPoint).normalize();
    toLight = (light.position - hit.intersectPoint);
    double dSquare = toLight.dot(toLight);
    toLight = toLight.normalize();
    halfWay = (toSource + toLight).normalize();
    double* ret = new double[3];
    double temp = halfWay.dot(hit.normal);
    ret[0] = scene.materials[hit.materialID - 1].specular.x * pow(temp, scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.x / dSquare;
    ret[1] = scene.materials[hit.materialID - 1].specular.y * pow(temp, scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.y / dSquare;
    ret[2] = scene.materials[hit.materialID - 1].specular.z * pow(temp, scene.materials[hit.materialID - 1].phong_exponent) * light.intensity.z / dSquare;
    return ret;
}

double* Diffuse(Ray& ray, Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toSource, toLight;
    toLight = (light.position - hit.intersectPoint);
    double dSquare = toLight.dot(toLight);
    toLight = toLight.normalize();
    double* ret = new double[3];
    double temp = MAX(toLight.dot(hit.normal), 0);
    ret[0] = scene.materials[hit.materialID - 1].diffuse.x * temp * (light.intensity.x / dSquare);
    ret[1] = scene.materials[hit.materialID - 1].diffuse.y * temp * (light.intensity.y / dSquare);
    ret[2] = scene.materials[hit.materialID - 1].diffuse.z * temp * (light.intensity.z / dSquare);

    return ret;
}

bool isShadow(Hit& hit, PointLight& light, Scene& scene)
{
    Vec3f toLight;
    Vec3f toShadow;
    Ray newRay;
    toLight = (light.position - hit.intersectPoint);
    newRay.dir = toLight;
    newRay.start = hit.intersectPoint + hit.normal * scene.shadow_ray_epsilon;
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
        color.x = clip(scene.background_color.x);
        color.y = clip(scene.background_color.y);
        color.z = clip(scene.background_color.z);
        return ret;
    }
    // Ambient color
    color.x = color.x + scene.materials[hit.materialID - 1].ambient.x * scene.ambient_light.x;
    color.y = color.y + scene.materials[hit.materialID - 1].ambient.y * scene.ambient_light.y;
    color.z = color.z + scene.materials[hit.materialID - 1].ambient.z * scene.ambient_light.z;

    // Calculate shadow for all light
    for (int lightNo = 0; lightNo < scene.point_lights.size(); lightNo++) {
        PointLight& currentLight = scene.point_lights[lightNo];

        if (isShadow(hit, currentLight, scene))
            continue;

        // Diffuse and Specular if not in shadow

        double* specular = Specular(ray, hit, currentLight, scene);
        color.x = (color.x + specular[0]);
        color.y = (color.y + specular[1]);
        color.z = (color.z + specular[2]);
        delete[] specular;

        double* diffuse = Diffuse(ray, hit, currentLight, scene);
        color.x = (color.x + diffuse[0]);
        color.y = (color.y + diffuse[1]);
        color.z = (color.z + diffuse[2]);
        delete[] diffuse;
    }

    // Reflected component
    unsigned char* mirrorness;
    if (scene.materials[hit.materialID - 1].mirror.x || scene.materials[hit.materialID - 1].mirror.y || scene.materials[hit.materialID - 1].mirror.z) {
        Ray newRay, toSource;
        toSource.dir = (ray.start - hit.intersectPoint).normalize();
        newRay.dir = hit.normal * 2 * hit.normal.dot(toSource.dir) - toSource.dir;
        newRay.start = hit.intersectPoint + hit.normal * (scene.shadow_ray_epsilon);
        mirrorness = CalculateColor(newRay, iterationCount - 1, scene);

        color.x = (color.x + mirrorness[0] * scene.materials[hit.materialID - 1].mirror.x);
        color.y = (color.y + mirrorness[1] * scene.materials[hit.materialID - 1].mirror.y);
        color.z = (color.z + mirrorness[2] * scene.materials[hit.materialID - 1].mirror.z);
        delete[] mirrorness;
    }

    // Rounding and clipping
    ret[0] = clip(color.x);
    ret[1] = clip(color.y);
    ret[2] = clip(color.z);
    return ret;
}

int main(int argc, char* argv[])
{
    // Sample usage for reading an XML scene file

    for (int inID = 1; inID < argc; inID++) {

        Scene scene;
        scene.loadFromXml(argv[inID]);

        // test values

        auto start = std::chrono::high_resolution_clock::now();

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
            delete[] image;
        }
        auto stop = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
        std::cout << argv[inID] << std::endl;
        std::cout << duration.count() << std::endl;
    }
    return 0;
}
