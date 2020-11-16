#include "parser.h"
#include "tinyxml2.h"
#include <sstream>
#include <stdexcept>

bool mysortx(parser::Face& first, parser::Face& second)
{
    return first.max[0] < second.max[0];
}
bool mysorty(parser::Face& first, parser::Face& second)
{
    return first.max[1] < second.max[1];
}
bool mysortz(parser::Face& first, parser::Face& second)
{
    return first.max[2] < second.max[2];
}

parser::Box* formBVH(std::vector<parser::Face>& arr, int i, int j, int level)
{
    parser::Box* ret = new parser::Box;
    bool (*sortingfn)(parser::Face&, parser::Face&);
    if (level % 3 == 0)
        sortingfn = &mysortx;
    else if (level % 3 == 1)
        sortingfn = &mysorty;
    else
        sortingfn = &mysortz;
    ret->left = ret->right = NULL;
    ret->leftindex = i;
    ret->rigthindex = j;
    ret->max.x = 0;
    ret->max.y = 0;
    ret->max.z = 0;
    ret->min.x = __DBL_MAX__;
    ret->min.y = __DBL_MAX__;
    ret->min.z = __DBL_MAX__;
    for (int index = i; index < j; index++) {
        ret->max.x = MAX(ret->max.x, arr[index].max[0]);
        ret->max.y = MAX(ret->max.y, arr[index].max[1]);
        ret->max.z = MAX(ret->max.z, arr[index].max[2]);
        ret->min.x = MIN(ret->min.x, arr[index].min[0]);
        ret->min.y = MIN(ret->min.y, arr[index].min[1]);
        ret->min.z = MIN(ret->min.z, arr[index].min[2]);
    }
    if (j - i < 6) {
        return ret;
    }
}

void parser::Scene::loadFromXml(const std::string& filepath)
{
    tinyxml2::XMLDocument file;
    std::stringstream stream;

    auto res = file.LoadFile(filepath.c_str());
    if (res) {
        throw std::runtime_error("Error: The xml file cannot be loaded.");
    }

    auto root = file.FirstChild();
    if (!root) {
        throw std::runtime_error("Error: Root is not found.");
    }

    //Get BackgroundColor
    auto element = root->FirstChildElement("BackgroundColor");
    if (element) {
        stream << element->GetText() << std::endl;
    } else {
        stream << "0 0 0" << std::endl;
    }
    stream >> background_color.x >> background_color.y >> background_color.z;

    //Get ShadowRayEpsilon
    element = root->FirstChildElement("ShadowRayEpsilon");
    if (element) {
        stream << element->GetText() << std::endl;
    } else {
        stream << "0.001" << std::endl;
    }
    stream >> shadow_ray_epsilon;

    //Get MaxRecursionDepth
    element = root->FirstChildElement("MaxRecursionDepth");
    if (element) {
        stream << element->GetText() << std::endl;
    } else {
        stream << "0" << std::endl;
    }
    stream >> max_recursion_depth;

    //Get Cameras
    element = root->FirstChildElement("Cameras");
    element = element->FirstChildElement("Camera");
    Camera camera;
    while (element) {
        auto child = element->FirstChildElement("Position");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Gaze");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Up");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("NearPlane");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("NearDistance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("ImageResolution");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("ImageName");
        stream << child->GetText() << std::endl;

        stream >> camera.position.x >> camera.position.y >> camera.position.z;
        stream >> camera.gaze.x >> camera.gaze.y >> camera.gaze.z;
        stream >> camera.up.x >> camera.up.y >> camera.up.z;
        stream >> camera.near_plane.x >> camera.near_plane.y >> camera.near_plane.z >> camera.near_plane.w;
        stream >> camera.near_distance;
        stream >> camera.image_width >> camera.image_height;
        stream >> camera.image_name;

        // Calculate gaze up right

        camera.gaze = camera.gaze.normalize();
        camera.right = camera.gaze.cross(camera.up);
        camera.right = camera.right.normalize();
        camera.up = camera.right.cross(camera.gaze);
        camera.up = camera.up.normalize();

        // Calculate topleft

        camera.topleft = camera.position + camera.gaze * camera.near_distance + camera.up * camera.near_plane.w + camera.right * camera.near_plane.x;
        // middle = camera.position + camera.gaze * camera.near_distance;
        // topleft = middle + camera.up * t + camera.right * l;

        // Calculate half pixels

        camera.halfpixelD = camera.up * ((camera.near_plane.z - camera.near_plane.w) / (2 * camera.image_height));
        camera.halfpixelR = camera.right * ((camera.near_plane.y - camera.near_plane.x) / (2 * camera.image_width));

        cameras.push_back(camera);
        element = element->NextSiblingElement("Camera");
    }

    //Get Lights
    element = root->FirstChildElement("Lights");
    auto child = element->FirstChildElement("AmbientLight");
    stream << child->GetText() << std::endl;
    stream >> ambient_light.x >> ambient_light.y >> ambient_light.z;
    element = element->FirstChildElement("PointLight");
    PointLight point_light;
    while (element) {
        child = element->FirstChildElement("Position");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("Intensity");
        stream << child->GetText() << std::endl;

        stream >> point_light.position.x >> point_light.position.y >> point_light.position.z;
        stream >> point_light.intensity.x >> point_light.intensity.y >> point_light.intensity.z;

        point_lights.push_back(point_light);
        element = element->NextSiblingElement("PointLight");
    }

    //Get Materials
    element = root->FirstChildElement("Materials");
    element = element->FirstChildElement("Material");
    Material material;
    while (element) {
        child = element->FirstChildElement("AmbientReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("DiffuseReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("SpecularReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("MirrorReflectance");
        stream << child->GetText() << std::endl;
        child = element->FirstChildElement("PhongExponent");
        stream << child->GetText() << std::endl;

        stream >> material.ambient.x >> material.ambient.y >> material.ambient.z;
        stream >> material.diffuse.x >> material.diffuse.y >> material.diffuse.z;
        stream >> material.specular.x >> material.specular.y >> material.specular.z;
        stream >> material.mirror.x >> material.mirror.y >> material.mirror.z;
        stream >> material.phong_exponent;

        materials.push_back(material);
        element = element->NextSiblingElement("Material");
    }

    //Get VertexData
    element = root->FirstChildElement("VertexData");
    stream << element->GetText() << std::endl;
    Vec3f vertex;
    while (!(stream >> vertex.x).eof()) {
        stream >> vertex.y >> vertex.z;
        vertex_data.push_back(vertex);
    }
    stream.clear();

    //Get Meshes
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Mesh");
    Mesh mesh;
    while (element) {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> mesh.material_id;

        child = element->FirstChildElement("Faces");
        stream << child->GetText() << std::endl;
        Face face;
        while (!(stream >> face.v0_id).eof()) {
            stream >> face.v1_id >> face.v2_id;
            Vec3f triLine1, triLine2;
            triLine1 = vertex_data[face.v1_id - 1] - vertex_data[face.v0_id - 1];
            triLine2 = vertex_data[face.v2_id - 1] - vertex_data[face.v1_id - 1];
            face.normal = triLine1.cross(triLine2).normalize();
            face.max[0] = MAX(MAX(vertex_data[face.v1_id - 1].x, vertex_data[face.v2_id - 1].x), vertex_data[face.v0_id - 1].x);
            face.max[1] = MAX(MAX(vertex_data[face.v1_id - 1].y, vertex_data[face.v2_id - 1].y), vertex_data[face.v0_id - 1].y);
            face.max[2] = MAX(MAX(vertex_data[face.v1_id - 1].z, vertex_data[face.v2_id - 1].z), vertex_data[face.v0_id - 1].z);
            face.min[0] = MIN(MIN(vertex_data[face.v1_id - 1].x, vertex_data[face.v2_id - 1].x), vertex_data[face.v0_id - 1].x);
            face.min[1] = MIN(MIN(vertex_data[face.v1_id - 1].y, vertex_data[face.v2_id - 1].y), vertex_data[face.v0_id - 1].y);
            face.min[2] = MIN(MIN(vertex_data[face.v1_id - 1].z, vertex_data[face.v2_id - 1].z), vertex_data[face.v0_id - 1].z);
            mesh.faces.push_back(face);
        }
        stream.clear();
        std::sort(mesh.faces.begin(), mesh.faces.end(), mysortx);
        meshes.push_back(mesh);
        mesh.faces.clear();
        element = element->NextSiblingElement("Mesh");
    }
    stream.clear();

    //Get Triangles
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Triangle");
    Triangle triangle;
    while (element) {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> triangle.material_id;

        child = element->FirstChildElement("Indices");
        stream << child->GetText() << std::endl;
        stream >> triangle.indices.v0_id >> triangle.indices.v1_id >> triangle.indices.v2_id;

        Vec3f triLine1, triLine2;
        triLine1 = vertex_data[triangle.indices.v1_id - 1] - vertex_data[triangle.indices.v0_id - 1];
        triLine2 = vertex_data[triangle.indices.v2_id - 1] - vertex_data[triangle.indices.v1_id - 1];
        triangle.indices.normal = triLine1.cross(triLine2).normalize();
        triangle.indices.max[0] = MAX(MAX(vertex_data[triangle.indices.v1_id - 1].x, vertex_data[triangle.indices.v2_id - 1].x), vertex_data[triangle.indices.v0_id - 1].x);
        triangle.indices.max[1] = MAX(MAX(vertex_data[triangle.indices.v1_id - 1].y, vertex_data[triangle.indices.v2_id - 1].y), vertex_data[triangle.indices.v0_id - 1].y);
        triangle.indices.max[2] = MAX(MAX(vertex_data[triangle.indices.v1_id - 1].z, vertex_data[triangle.indices.v2_id - 1].z), vertex_data[triangle.indices.v0_id - 1].z);
        triangles.push_back(triangle);
        element = element->NextSiblingElement("Triangle");
    }

    //Get Spheres
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Sphere");
    Sphere sphere;
    while (element) {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> sphere.material_id;

        child = element->FirstChildElement("Center");
        stream << child->GetText() << std::endl;
        stream >> sphere.center_vertex_id;

        child = element->FirstChildElement("Radius");
        stream << child->GetText() << std::endl;
        stream >> sphere.radius;

        spheres.push_back(sphere);
        element = element->NextSiblingElement("Sphere");
    }
}
