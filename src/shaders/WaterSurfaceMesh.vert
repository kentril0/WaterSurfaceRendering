#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec2 outUV;

layout(set = 0, binding = 0) uniform VertexUBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
    float WSHeightAmp;
} ubo;

layout(binding = 2) uniform sampler2D DisplacementMap;


void main()
{
    vec3 D = texture(DisplacementMap, inUV).xyz;
    D.y *= ubo.WSHeightAmp;
    outPos = inPos + D;

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(outPos, 1.0);

    outUV = inUV;
}
