#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec4 inNormal;

layout(set = 0, binding = 0) uniform VertexUBO
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outPos;
layout(location = 2) out vec3 outRefractPos;


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

void main()
{
    outNormal = normalize(inNormal.xyz);

    const vec3 kSunDir = normalize(vec3(0.0, 1.0, 0.4));
    const vec3 kRefractDir = RefractAirIncident(kSunDir, outNormal);

    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);
    outPos = inPos.xyz;
    outRefractPos = IntersectGround(Ray(outPos, kRefractDir));
}
