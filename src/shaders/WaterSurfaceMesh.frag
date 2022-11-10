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
#define WATER_IOR 1.33477
#define AIR_IOR 1.0
#define IOR_AIR2WATER (AIR_IOR / WATER_IOR)

#define NORMAL_WORLD_UP vec3(0.0, 1.0, 0.0)
#define TERRAIN_TOP_PLANE_HEIGHT -100.0

//const vec4 kTerrainBoundPlane = vec4(NORMAL_WORLD_UP, TERRAIN_TOP_PLANE_HEIGHT);
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
 * @return Distance along the ray; positive if intersects, else negative
 */
float IntersectGround(const in Ray ray)
{
    return -( dot(ray.org, kTerrainBoundPlane.xyz) - kTerrainBoundPlane.w ) /
           dot(ray.dir, kTerrainBoundPlane.xyz);
}

vec3 TerrainNormal(const in vec2 p)
{
    return NORMAL_WORLD_UP;
}

vec3 TerrainColor(const in vec3 p)
{
    const vec3 kMate = vec3(0.964, 1.0, 0.824);
    return kMate;
}

/*
const float kC_p = 2.0; // Water type III
const vec3 kWavelengths = vec3(680e-9, 550e-9, 440e-9);
const vec3 K_d = vec3(0.54357, 0.120, 0.150);    // Ocean waters type III
const vec3 kAbsorbCoef = K_d;

const vec3 kScatterCoef = 0.219 * ( (-0.00113 * kWavelengths + 1.62517) /
                                    (-0.00113 * 514e-9       + 1.62517) );

//const vec3 kBackScatterCoef = 0.01829 * kScatterCoef + 0.00006;
const vec3 kBackScatterCoef = 0.5 * vec3(0.1) + (
        0.002 + 0.02 * ( 0.5 - 0.25 * ((1.0/log(10.0)) * log(kC_p)) ) *
        ( 550.0e-9 / kWavelengths )
    ) * 0.30 * pow(kC_p, 0.62);

const vec3 kAttenCoef = kAbsorbCoef + kBackScatterCoef;
*/

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
    //theta_i = max(theta_i, 0.0);
    //theta_t = max(theta_t, 0.0);

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
    return exp( -surface.absorpCoef * 0.1*kDistance -surface.scatterCoef * 0.1*kDepth);
}

/**
 * @param kNormal Normal vector at the fragment
 * @param kLightDir Unit vector pointing to the light at the fragment
 * @param kViewDir Unit vector pointing in the view direction (to the camera)
 */
vec3 ComputeSunReflectedContrib(
    const in vec3 kNormal,
    const in vec3 kLightDir,
    const in vec3 kViewDir
)
{           
    const float kNormalDotLight = dot(kNormal, kLightDir);
    const vec3 kReflectDir = reflect(-kLightDir, kNormal);

            // diffuse
    return surface.sunColor.a * surface.sunColor.rgb * kNormalDotLight +
            // specular
           surface.specularIntensity *
            clamp( 
                pow( 
                    max( dot(kReflectDir, kViewDir), 0.0 ),
                surface.specularHighlights)
            , 0.0, 1.0) * surface.sunColor.a * surface.sunColor.rgb;
}

vec3 ComputeSkyContrib(
    const in vec3 p_w,
    const in vec3 kLightReflectDir
)
{
    return SkyColor( Ray(p_w, kLightReflectDir) ) * surface.skyIntensity;
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
    //const float n = IOR_AIR2WATER
    //const float w = n * NdL;
    //const float k = sqrt(1.0 + (w-n)*(w+n));
    //const vec3 refractDir = (w - k)*kNormal - n*kLightDir;
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
    return TerrainColor(p_g) *
            max(
                dot(TerrainNormal(p_g.xz), -kIncidentDir),
            0.0);
}

vec3 ComputeWaterSurfaceColor(
    const in Ray ray,
    const in vec3 p_w,
    const in vec3 kNormal)
{
    const vec3 kLightDir = normalize(surface.sunDir);
    const vec3 kReflectDir = reflect(-kLightDir, kNormal);

    const vec3 kViewDir = normalize(-ray.dir);

    // Amount of light coming directly from the sun reflected to the camera
    const vec3 L_s = ComputeSunReflectedContrib(kNormal, kLightDir, kViewDir);

    // Diffuse atmospheric skylight
    const vec3 L_a = ComputeSkyContrib(p_w, kReflectDir);

    const vec3 kRefractDir = RefractAirIncident(kLightDir, kNormal);

    // Light just below the surface transmitted through into the air
    vec3 L_u;
    {
        // Downwelling irradiance just below the water surface
        const vec3 E_d0 = M_PI*L_a + L_s;

        // Constant diffuse radiance just below the surface from sun and sky
        const vec3 L_df0 = (0.33*surface.backscatterCoef) / surface.absorpCoef * (E_d0 * ONE_OVER_PI);

        const Ray kRefractRay = Ray( p_w, kRefractDir );

        const float t_g = IntersectGround(kRefractRay);
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
