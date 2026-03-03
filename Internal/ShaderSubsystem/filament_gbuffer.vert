#version 450

/*====================================================================================================================================
                                                   FILAMENT_GBUFFER.VERT
====================================================================================================================================*/
// 🧩 GBuffer vertex shader — transforms vertices and passes attributes to fragment stage.

// ⚙️ Set 0: Scene UBO
layout(set = 0, binding = 0) uniform SceneUBO
{
    mat4 viewMatrix;
    mat4 projMatrix;
    mat4 invViewMatrix;
    vec4 lightDirection;
    vec4 lightColor;
    vec4 cameraPosition;
    vec4 timeAndResolution;
} scene;

// ⚙️ Push constant: per-object transform
layout(push_constant) uniform PushConstants
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} push;

// ⚙️ Vertex inputs
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// ⚙️ Outputs to fragment stage
layout(location = 0) out vec3 fragWorldPosition;
layout(location = 1) out vec3 fragWorldNormal;
layout(location = 2) out vec2 fragUV;

void main()
{
    vec4 worldPos    = push.modelMatrix * vec4(inPosition, 1.0);
    fragWorldPosition = worldPos.xyz;
    fragWorldNormal   = normalize(mat3(push.normalMatrix) * inNormal);
    fragUV            = inUV;

    gl_Position = scene.projMatrix * scene.viewMatrix * worldPos;
}
