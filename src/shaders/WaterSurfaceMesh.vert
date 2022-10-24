#version 450

layout(location = 0) in vec4 inPos;

layout(location = 0) out vec3 outPos;

layout(set = 0, binding = 0) uniform VertexUBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;


void main()
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
    outPos = gl_Position.xyz;
    //outPos = inPos.xyz;
}
