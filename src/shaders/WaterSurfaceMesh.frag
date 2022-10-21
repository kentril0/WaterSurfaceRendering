#version 450

layout(location = 0) in vec3  inNormal;

layout(set = 0, binding = 1) uniform WaterSurfaceUBO
{
    float heightAmp;
} waterSurface;

layout(location = 0) out vec4 fragColor;

void main ()
{
    const vec3 kNormal = normalize(inNormal);

    fragColor = vec4(waterSurface.heightAmp * kNormal, 1.0);
}
