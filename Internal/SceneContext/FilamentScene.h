// File: Source/Filament/Internal/SceneContext/FilamentScene.h
#pragma once

/*====================================================================================================================================
                                                     FILAMENTSCENE.H
====================================================================================================================================*/
// 🧩 Scene graph, procedural mesh generation (cube, sphere, grid), object management.

#include "../Auxiliary/FilamentTypes.h"
#include "../TextureSubsystem/FilamentMaterial.h"
#include "../Algebra/Vector.h"
#include <vector>
#include <cstdint>

//------------------------------------------------------------------------------------------------------------------------
//                                                    MESH DATA
//------------------------------------------------------------------------------------------------------------------------

struct MeshData
{
    std::vector<FilamentVertex> vertices;        // [-] - Vertex buffer data
    std::vector<uint32_t>      indices;          // [-] - Index buffer data
};

//------------------------------------------------------------------------------------------------------------------------
//                                                    PUBLIC API
//------------------------------------------------------------------------------------------------------------------------

namespace FilamentSceneBuilder
{
    // ① Generate a unit cube (side length 1, centered at origin)
    [[nodiscard]] MeshData constructCube() noexcept;

    // ② Generate a UV sphere (radius 1, centered at origin)
    [[nodiscard]] MeshData constructSphere(uint32_t segments = 32, uint32_t rings = 16) noexcept;

    // ③ Generate a grid/plane floor (XZ plane, centered at origin)
    [[nodiscard]] MeshData constructGrid(float halfExtent = 5.0f, uint32_t divisions = 10) noexcept;

    // ④ Populate the default scene with cube, sphere, grid, and multiple materials
    void populateDefaultScene(std::vector<MeshData>&   outMeshes,
                              std::vector<SceneObject>& outObjects) noexcept;
}
