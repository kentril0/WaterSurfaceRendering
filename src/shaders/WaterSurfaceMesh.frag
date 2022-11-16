#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform WaterSurfaceUBO
{
    vec3  camPos;
    vec3  sunDir;
    vec4  sunColor;  // vec4( vec3(sunColor), sunIntensity )
    // ----
    float terrainDepth;
    float skyIntensity;
    float specularIntensity;
    float specularHighlights;
    vec3 absorpCoef;
    vec3 scatterCoef;
    vec3 backscatterCoef;
} surface;

#define M_PI 3.14159265358979323846
#define ONE_OVER_PI (1.0 / M_PI)

// Water and Air indices of refraction
#define WATER_IOR 1.33477
#define AIR_IOR 1.0
#define IOR_AIR2WATER (AIR_IOR / WATER_IOR)

#define NORMAL_WORLD_UP vec3(0.0, 1.0, 0.0)

vec4 kTerrainBoundPlane = vec4(NORMAL_WORLD_UP, surface.terrainDepth);

struct Ray
{
    vec3 org;
    vec3 dir;
};

vec3 SkyColor(const in Ray ray)
{
    float y = ray.dir.y;
    y = (max(y,0.0)*0.8+0.2)*0.8;
    return vec3(pow(1.0-y,2.0), 1.0-y, 0.6+(1.0-y)*0.4) * 1.1;
}

/**
 * @brief Intersects terrain from above its bounding plane
 * @brief Finds an intersection point of ray with the terrain
 * @return Distance along the ray; positive if intersects, else negative
 */
float IntersectTerrain(const in Ray ray)
{
    return -( dot(ray.org, kTerrainBoundPlane.xyz) - kTerrainBoundPlane.w ) /
           dot(ray.dir, kTerrainBoundPlane.xyz);
}

vec3 TerrainNormal(const in vec2 p)
{
    return NORMAL_WORLD_UP;
}

vec3 TerrainColor(const in vec2 p)
{
    const vec3 kMate = vec3(0.964, 1.0, 0.824);
    return kMate;
}

/**
 * @brief Fresnel reflectance for unpolarized incident light
 *  holds for both air-incident and water-incident cases
 * @param theta_i Angle of incidence between surface normal and the direction
 *  of propagation
 * @param theta_t Angle of the transmitted (refracted) ray relative to the surface
 *  normal
 */
float FresnelFull(in float theta_i, in float theta_t)
{
    // Normally incident light
    if (theta_i <= 0.00001)
    {
        const float at0 = (WATER_IOR-1.0) / (WATER_IOR+1.0);
        return at0*at0;
    }

    const float t1 = sin(theta_i-theta_t) / sin(theta_i+theta_t);
    const float t2 = tan(theta_i-theta_t) / tan(theta_i+theta_t);
    return 0.5 * (t1*t1 + t2*t2);
}

vec3 Attenuate(const in float kDistance, const in float kDepth)
{
    return exp(-surface.absorpCoef  * 0.1f * kDistance 
               -surface.scatterCoef * 0.1f * kDepth);
}

/**
 * @brief Refraction of air incident light
 * @param kIncident Direction of the incident light, unit vector
 * @param kNormal Surface normal, unit vector
 * @return Direction of the refracted ray
 */
vec3 RefractAirIncident(const in vec3 kIncident, const in vec3 kNormal)
{
    return refract(kIncident, kNormal, IOR_AIR2WATER);
}

/**
 * @param p_g Point on the terrain
 * @param kIncidentDir Incident unit vector
 * @return Outgoing radiance from the terrain to the refracted direction on
 *  the water surface.
 */
vec3 ComputeTerrainRadiance(
    const in vec3 p_g,
    const in vec3 kIncidentDir
)
{
    // TODO better TERRAIN COLOR
    return TerrainColor(p_g.xz) * dot(TerrainNormal(p_g.xz), -kIncidentDir);
}

vec3 ComputeWaterSurfaceColor(
    const in Ray ray,
    const in vec3 p_w,
    const in vec3 kNormal)
{
    const vec3 kViewDir = vec3(-ray.dir);
    const vec3 kLightDir = normalize(surface.sunDir);

    // Diffuse atmospheric skylight
    vec3 L_a;
    {
        const vec3 kViewReflect = reflect(kViewDir, kNormal);
        L_a = SkyColor(Ray(p_w, kViewReflect)) * surface.skyIntensity;

        // Sun diffuse contribution - hitting the surface directly
        // TODO should be already in the sky model
        const float kNormalDotLight = max(dot(kNormal, kLightDir), 0.0f);
        L_a += surface.sunColor.a * surface.sunColor.rgb * kNormalDotLight;
    }

    // Amount of light coming directly from the sun reflected to the camera
    vec3 L_s;
    {
        const vec3 kReflectDir = reflect(-kLightDir, kNormal);
        const float specular = surface.specularIntensity *
                clamp( 
                    pow( 
                        max( dot(kReflectDir, kViewDir), 0.0 ),
                    surface.specularHighlights)
                , 0.0, 1.0);

        L_s = surface.sunColor.a * surface.sunColor.rgb * specular;
    }

    // Light just below the surface transmitted through into the air
    const vec3 kRefractDir = RefractAirIncident(-kLightDir, kNormal);
    vec3 L_u;
    {
        // Downwelling irradiance just below the water surface
        const vec3 E_d0 = M_PI*L_a + L_s;

        // Constant diffuse radiance just below the surface from sun and sky
        const vec3 L_df0 = (0.33*surface.backscatterCoef) /
                            surface.absorpCoef * (E_d0 * ONE_OVER_PI);

        const Ray kRefractRay = Ray( p_w, kRefractDir );

        const float t_g = IntersectTerrain(kRefractRay);
        const vec3 p_g = kRefractRay.org + t_g * kRefractRay.dir;
        const vec3 L_g = ComputeTerrainRadiance(p_g, kRefractRay.dir);

        L_u = Attenuate(t_g, 0.0) * L_g +
              ( 1.0-Attenuate(t_g, abs(p_w.y - p_g.y) ) ) * L_df0;
    }

    // Fresnel reflectivity to the camera
    float F_r = FresnelFull( dot(kViewDir,kNormal), dot(-kNormal,kRefractDir) );

    return F_r*(L_s + L_a) + (1.0-F_r)*L_u;
}

void main ()
{
    const Ray ray = Ray(surface.camPos, normalize(inPos.xyz-surface.camPos));
    const vec3 kNormal = normalize( inNormal );

    vec3 color = ComputeWaterSurfaceColor(ray, inPos.xyz, kNormal);

    // TODO foam
    if (inPos.w < 0.0)
        color = vec3(1.0);

    // TODO color correction, grading

    fragColor = vec4( color ,1.0);
}
