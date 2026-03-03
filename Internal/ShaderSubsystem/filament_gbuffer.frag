#version 450

/*====================================================================================================================================
                                                   FILAMENT_GBUFFER.FRAG
====================================================================================================================================*/
// 🧩 GBuffer fragment shader — packs 5 PBR channels into 3 render targets.
//    RT0: Albedo.RGB + Metallic.A    (R8G8B8A8_UNORM)
//    RT1: Normal.XY (octahedral)     (R16G16_SFLOAT)
//    RT2: Roughness.R + Specular.G   (R8G8_UNORM)

// ⚙️ Set 1: Material UBO
layout(set = 1, binding = 0) uniform MaterialUBO
{
    vec4  albedo;
    float roughness;
    float metallic;
    float specularF0;
    float refractionStrength;
    vec4  pad0;
    vec4  pad1;
} material;

// ⚙️ Inputs from vertex stage
layout(location = 0) in vec3 fragWorldPosition;
layout(location = 1) in vec3 fragWorldNormal;
layout(location = 2) in vec2 fragUV;

// ⚙️ MRT outputs
layout(location = 0) out vec4 outRT0;            // Albedo.RGB + Metallic.A
layout(location = 1) out vec2 outRT1;            // Normal.XY (octahedral)
layout(location = 2) out vec2 outRT2;            // Roughness.R + Specular.G


//------------------------------------------------------------------------------------------------------------------------
//                                               OCTAHEDRAL NORMAL ENCODING
//------------------------------------------------------------------------------------------------------------------------

// ⚙️ Encode unit normal to octahedral UV [-1,1] → [0,1]
//    Reference: "A Survey of Efficient Representations for Independent Unit Vectors" (Cigolle et al. 2014)

vec2 encodeOctahedral(vec3 n)
{
    // ① Project onto octahedron
    vec2 p = n.xy * (1.0 / (abs(n.x) + abs(n.y) + abs(n.z)));

    // ② Reflect the folds of the lower hemisphere
    if (n.z < 0.0)
    {
        p = (1.0 - abs(p.yx)) * vec2(p.x >= 0.0 ? 1.0 : -1.0,
                                      p.y >= 0.0 ? 1.0 : -1.0);
    }

    return p;
}


//------------------------------------------------------------------------------------------------------------------------
//                                                    MAIN
//------------------------------------------------------------------------------------------------------------------------

void main()
{
    vec3 normal = normalize(fragWorldNormal);

    // ① RT0: Albedo RGB + Metallic in alpha
    outRT0 = vec4(material.albedo.rgb, material.metallic);

    // ② RT1: Octahedral-encoded normal
    outRT1 = encodeOctahedral(normal);

    // ③ RT2: Roughness + Specular F0
    outRT2 = vec2(material.roughness, material.specularF0);
}
