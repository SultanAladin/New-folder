// File: Source/Filament/Internal/SceneContext/FilamentScene.cpp

/*====================================================================================================================================
                                                    FILAMENTSCENE.CPP
====================================================================================================================================*/
// 🧩 Procedural mesh generation and default scene population.

#include "FilamentScene.h"
#include "../Algebra/Utilities.h"
#include <cmath>

namespace FilamentSceneBuilder
{

//------------------------------------------------------------------------------------------------------------------------
//                                                    CUBE
//------------------------------------------------------------------------------------------------------------------------

MeshData constructCube() noexcept
{
    MeshData mesh;

    // ⚙️ 24 vertices (4 per face for flat shading normals), 36 indices
    struct FaceInfo { Vec3 normal; Vec3 up; Vec3 right; };
    const FaceInfo faces[6] = {
        { { 0, 0, 1}, { 0, 1, 0}, { 1, 0, 0} },  // +Z front
        { { 0, 0,-1}, { 0, 1, 0}, {-1, 0, 0} },  // -Z back
        { { 1, 0, 0}, { 0, 1, 0}, { 0, 0,-1} },  // +X right
        { {-1, 0, 0}, { 0, 1, 0}, { 0, 0, 1} },  // -X left
        { { 0, 1, 0}, { 0, 0,-1}, { 1, 0, 0} },  // +Y top
        { { 0,-1, 0}, { 0, 0, 1}, { 1, 0, 0} },  // -Y bottom
    };

    mesh.vertices.reserve(24);
    mesh.indices.reserve(36);

    for (int f = 0; f < 6; ++f)
    {
        const Vec3& n = faces[f].normal;
        const Vec3& u = faces[f].up;
        const Vec3& r = faces[f].right;

        Vec3 center = n * 0.5f;

        FilamentVertex v0, v1, v2, v3;

        Vec3 p0 = center - r * 0.5f - u * 0.5f;
        Vec3 p1 = center + r * 0.5f - u * 0.5f;
        Vec3 p2 = center + r * 0.5f + u * 0.5f;
        Vec3 p3 = center - r * 0.5f + u * 0.5f;

        v0 = { {p0.x, p0.y, p0.z}, {n.x, n.y, n.z}, {0.0f, 0.0f} };
        v1 = { {p1.x, p1.y, p1.z}, {n.x, n.y, n.z}, {1.0f, 0.0f} };
        v2 = { {p2.x, p2.y, p2.z}, {n.x, n.y, n.z}, {1.0f, 1.0f} };
        v3 = { {p3.x, p3.y, p3.z}, {n.x, n.y, n.z}, {0.0f, 1.0f} };

        uint32_t base = static_cast<uint32_t>(mesh.vertices.size());
        mesh.vertices.push_back(v0);
        mesh.vertices.push_back(v1);
        mesh.vertices.push_back(v2);
        mesh.vertices.push_back(v3);

        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 1);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 0);
        mesh.indices.push_back(base + 2);
        mesh.indices.push_back(base + 3);
    }

    return mesh;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    SPHERE
//------------------------------------------------------------------------------------------------------------------------

MeshData constructSphere(uint32_t segments, uint32_t rings) noexcept
{
    MeshData mesh;

    // ⚙️ UV sphere generation with smooth normals
    mesh.vertices.reserve((rings + 1) * (segments + 1));
    mesh.indices.reserve(rings * segments * 6);

    for (uint32_t r = 0; r <= rings; ++r)
    {
        float phi   = PI_F * static_cast<float>(r) / static_cast<float>(rings);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (uint32_t s = 0; s <= segments; ++s)
        {
            float theta   = TWO_PI_F * static_cast<float>(s) / static_cast<float>(segments);
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            Vec3 normal = { sinPhi * cosTheta, cosPhi, sinPhi * sinTheta };
            Vec3 pos    = normal;             // ⚙️ Radius = 1

            FilamentVertex v;
            v.position[0] = pos.x;
            v.position[1] = pos.y;
            v.position[2] = pos.z;
            v.normal[0]   = normal.x;
            v.normal[1]   = normal.y;
            v.normal[2]   = normal.z;
            v.uv[0]       = static_cast<float>(s) / static_cast<float>(segments);
            v.uv[1]       = static_cast<float>(r) / static_cast<float>(rings);

            mesh.vertices.push_back(v);
        }
    }

    for (uint32_t r = 0; r < rings; ++r)
    {
        for (uint32_t s = 0; s < segments; ++s)
        {
            uint32_t i0 = r * (segments + 1) + s;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (segments + 1);
            uint32_t i3 = i2 + 1;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    return mesh;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    GRID / PLANE
//------------------------------------------------------------------------------------------------------------------------

MeshData constructGrid(float halfExtent, uint32_t divisions) noexcept
{
    MeshData mesh;

    // ⚙️ Generate a subdivided XZ plane
    uint32_t vertCount = (divisions + 1) * (divisions + 1);
    mesh.vertices.reserve(vertCount);
    mesh.indices.reserve(divisions * divisions * 6);

    float step = (halfExtent * 2.0f) / static_cast<float>(divisions);

    for (uint32_t z = 0; z <= divisions; ++z)
    {
        for (uint32_t x = 0; x <= divisions; ++x)
        {
            float px = -halfExtent + static_cast<float>(x) * step;
            float pz = -halfExtent + static_cast<float>(z) * step;

            FilamentVertex v;
            v.position[0] = px;
            v.position[1] = 0.0f;
            v.position[2] = pz;
            v.normal[0]   = 0.0f;
            v.normal[1]   = 1.0f;
            v.normal[2]   = 0.0f;
            v.uv[0]       = static_cast<float>(x) / static_cast<float>(divisions);
            v.uv[1]       = static_cast<float>(z) / static_cast<float>(divisions);

            mesh.vertices.push_back(v);
        }
    }

    for (uint32_t z = 0; z < divisions; ++z)
    {
        for (uint32_t x = 0; x < divisions; ++x)
        {
            uint32_t i0 = z * (divisions + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (divisions + 1);
            uint32_t i3 = i2 + 1;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    return mesh;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    DEFAULT SCENE
//------------------------------------------------------------------------------------------------------------------------

void populateDefaultScene(std::vector<MeshData>&   outMeshes,
                          std::vector<SceneObject>& outObjects) noexcept
{
    // ① Generate mesh primitives
    outMeshes.clear();
    outMeshes.push_back(constructCube());       // mesh index 0
    outMeshes.push_back(constructSphere(32, 16)); // mesh index 1
    outMeshes.push_back(constructGrid(5.0f, 10)); // mesh index 2

    outObjects.clear();

    // ② Gold sphere
    {
        SceneObject obj;
        obj.name      = "Sphere_Gold";
        obj.meshIndex = 1;
        obj.material  = MaterialPresets::gold();
        obj.position  = { -2.0f, 1.0f, 0.0f };
        outObjects.push_back(obj);
    }

    // ③ Plastic cube
    {
        SceneObject obj;
        obj.name      = "Cube_Plastic";
        obj.meshIndex = 0;
        obj.material  = MaterialPresets::plastic();
        obj.position  = { 0.0f, 0.5f, 0.0f };
        outObjects.push_back(obj);
    }

    // ④ Glass sphere
    {
        SceneObject obj;
        obj.name      = "Sphere_Glass";
        obj.meshIndex = 1;
        obj.material  = MaterialPresets::glass();
        obj.position  = { 2.0f, 1.0f, 0.0f };
        outObjects.push_back(obj);
    }

    // ⑤ Rough metal cube
    {
        SceneObject obj;
        obj.name      = "Cube_Metal";
        obj.meshIndex = 0;
        obj.material  = MaterialPresets::roughMetal();
        obj.position  = { 0.0f, 0.5f, 2.5f };
        outObjects.push_back(obj);
    }

    // ⑥ Additional rough sphere
    {
        SceneObject obj;
        obj.name      = "Sphere_Rough";
        obj.meshIndex = 1;
        obj.material  = MaterialPresets::roughMetal();
        obj.material.roughness = 1.0f;
        obj.position  = { -2.0f, 1.0f, 2.5f };
        outObjects.push_back(obj);
    }

    // ⑦ Floor grid
    {
        SceneObject obj;
        obj.name      = "Floor_Grid";
        obj.meshIndex = 2;
        obj.material  = MaterialPresets::floor();
        obj.position  = { 0.0f, 0.0f, 0.0f };
        outObjects.push_back(obj);
    }
}

} // namespace FilamentSceneBuilder
