Figment/
│
├── CMakeLists.txt
├── Figment.h                              ── engine facade / public API
├── Entry.cpp                              ── bootstrap
│
├── VulkanSubstrate/                             ── Vulkan 1.3 abstraction
│   ├── Context.h          .cpp            ── instance, device, queues
│   ├── Heap.h             .cpp            ── GPU memory sub-allocator
│   ├── Swapchain.h        .cpp            ── present surface + image cycle
│   ├── Pipeline.h         .cpp            ── graphics / compute / RT state
│   ├── ShaderObject.h     .cpp            ── SPIR-V load, reflection, modules
│   ├── Descriptor.h       .cpp            ── set layouts, pools, writes
│   ├── Bindless.h         .cpp            ── bindless resource table
│   ├── Command.h          .cpp            ── command buffer record + submit
│   ├── Barrier.h          .cpp            ── synchronization2 transitions
│   ├── Buffer.h           .cpp            ── typed GPU buffer wrapper
│   ├── Image.h            .cpp            ── image + view + layout
│   └── Query.h            .cpp            ── timestamp & occlusion
│
├── Radiance/                              ── global illumination
│   ├── Integrator.h       .cpp            ── Monte Carlo path integration
│   ├── ProbeField.h       .cpp            ── irradiance probe volume (DDGI)
│   ├── Reservoir.h        .cpp            ── ReSTIR sample reservoirs
│   ├── Transport.h        .cpp            ── light transport solver
│   ├── Photon.h           .cpp            ── photon scatter / gather
│   ├── Caustic.h          .cpp            ── caustic estimation
│   └── SurfelCache.h      .cpp            ── surface element radiance cache
│
├── Accel/                                 ── ray acceleration structures
│   ├── Structure.h        .cpp            ── BLAS / TLAS build + compact
│   ├── Trace.h            .cpp            ── ray dispatch, SBT
│   └── Partition.h        .cpp            ── spatial subdivision
│
├── Cascade/                               ── render pass sequence
│   ├── GBuffer.h          .cpp            ── geometry → dynamic rendering
│   ├── Shadow.h           .cpp            ── shadow atlas, cascades
│   ├── Reflect.h          .cpp            ── RT reflections / SSR fallback
│   ├── Composite.h        .cpp            ── final gather + compose
│   ├── ToneMap.h          .cpp            ── filmic / ACES operators
│   └── PostFX.h           .cpp            ── bloom, motion blur, grain
│
├── Surface/                               ── material & shading
│   ├── Material.h         .cpp            ── parameter packing, ID
│   ├── BSDF.h             .cpp            ── scattering distribution
│   ├── Texture.h          .cpp            ── upload, mip generation
│   └── Sampler.h          .cpp            ── filter + address modes
│
├── Optics/                                ── camera & lens
│   ├── Lens.h             .cpp            ── projection, aperture, DOF
│   ├── Frustum.h          .cpp            ── planes, culling
│   └── Exposure.h         .cpp            ── histogram auto-exposure
│
├── Topology/                              ── scene representation
│   ├── Graph.h            .cpp            ── DAG hierarchy
│   ├── Entity.h           .cpp            ── scene object handle
│   ├── Spatial.h          .cpp            ── local / world transforms
│   └── Bounds.h           .cpp            ── AABB, bounding sphere
│
├── Emission/                              ── light sources
│   ├── Emitter.h          .cpp            ── base interface
│   ├── Punctual.h         .cpp            ── point, spot, directional
│   ├── Area.h             .cpp            ── rect, disc, mesh emitter
│   └── Sky.h              .cpp            ── environment map, procedural
│
├── Geometry/                              ── mesh & vertex data
│   ├── Mesh.h             .cpp            ── vertex / index buffer pair
│   ├── Vertex.h           .cpp            ── layout, attributes, format
│   └── Primitive.h        .cpp            ── draw call primitive
│
├── Temporal/                              ── frame orchestration
│   ├── FrameGraph.h       .cpp            ── pass DAG, resource aliasing
│   ├── Cadence.h          .cpp            ── delta, frame pacing
│   └── Sync.h             .cpp            ── timeline semaphores, fences
│
├── Denoise/                               ── reconstruction
│   ├── SpatialFilter.h    .cpp            ── edge-preserving blur
│   ├── Accumulate.h       .cpp            ── temporal reprojection
│   └── Wavelet.h          .cpp            ── à-trous decomposition
│
├── Volume/                                ── participating media
│   ├── Medium.h           .cpp            ── absorption + scattering σ
│   ├── Phase.h            .cpp            ── Henyey-Greenstein, etc.
│   └── March.h            .cpp            ── ray march integration
│
├── Algebra/                               ── math, no dependencies
│   ├── Vec.h                              ── vec2 / vec3 / vec4
│   ├── Mat.h                              ── mat3 / mat4
│   ├── Quat.h                             ── quaternion
│   ├── AABB.h                             ── axis-aligned box ops
│   ├── Sampling.h                         ── Halton, Sobol, blue noise
│   ├── Spectral.h                         ── wavelength utilities
│   └── SH.h                              ── spherical harmonics (L2)
│
├── Interface/                             ── ImGui debug / editor
│   ├── Overlay.h          .cpp            ── ImGui Vulkan init + render
│   ├── Viewport.h         .cpp            ── scene view widget
│   ├── Inspector.h        .cpp            ── entity / material panel
│   └── Metrics.h          .cpp            ── GPU timings, pass cost
│
└── Shader/                                ── GLSL → SPIR-V
    ├── Compile.py                         ── glslangValidator batch
    │
    ├── GBuffer.vert
    ├── GBuffer.frag
    ├── Shadow.vert
    ├── Shadow.frag
    ├── Composite.frag
    ├── FullScreen.vert                    ── shared full-screen triangle
    │
    ├── ToneMap.comp
    ├── Denoise.comp
    ├── ProbeUpdate.comp
    ├── Histogram.comp
    │
    ├── Trace.rgen
    ├── Trace.rmiss
    ├── Trace.rchit
    ├── TraceShadow.rmiss
    │
    └── Include/                           ── shared GLSL headers
        ├── Common.glsl
        ├── BRDF.glsl
        ├── Sampling.glsl
        ├── Reservoir.glsl
        └── Tonemap.glsl