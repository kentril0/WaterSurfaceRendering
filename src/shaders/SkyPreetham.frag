#version 450

layout(location = 0) in vec2 inUV;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform PreethamUBO 
{
    vec2 resolution;
    mat3 camView;
    vec3 camPos;
    float camFOV;
    vec3 sunColor;
    float sunIntensity;
    // ------------ Preetham Sky
    vec3 sunDir;        ///< Normalized direction to the sun
    float turbidity;
    vec3 A;
    vec3 B;
    vec3 C;
    vec3 D;
    vec3 E;
    vec3 ZenithLum;
    vec3 ZeroThetaSun;
} params;

#define M_PI 3.14159265358979323846

float SaturateDot(const in vec3 v, const in vec3 u)
{
    return max( dot(v,u), 0.0f );
}

vec3 ComputePerezLuminanceYxy(const in float theta, const in float gamma)
{
    const float kBias = 1e-3f;
    return (1.f + params.A * exp( params.B / (cos(theta)+kBias) ) ) *
           (1.f + params.C * exp( params.D * gamma) +
            params.E * cos(gamma) * cos(gamma) );
}

vec3 ComputeSkyLuminance(const in vec3 kSunDir, const in vec3 kViewDir)
{
    const float thetaView = acos( SaturateDot(kViewDir, vec3(0,1,0)) );
    const float gammaView = acos( SaturateDot(kSunDir, kViewDir) );

    const vec3 fThetaGamma = ComputePerezLuminanceYxy(thetaView, gammaView);
    
    return params.ZenithLum * (fThetaGamma / params.ZeroThetaSun);
}

vec3 YxyToRGB(const in vec3 Yxy);

void main()
{
    const vec2 kRes = params.resolution;
    const vec2 uv = vec2(gl_FragCoord.x, kRes.y - gl_FragCoord.y);
    // UVs to [-1,1]
    const vec2 p = (2.0f * uv - kRes.xy) / kRes.y;

    const vec3 kViewDir = normalize(
        params.camView * normalize( vec3(p, params.camFOV) )
    );

    const vec3 kSunDir = normalize(params.sunDir);
    const vec3 kSunDisk = params.sunColor *
                          smoothstep(0.997f, 1.0f,
                                     SaturateDot(kViewDir, kSunDir) );

    const vec3 kSkyLuminance = YxyToRGB( ComputeSkyLuminance(kSunDir, kViewDir) );

    fragColor = vec4(kSkyLuminance * 0.05f + kSunDisk * kSkyLuminance, 1.0);

    // Tone mapping
    fragColor = vec4(1.0) - exp(-fragColor * 2.0f);
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
