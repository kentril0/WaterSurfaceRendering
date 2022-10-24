#version 450

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform WaterSurfaceUBO
{
    float heightAmp;
} waterSurface;

layout(binding = 2) uniform sampler2D WSHeightmap;
layout(binding = 3) uniform sampler2D WSNormalmap;


#define WATER_IOR 1.33477
#define AIR_IOR   1.0

#define NORMAL_WORLD_UP vec3(0.0, 1.0, 0.0)
#define TERRAIN_TOP_PLANE_HEIGHT -10.0

const vec4 kTerrainBoundPlane = vec4(NORMAL_WORLD_UP, TERRAIN_TOP_PLANE_HEIGHT);

struct Ray
{
    vec3 org;
    vec3 dir;
};


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


/**
 * @brief Refraction of air incident light
 * @param light Direction of the incident light, unit vector
 * @param normal Surface normal, unit vector
 * @return Direction of the refracted ray
 */
vec3 RefractAirIncident(const in vec3 light, const in vec3 normal)
{
    const float n = AIR_IOR / WATER_IOR;
    return refract(light, normal, n);
}

/**
 * @brief Intersects terrain from above its bounding plane
 * @return Distance along the ray; positive if intersects, else negative
 */
vec3 IntersectGround(const in Ray ray)
{
    float t = -( dot(ray.org, kTerrainBoundPlane.xyz) - kTerrainBoundPlane.w ) /
                dot(ray.dir, kTerrainBoundPlane.xyz);
    return ray.org + t*ray.dir;
}

void main ()
{
    const vec2 kResolution = vec2(1280.0, 720.0);
    //const vec2 uv = vec2(gl_FragCoord.x, kResolution.y - gl_FragCoord.y);
    //const vec2 uv = vec2(gl_FragCoord.xy);
    //const vec2 p = (2.0 * uv - kResolution.xy) / kResolution.y;

    const vec3 kNormal = normalize( texture(WSNormalmap, gl_FragCoord.xy / kResolution).xyz );

    fragColor = vec4( GetMaterialColor(kNormal), 1.0);
    return;

    const vec3 kSunDir = normalize(vec3(0.0, 1.0, 0.4));
    const vec3 kRefractDir = RefractAirIncident(kSunDir, kNormal);

    // inPos.xyz transformed position
    const vec3 kRefractPos = IntersectGround(Ray(inPos, kRefractDir));

    vec3 color;

    const float kOldArea = length(dFdx(inPos)) * length(dFdy(inPos));
    const float kNewArea = length(dFdx(kRefractPos)) * length(dFdy(kRefractPos));

    color = vec3(kOldArea / kNewArea * 0.2);


    fragColor = vec4(color, 1.0);
}
