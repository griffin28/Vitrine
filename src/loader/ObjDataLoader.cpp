#include "ObjDataLoader.h"

#include <anari/anari.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include <array>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "AppUtils.h" // Vertex struct + std::hash<Vertex>

namespace vitrine
{

namespace
{

// Create a device-managed ANARI array and copy `count` elements of `elemSize`
// bytes into it. Unlike the shared (app-memory) form, this owns its storage,
// so the caller's buffers do not need to outlive the array / geometry.
ANARIArray1D makeManagedArray1D(ANARIDevice device, ANARIDataType type,
                                const void* data, size_t count, size_t elemSize)
{
    ANARIArray1D array = anariNewArray1D(device, nullptr, nullptr, nullptr, type, count);
    if (array && count > 0) {
        if (void* mapped = anariMapArray(device, array)) {
            std::memcpy(mapped, data, count * elemSize);
        }
        anariUnmapArray(device, array);
    }
    return array;
}

// 2D analogue of makeManagedArray1D: copies width*height elements of `elemSize`
// bytes into a device-managed array (used to back image samplers).
ANARIArray2D makeManagedArray2D(ANARIDevice device, ANARIDataType type,
                                const void* data, size_t width, size_t height,
                                size_t elemSize)
{
    ANARIArray2D array = anariNewArray2D(device, nullptr, nullptr, nullptr, type, width, height);
    if (array && width > 0 && height > 0) {
        if (void* mapped = anariMapArray(device, array)) {
            std::memcpy(mapped, data, width * height * elemSize);
        }
        anariUnmapArray(device, array);
    }
    return array;
}

} // namespace

bool ObjDataLoader::loadSceneFromFile(ANARIDevice device,
                                      AnariScene& scene,
                                      const QString& filePath)
{
    if (!device || !scene.world) {
        return false;
    }

    // Load OBJ File
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.toStdString().c_str())) {
        emitStatus(LogLevel::Warning,
               QStringLiteral("Failed to load OBJ '%1': %2 %3")
                   .arg(filePath, QString::fromStdString(warn), QString::fromStdString(err)));
        return false;
    }

    size_t totalVerts = 0;
    size_t totalTris = 0;

    // TODO: Remove once the OBJ loader parses materials/textures from the .mtl.
    // For now every surface is textured with the bundled Viking Room image so
    // the texture path is exercised end-to-end. Load it once and share a single
    // image2D sampler across all materials (samplers are reference-counted).
    ANARISampler sampler{nullptr};
    const QString texturePath =
        QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("textures/viking_room.png"));
    int texWidth = 0;
    int texHeight = 0;
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(texturePath.toStdString().c_str(),
                                &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (pixels) {
        sampler = anariNewSampler(device, "image2D");
        ANARIArray2D imageArray = makeManagedArray2D(
            device, ANARI_UFIXED8_VEC4, pixels,
            static_cast<size_t>(texWidth), static_cast<size_t>(texHeight), 4);
        anariSetParameter(device, sampler, "image", ANARI_ARRAY2D, &imageArray);
        anariRelease(device, imageArray);

        // Sample using the texcoords we upload as vertex.attribute0; wrap and
        // filter like the original Vulkan sampler did.
        const char* inAttribute = "attribute0";
        const char* wrapMode = "repeat";
        const char* filter = "linear";
        anariSetParameter(device, sampler, "inAttribute", ANARI_STRING, inAttribute);
        anariSetParameter(device, sampler, "wrapMode1", ANARI_STRING, wrapMode);
        anariSetParameter(device, sampler, "wrapMode2", ANARI_STRING, wrapMode);
        anariSetParameter(device, sampler, "filter", ANARI_STRING, filter);
        anariCommitParameters(device, sampler);

        // The image array data was copied into the device-managed array above,
        // so the CPU-side pixels are no longer needed.
        stbi_image_free(pixels);
    } else {
        emitStatus(LogLevel::Warning,
                   QStringLiteral("Failed to load texture '%1'; surfaces will be untextured.")
                       .arg(texturePath));
    }

    // Build a surface per shape: dedup that shape's vertices, create its own
    // triangle geometry + matte material, and gather the surface into the
    // scene.
    for (const auto& shape : shapes) {
        std::unordered_map<Vertex, uint32_t> uniqueVertices;
        std::vector<Vertex> verts;
        std::vector<uint32_t> idx;
        bool useTexCoords = false;
        bool useVertexNormals = false;

        for (const auto& index : shape.mesh.indices) {
            Vertex v{};
            // Vertex position
            v.pos = {attrib.vertices[3 * index.vertex_index + 0],
                     attrib.vertices[3 * index.vertex_index + 1],
                     attrib.vertices[3 * index.vertex_index + 2]};
            // Vertex texture coord
            if (index.texcoord_index >= 0) {
                useTexCoords = true;
                v.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                              1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            }
            // Vertex normal
            if (index.normal_index >=0) {
                useVertexNormals = true;
                v.normal = {attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]};
            }
            // Vertex color
            v.color = {1.0f, 1.0f, 1.0f};

            auto it = uniqueVertices.find(v);
            if (it == uniqueVertices.end()) {
                const uint32_t newIdx = static_cast<uint32_t>(verts.size());
                uniqueVertices.emplace(v, newIdx);
                verts.push_back(v);
                idx.push_back(newIdx);
            } else {
                idx.push_back(it->second);
            }
        }
        if (verts.empty() || idx.size() < 3) {
            continue;
        }

        // Repack into plain float[3]/uint32_t[3] buffers; these are copied into
        // device-managed arrays below, so they may die at the end of the loop.
        std::vector<std::array<float, 3>> positions;
        positions.reserve(verts.size());
        std::vector<std::array<float, 3>> colors;
        colors.reserve(verts.size());
        for (const auto& v : verts) {
            positions.push_back({v.pos.x, v.pos.y, v.pos.z});
            colors.push_back({v.color.x, v.color.y, v.color.z});
        }

        std::vector<std::array<uint32_t, 3>> indices;
        indices.reserve(idx.size() / 3);
        for (size_t i = 0; i + 2 < idx.size(); i += 3) {
            indices.push_back({idx[i], idx[i + 1], idx[i + 2]});
        }

        ANARIGeometry geometry = anariNewGeometry(device, "triangle");
        if (!geometry) {
            emitStatus(LogLevel::Warning,
                   QStringLiteral("Backend does not implement triangle geometry yet "
                                  "(anariNewGeometry returned null). Scene wiring is "
                                  "live; only clear-frame will render."));
            return false;
        }

        anariSetParameter(device, geometry, "name", ANARI_STRING, shape.name.c_str());

        ANARIArray1D positionArray = makeManagedArray1D(
            device, ANARI_FLOAT32_VEC3, positions.data(), positions.size(), sizeof(positions[0]));
        ANARIArray1D colorArray = makeManagedArray1D(
            device, ANARI_FLOAT32_VEC3, colors.data(), colors.size(), sizeof(colors[0]));
        ANARIArray1D indexArray = makeManagedArray1D(
            device, ANARI_UINT32_VEC3, indices.data(), indices.size(), sizeof(indices[0]));
        anariSetParameter(device, geometry, "vertex.position", ANARI_ARRAY1D, &positionArray);
        anariSetParameter(device, geometry, "vertex.color", ANARI_ARRAY1D, &colorArray);
        anariSetParameter(device, geometry, "primitive.index", ANARI_ARRAY1D, &indexArray);

        if (useVertexNormals)
        {
            std::vector<std::array<float, 3>> normals;
            for (const auto& v : verts) {
                normals.push_back({v.normal.x, v.normal.y, v.normal.z});
            }

            ANARIArray1D normalArray = makeManagedArray1D(
                device, ANARI_FLOAT32_VEC3, normals.data(), normals.size(), sizeof(normals[0])); 
            anariSetParameter(device, geometry, "vertex.normal", ANARI_ARRAY1D, &normalArray);
            anariRelease(device, normalArray);
        }

        if (useTexCoords)
        {
            std::vector<std::array<float, 2>> texCoords;
            for (const auto& v : verts) {
                texCoords.push_back({v.texCoord.x, v.texCoord.y});
            }

            ANARIArray1D texCoordArray = makeManagedArray1D(
                device, ANARI_FLOAT32_VEC2, texCoords.data(), texCoords.size(), sizeof(texCoords[0])); 
            anariSetParameter(device, geometry, "vertex.attribute0", ANARI_ARRAY1D, &texCoordArray);
            anariRelease(device, texCoordArray);
        }

        anariRelease(device, positionArray);
        anariRelease(device, colorArray);
        anariRelease(device, indexArray);
        anariCommitParameters(device, geometry);

        ANARIMaterial material = anariNewMaterial(device, "matte");
        if (material) {
            if (sampler) {
                anariSetParameter(device, material, "color", ANARI_SAMPLER, &sampler);
            }
            anariCommitParameters(device, material);
        }

        ANARISurface surface = anariNewSurface(device);
        if (!surface) {
            anariRelease(device, geometry);
            if (material) {
                anariRelease(device, material);
            }
            continue;
        }
        anariSetParameter(device, surface, "geometry", ANARI_GEOMETRY, &geometry);
        if (material) {
            anariSetParameter(device, surface, "material", ANARI_MATERIAL, &material);
        }
        anariCommitParameters(device, surface);

        // The surface retains its own references to geometry/material; we can
        // drop ours now and track only the surface for teardown.
        anariRelease(device, geometry);
        if (material) {
            anariRelease(device, material);
        }

        scene.surfaces.push_back(surface);
        totalVerts += verts.size();
        totalTris += indices.size();
    }

    // Each material that bound the sampler holds its own reference now, so drop
    // ours. Released after the loop so a single sampler is shared by all.
    if (sampler) {
        anariRelease(device, sampler);
    }

    if (scene.surfaces.empty()) {
        emitStatus(LogLevel::Warning,
               QStringLiteral("OBJ '%1' produced no triangles.").arg(filePath));
        // Clear the world's instance list so any prior scene is gone.
        anariUnsetParameter(device, scene.world, "instance");
        anariCommitParameters(device, scene.world);
        return false;
    }

    // Gather every surface into a single group / instance and attach to world.
    // ANARIGroup group = anariNewGroup(device);
    ANARIArray1D surfaceArray = makeManagedArray1D(
        device, ANARI_SURFACE, scene.surfaces.data(), scene.surfaces.size(),
        sizeof(scene.surfaces[0]));

    anariSetParameter(device, scene.world, "surface", ANARI_ARRAY1D, &surfaceArray);
    anariRelease(device, surfaceArray);
    anariCommitParameters(device, scene.world);
    // anariSetParameter(device, group, "surface", ANARI_ARRAY1D, &surfaceArray);
    // anariRelease(device, surfaceArray);
    // anariCommitParameters(device, group);
    // scene.groups.push_back(group);

    // ANARIInstance instance = anariNewInstance(device, "transform");
    // if (instance) {
    //     anariSetParameter(device, instance, "group", ANARI_GROUP, &group);
    //     anariCommitParameters(device, instance);
    //     scene.instances.push_back(instance);

    //     ANARIArray1D instanceArray = makeManagedArray1D(
    //         device, ANARI_INSTANCE, scene.instances.data(), scene.instances.size(),
    //         sizeof(scene.instances[0]));
    //     anariSetParameter(device, scene.world, "instance", ANARI_ARRAY1D, &instanceArray);
    //     anariRelease(device, instanceArray);
    //     anariCommitParameters(device, scene.world);
    // }

    emitStatus(LogLevel::Info,
           QStringLiteral("Loaded OBJ '%1' (%2 surfaces, %3 verts, %4 tris).")
               .arg(filePath)
               .arg(scene.surfaces.size())
               .arg(totalVerts)
               .arg(totalTris));
    return true;
}

} // namespace vitrine
