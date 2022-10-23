#version 450

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec3 inPos;
layout(location = 2) in vec3 inRefractPos;

layout(set = 0, binding = 1) uniform WaterSurfaceUBO
{
    float heightAmp;
} waterSurface;

layout(location = 0) out vec4 fragColor;


vec3 GetMaterialColor(const in vec3 kNormal)
{
    const vec3 SEA_BASE = vec3(0.0,0.09,0.18);
    const vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6)*0.6;

    vec3 kMaterial = SEA_BASE + SEA_WATER_COLOR * 0.2;

    // Sun contribution
    const vec3 kSunDir = normalize(vec3(0.1, 0.8, 0.1));
    const float kSunDiff = clamp( dot(kNormal, kSunDir), 0.0, 1.0 );
    const float kSunIntensity = 2.2;
    const vec3 kSunColor = vec3(0.7, 0.45, 0.3);
    vec3 color = kMaterial * kSunIntensity * kSunColor * kSunDiff;

    // Sky contribution
    const vec3 kDirUp = vec3(0.0, 1.0, 0.0);
    const float kSkyDiff = clamp(0.5 + 0.5 * dot(kNormal, kDirUp), 0.0, 1.0);
    color += kMaterial * vec3(0.5, 0.8, 0.9) * kSkyDiff;

    // Bounce lighting
    const vec3 kDirDown = vec3(0.0, -1.0, 0.0);
    const float kBounceDiff = clamp(0.5 + 0.5 * dot(kNormal, kDirDown),
                                    0.0, 1.0);
    //color += kMaterial * vec3(0.7, 0.3, 0.2) * kBounceDiff;
    return color;
}

void main ()
{
    const vec3 kNormal = normalize(inNormal);

    vec3 color;

    const float kOldArea = length(dFdx(inPos)) * length(dFdy(inPos));
    const float kNewArea = length(dFdx(inRefractPos)) * length(dFdy(inRefractPos));

    color = vec3(kOldArea / kNewArea * 0.2);


    fragColor = vec4(color, 1.0);
}
