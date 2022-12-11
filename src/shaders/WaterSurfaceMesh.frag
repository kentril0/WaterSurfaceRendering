#version 450

layout(location = 0) in vec4 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 1) uniform WaterSurfaceUBO
{
    vec3  camPos;
    float height;
    vec3 absorpCoef;
    vec3 scatterCoef;
    vec3 backscatterCoef;
    // ------------- Terrain
    vec3 terrainColor;
    // ------------ Sky
    float skyIntensity;
    float specularIntensity;
    float specularHighlights;
    vec3  sunColor;
    float sunIntensity;
    // ------------- Preetham Sky
    vec3  sunDir;
    float turbidity;
    vec3 A;
    vec3 B;
    vec3 C;
    vec3 D;
    vec3 E;
    vec3 ZenithLum;
    vec3 ZeroThetaSun;
} surface;

#define M_PI 3.14159265358979323846
#define ONE_OVER_PI (1.0 / M_PI)

// Water and Air indices of refraction
#define WATER_IOR 1.33477
#define AIR_IOR 1.0
#define IOR_AIR2WATER (AIR_IOR / WATER_IOR)

#define NORMAL_WORLD_UP vec3(0.0, 1.0, 0.0)

#define TERRAIN_HEIGHT 0.0f
vec4 kTerrainBoundPlane = vec4(NORMAL_WORLD_UP, TERRAIN_HEIGHT);

struct Ray
{
    vec3 org;
    vec3 dir;
};

/**
 * @brief Returns sky luminance at view direction
 * @param kSunDir Normalized vector to sun
 * @param kViewDir Normalized vector to eye
 */
vec3 SkyLuminance(const in vec3 kSunDir, const in vec3 kViewDir);

/**
 * @brief Finds an intersection point of ray with the terrain
 * @return Distance along the ray; positive if intersects, else negative
 */
float IntersectTerrain(const in Ray ray);

vec3 TerrainNormal(const in vec2 p);

vec3 TerrainColor(const in vec2 p);

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
    const float kScale = 0.1;
    return exp(-surface.absorpCoef  * kScale * kDistance 
               -surface.scatterCoef * kScale * kDepth);
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
    const vec3 kLightDir = normalize(surface.sunDir);

    // Reflection of camera ray
    const vec3 kReflectDir = reflect(ray.dir, kNormal);
    const vec3 kSkyReflected = SkyLuminance(kLightDir, kReflectDir);

    // Diffuse atmospheric skylight
    vec3 L_a;
    {
        const float kNdL = max(dot(kNormal, kLightDir), 0.0);
        L_a = surface.skyIntensity * kNdL * kSkyReflected;
    }

    // Direction to camera
    const vec3 kViewDir = vec3(-ray.dir);

    // Amount of light coming directly from the sun reflected to the camera
    vec3 L_s;
    {
    // Phong
    #if 0
        const vec3 kReflectDir = reflect(-kLightDir, kNormal);
        const float specular = surface.specularIntensity *
                clamp(
                    pow(
                        max( dot(kReflectDir, kViewDir), 0.0 ),
                    surface.specularHighlights)
                , 0.0, 1.0);
    // Blinn-Phong - more accurate when compared to ocean sunset photos
    #else
        const vec3 kHalfWayDir = normalize(kLightDir + kViewDir);
        const float specular = surface.specularIntensity *
                clamp(
                    pow(
                        max( dot(kNormal, kHalfWayDir), 0.0 ),
                    surface.specularHighlights)
                , 0.0, 1.0);
    #endif

        L_s = surface.sunIntensity * specular * kSkyReflected;
    }

    const vec3 kRefractDir = RefractAirIncident(-kLightDir, kNormal);

    // Light just below the surface transmitted through into the air
    vec3 L_u;
    {
        // Downwelling irradiance just below the water surface
        const vec3 E_d0 = M_PI * L_a;

        // Constant diffuse radiance just below the surface from sun and sky
        const vec3 L_df0 = (0.33*surface.backscatterCoef) /
                            surface.absorpCoef * (E_d0 * ONE_OVER_PI);

        const Ray kRefractRay = Ray( p_w, kRefractDir );

        const float t_g = IntersectTerrain(kRefractRay);
        const vec3 p_g = kRefractRay.org + t_g * kRefractRay.dir;
        const vec3 L_g = ComputeTerrainRadiance(p_g, kRefractRay.dir);

        L_u =        Attenuate(t_g, 0.0)                   * L_g +
              (1.0 - Attenuate(t_g, abs(p_w.y - p_g.y) ) ) * L_df0;
    }

    // Fresnel reflectivity to the camera
    float F_r = FresnelFull( dot(kNormal, kViewDir), dot(-kNormal, kRefractDir) );

    return F_r*(L_s + L_a) + (1.0-F_r)*L_u;
}

void main ()
{
    const Ray ray = Ray(surface.camPos, normalize(inPos.xyz - surface.camPos));
    const vec3 kNormal = normalize( inNormal );

    const vec3 p_w = vec3(inPos.x, inPos.y + surface.height, inPos.z);
    vec3 color = ComputeWaterSurfaceColor(ray, p_w, kNormal);

    // TODO foam
    if (inPos.w < 0.0)
        color = vec3(1.0);

    fragColor = vec4(color, 1.0);

    // Tone mapping
    fragColor = vec4(1.0) - exp(-fragColor * 2.0f);
}


// =============================================================================
// Terrain functions

float Fbm4Noise2D(in vec2 p);

float TerrainHeight(const in vec2 p)
{
    return TERRAIN_HEIGHT - 8.f * Fbm4Noise2D(p.yx * 0.02f);
}

vec3 TerrainNormal(const in vec2 p)
{
    // Approximate normal based on central differences method for height map
    const vec2 kEpsilon = vec2(0.0001, 0.0);

    return normalize(
        vec3(
            // x + offset
            TerrainHeight(p - kEpsilon.xy) - TerrainHeight(p + kEpsilon.xy), 
            10.0f * kEpsilon.x,
            // z + offset
            TerrainHeight(p - kEpsilon.yx) - TerrainHeight(p + kEpsilon.yx)
        )
    );
}

vec3 TerrainColor(const in vec2 p)
{
    //const vec3 kMate = vec3(0.964, 1.0, 0.824);
    float n = clamp(Fbm4Noise2D(p.yx * 0.02 * 2.), 0.6, 0.9);

    return n * surface.terrainColor;
}

float IntersectTerrain(const in Ray ray)
{
    return -( dot(ray.org, kTerrainBoundPlane.xyz) - kTerrainBoundPlane.w ) /
           dot(ray.dir, kTerrainBoundPlane.xyz);
}

// =============================================================================
// Noise functions

float hash1(in vec2 i)
{
    i = 50.0 * fract( i * ONE_OVER_PI );
    return fract( i.x * i.y * (i.x + i.y) );
}

float Noise2D(const in vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

#ifdef INTERPOLATION_CUBIC
    vec2 u = f*f * (3.0-2.0*f);
#else
    vec2 u = f*f*f * (f * (f*6.0-15.0) +10.0);
#endif
    float a = hash1(i + vec2(0,0) );
    float b = hash1(i + vec2(1,0) );
    float c = hash1(i + vec2(0,1) );
    float d = hash1(i + vec2(1,1) );

    return -1.0 + 2.0 * (a + 
                         (b - a) * u.x + 
                         (c - a) * u.y + 
                         (a - b - c + d) * u.x * u.y);
}

const mat2 MAT345 = mat2( 4./5., -3./5.,
                          3./5.,  4./5. );

float Fbm4Noise2D(in vec2 p)
{
    const float kFreq = 2.0;
    const float kGain = 0.5;
    float amplitude = 0.5;
    float value = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        value += amplitude * Noise2D(p);
        amplitude *= kGain;
        p = kFreq * MAT345 * p;
    }
    return value;
}

// =============================================================================
// Preetham Sky

float SaturateDot(const in vec3 v, const in vec3 u)
{
    return max( dot(v,u), 0.0f );
}

vec3 ComputePerezLuminanceYxy(const in float theta, const in float gamma)
{
    const float kBias = 1e-3f;
    return (1.f + surface.A * exp( surface.B / (cos(theta)+kBias) ) ) *
           (1.f + surface.C * exp( surface.D * gamma) +
            surface.E * cos(gamma) * cos(gamma) );
}

vec3 ComputeSkyLuminance(const in vec3 kSunDir, const in vec3 kViewDir)
{
    const float thetaView = acos( SaturateDot(kViewDir, vec3(0,1,0)) );
    const float gammaView = acos( SaturateDot(kSunDir, kViewDir) );

    const vec3 fThetaGamma = ComputePerezLuminanceYxy(thetaView, gammaView);
    
    return surface.ZenithLum * (fThetaGamma / surface.ZeroThetaSun);
}

vec3 YxyToRGB(const in vec3 Yxy);

vec3 SkyLuminance(const in vec3 kSunDir, const in vec3 kViewDir)
{
    const float kSunDisk = smoothstep(0.997f, 1.f,
                                      SaturateDot(kViewDir, kSunDir) );

    const vec3 kSkyLuminance = YxyToRGB( ComputeSkyLuminance(kSunDir, kViewDir) );
    return kSkyLuminance * 0.05f + kSunDisk * kSkyLuminance;
}

// =============================================================================
// Althar. Preetham Sky. [online]. Shadertoy.com. 2015.
// https://www.shadertoy.com/view/llSSDR

vec3 YxyToXYZ( const in vec3 Yxy )
{
    const float Y = Yxy.r;
    const float x = Yxy.g;
    const float y = Yxy.b;

    const float X = x * ( Y / y );
    const float Z = ( 1.0 - x - y ) * ( Y / y );

    return vec3(X,Y,Z);
}

vec3 XYZToRGB( const in vec3 XYZ )
{
    // CIE/E
    const mat3 M = mat3
    (
         2.3706743, -0.9000405, -0.4706338,
        -0.5138850,  1.4253036,  0.0885814,
          0.0052982, -0.0146949,  1.0093968
    );

    return XYZ * M;
}


vec3 YxyToRGB( const in vec3 Yxy )
{
    const vec3 XYZ = YxyToXYZ( Yxy );
    const vec3 RGB = XYZToRGB( XYZ );
    return RGB;
}

// =============================================================================
