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

struct BVHArgs {
    std::vector<parser::Face>& arr;
    int i;
    int j;
    int level;
    BVHArgs(std::vector<parser::Face>& arrin, int i, int j, int level)
        : arr(arrin)
    {
        this->i = i;
        this->j = j;
        this->level = level;
    }
};

parser::Box* formBVH(BVHArgs* arg)
{
    std::vector<parser::Face>& arr = arg->arr;
    int i = arg->i;
    int j = arg->j;
    int level = arg->level;
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
    std::sort(arr.begin() + i, arr.begin() + j, sortingfn);

    BVHArgs* temp;
    temp = new BVHArgs(arr, i, (i + j) / 2, level + 1);
    ret->left = formBVH(temp);
    delete temp;

    temp = new BVHArgs(arr, (i + j) / 2, j, level + 1);
    ret->right = formBVH(temp);
    delete temp;

    return ret;
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
    if (child) {
        stream << child->GetText() << std::endl;
    } else {
        stream << "0 0 0" << std::endl;
    }
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
        if (child) {
            stream << child->GetText() << std::endl;
        } else {
            stream << "0 0 0" << std::endl;
        }
        child = element->FirstChildElement("DiffuseReflectance");
        if (child) {
            stream << child->GetText() << std::endl;
        } else {
            stream << "0 0 0" << std::endl;
        }
        child = element->FirstChildElement("SpecularReflectance");
        if (child) {
            stream << child->GetText() << std::endl;
        } else {
            stream << "0 0 0" << std::endl;
        }
        child = element->FirstChildElement("MirrorReflectance");
        if (child) {
            stream << child->GetText() << std::endl;
        } else {
            stream << "0 0 0" << std::endl;
        }
        child = element->FirstChildElement("PhongExponent");
        if (child) {
            stream << child->GetText() << std::endl;
        } else {
            stream << "0" << std::endl;
        }

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
    Vertex vertex;
    while (!(stream >> vertex.coordinates.x).eof()) {
        stream >> vertex.coordinates.y >> vertex.coordinates.z;
        vertex.u = vertex.v = -1;
        vertex_data.push_back(vertex);
    }
    stream.clear();

    // Get transformations data
    element = root->FirstChildElement("Transformations");
    if (element) {
        //Get Translations
        child = element->FirstChildElement("Translation");
        Vec3f trans;
        while (child) {
            stream << child->GetText() << std::endl;
            stream >> trans.x >> trans.y >> trans.z;

            translation.push_back(trans);
            child = child->NextSiblingElement("Translation");
        }
        //Get Scaling
        child = element->FirstChildElement("Scaling");
        Vec3f scale;
        while (child) {
            stream << child->GetText() << std::endl;
            stream >> scale.x >> scale.y >> scale.z;

            scaling.push_back(scale);
            child = child->NextSiblingElement("Scaling");
        }
        //Get Rotation
        child = element->FirstChildElement("Rotation");
        Vec4f rotate;
        while (child) {
            stream << child->GetText() << std::endl;
            stream >> rotate.x >> rotate.y >> rotate.z >> rotate.w;

            rotation.push_back(rotate);
            child = child->NextSiblingElement("Rotation");
        }
    }

    // Get texture data
    element = root->FirstChildElement("Textures");
    if (element) {
        element = element->FirstChildElement("Texture");
        Texture texture;
        std::string temp;
        while (element) {
            auto child = element->FirstChildElement("ImageName");
            stream << child->GetText() << std::endl;
            child = element->FirstChildElement("Interpolation");
            stream << child->GetText() << std::endl;
            child = element->FirstChildElement("DecalMode");
            stream << child->GetText() << std::endl;
            child = element->FirstChildElement("Appearance");
            stream << child->GetText() << std::endl;

            stream >> temp; //get image stuff TODO
            read_jpeg_header(temp.c_str(), texture.width, texture.height);
            texture.image = new unsigned char[texture.width * texture.height * 3];
            read_jpeg(temp.c_str(), texture.image, texture.width, texture.height);

            stream >> temp; //get interpolation
            if (!strcmp(temp.c_str(), "bilinear"))
                texture.interpolation = BILINEAR;
            if (!strcmp(temp.c_str(), "nearest"))
                texture.interpolation = NEAREST;

            stream >> temp; //get color mode
            if (!strcmp(temp.c_str(), "replace_kd"))
                texture.colormode = REPLACE_KD;
            if (!strcmp(temp.c_str(), "blend_kd"))
                texture.colormode = BLEND_KD;
            if (!strcmp(temp.c_str(), "replace_all"))
                texture.colormode = REPLACE_ALL;

            stream >> temp; //get repeat mode
            if (!strcmp(temp.c_str(), "repeat"))
                texture.repeatmode = REPEAT;
            if (!strcmp(temp.c_str(), "clamp"))
                texture.repeatmode = CLAMP;

            textures.push_back(texture);
            element = element->NextSiblingElement("Texture");
        }
    }

    // Get texel coordinate data
    element = root->FirstChildElement("TexCoordData");
    if (element) {
        stream << element->GetText() << std::endl;
        int index = 0;
        double u;
        while (!(stream >> u).eof()) {
            stream >> vertex_data[index].v;
            vertex_data[index++].u = u;
        }
        stream.clear();
    }

    //Get Meshes
    element = root->FirstChildElement("Objects");
    element = element->FirstChildElement("Mesh");
    Mesh mesh;
    while (element) {
        child = element->FirstChildElement("Material");
        stream << child->GetText() << std::endl;
        stream >> mesh.material_id;

        child = element->FirstChildElement("Texture");
        if (child) {
            stream << child->GetText() << std::endl;
            stream >> mesh.texture_id;
        } else {
            mesh.texture_id = -1;
        }

        child = element->FirstChildElement("Transformations");
        matrix M;
        M.MakeIdentity();
        if (child) {
            stream << child->GetText() << std::endl;
            char c;
            int id;
            while (!(stream >> c).eof()) {
                if (c == ' ')
                    continue;
                if (c == 's') {
                    stream >> id;
                    // scale
                    M = scale(scaling[id - 1].x, scaling[id - 1].y, scaling[id - 1].z) * M;
                } else if (c == 't') {
                    stream >> id;
                    M = translate(translation[id - 1].x, translation[id - 1].y, translation[id - 1].z) * M;
                    // translate
                } else if (c == 'r') {
                    stream >> id;
                    M = rotate(rotation[id - 1].x, rotation[id - 1].y, rotation[id - 1].z, rotation[id - 1].w) * M;
                    // rotation
                }
            }

            stream.clear();
        }

        child = element->FirstChildElement("Faces");
        stream << child->GetText() << std::endl;
        Face face;
        int v0, v1, v2;
        while (!(stream >> v0).eof()) {
            stream >> v1 >> v2;
            face.v0 = vertex_data[v0 - 1];
            face.v1 = vertex_data[v1 - 1];
            face.v2 = vertex_data[v2 - 1];
            face.v0.coordinates *= M;
            face.v1.coordinates *= M;
            face.v2.coordinates *= M;
            Vec3f triLine1, triLine2;
            triLine1 = face.v1 - face.v0;
            triLine2 = face.v2 - face.v1;
            face.normal = triLine1.cross(triLine2).normalize();
            face.max[0] = MAX(MAX(face.v1.coordinates.x, face.v2.coordinates.x), face.v0.coordinates.x);
            face.max[1] = MAX(MAX(face.v1.coordinates.y, face.v2.coordinates.y), face.v0.coordinates.y);
            face.max[2] = MAX(MAX(face.v1.coordinates.z, face.v2.coordinates.z), face.v0.coordinates.z);
            face.min[0] = MIN(MIN(face.v1.coordinates.x, face.v2.coordinates.x), face.v0.coordinates.x);
            face.min[1] = MIN(MIN(face.v1.coordinates.y, face.v2.coordinates.y), face.v0.coordinates.y);
            face.min[2] = MIN(MIN(face.v1.coordinates.z, face.v2.coordinates.z), face.v0.coordinates.z);
            mesh.faces.push_back(face);
        }
        stream.clear();

        BVHArgs* arg = new BVHArgs(mesh.faces, 0, mesh.faces.size(), 0);
        mesh.head = formBVH(arg);
        delete arg;

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

        child = element->FirstChildElement("Texture");
        if (child) {
            stream << child->GetText() << std::endl;
            stream >> triangle.texture_id;
        } else {
            triangle.texture_id = -1;
        }

        child = element->FirstChildElement("Indices");
        stream << child->GetText() << std::endl;
        int v0_id, v1_id, v2_id;
        stream >> v0_id >> v1_id >> v2_id;

        child = element->FirstChildElement("Transformations");
        matrix M;
        M.MakeIdentity();
        if (child) {
            stream << child->GetText() << std::endl;
            char c;
            int id;
            while (!(stream >> c).eof()) {
                if (c == ' ')
                    continue;
                if (c == 's') {
                    stream >> id;
                    // scale
                    M = scale(scaling[id - 1].x, scaling[id - 1].y, scaling[id - 1].z) * M;
                } else if (c == 't') {
                    stream >> id;
                    M = translate(translation[id - 1].x, translation[id - 1].y, translation[id - 1].z) * M;
                    // translate
                } else if (c == 'r') {
                    stream >> id;
                    M = rotate(rotation[id - 1].x, rotation[id - 1].y, rotation[id - 1].z, rotation[id - 1].w) * M;
                    // rotation
                }
            }
            stream.clear();
        }

        triangle.indices.v0 = vertex_data[v0_id - 1];
        triangle.indices.v1 = vertex_data[v1_id - 1];
        triangle.indices.v2 = vertex_data[v2_id - 1];

        triangle.indices.v0.coordinates *= M;
        triangle.indices.v1.coordinates *= M;
        triangle.indices.v2.coordinates *= M;

        Vec3f triLine1, triLine2;
        triLine1 = triangle.indices.v1 - triangle.indices.v0;
        triLine2 = triangle.indices.v2 - triangle.indices.v1;
        triangle.indices.normal = triLine1.cross(triLine2).normalize();

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

        child = element->FirstChildElement("Texture");
        if (child) {
            stream << child->GetText() << std::endl;
            stream >> sphere.texture_id;
        } else {
            sphere.texture_id = -1;
        }

        child = element->FirstChildElement("Center");
        stream << child->GetText() << std::endl;
        int centerid;
        stream >> centerid;
        sphere.center_vertex = vertex_data[centerid - 1].coordinates;

        child = element->FirstChildElement("Radius");
        stream << child->GetText() << std::endl;
        stream >> sphere.radius;

        sphere.u.x = 1; //init spheres own coordinate system
        sphere.u.y = 0; //with global coordinate system
        sphere.u.z = 0;

        sphere.v.x = 0;
        sphere.v.y = 1;
        sphere.v.z = 0;

        sphere.w.x = 0;
        sphere.w.y = 0;
        sphere.w.z = 1;

        child = element->FirstChildElement("Transformations");
        if (child) {
            stream << child->GetText() << std::endl;
            char c;
            int id;
            matrix M;
            M.MakeIdentity();
            matrix Coord;
            Coord.MakeIdentity();
            double Radius = 1;
            while (!(stream >> c).eof()) {
                if (c == ' ')
                    continue;
                if (c == 's') {
                    stream >> id;
                    // scale
                    M = scale(scaling[id - 1].x, scaling[id - 1].y, scaling[id - 1].z) * M;
                    Radius *= scaling[id - 1].x;
                } else if (c == 't') {
                    stream >> id;
                    M = translate(translation[id - 1].x, translation[id - 1].y, translation[id - 1].z) * M;
                    // translate
                } else if (c == 'r') {
                    stream >> id;
                    M = rotate(rotation[id - 1].x, rotation[id - 1].y, rotation[id - 1].z, rotation[id - 1].w) * M;
                    Coord = rotate(rotation[id - 1].x, rotation[id - 1].y, rotation[id - 1].z, rotation[id - 1].w) * Coord;
                    // rotation
                }
            }
            sphere.u *= Coord;
            sphere.v *= Coord;
            sphere.w *= Coord;

            sphere.u.normalize();
            sphere.v.normalize();
            sphere.w.normalize();

            sphere.radius *= Radius;

            sphere.center_vertex *= M;

            stream.clear();
        }

        spheres.push_back(sphere);
        element = element->NextSiblingElement("Sphere");
    }
}
