#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inNormal;

layout(set = 0, binding = 0) uniform VertexUBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3  outNormal;

void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);

    outNormal = normalize(inNormal.xyz);
}
