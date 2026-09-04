precision mediump float;

in vec3 v_worldPos;
in vec3 v_worldNormal;
in vec2 v_uv;
in vec4 v_worldTangent;

uniform sampler2D u_baseColorTexture;
uniform sampler2D u_metallicRoughnessTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_occlusionTexture;
uniform sampler2D u_emissiveTexture;
uniform sampler2D u_irradianceTexture;
uniform sampler2D u_specularTexture;
uniform sampler2D u_brdfLUTTexture;

const int MAX_SPECULAR_MIPS = 8;

layout(std140) uniform Scene3DFrame {
    mat4 u_projectionView;
    vec4 u_cameraWorldPos;
    vec4 u_sunDirection;
    vec4 u_sunColorIntensity;
    vec4 u_ambientColorIntensity;
    vec4 u_iblParams;
    ivec4 u_frameFlags;
    ivec4 u_specularMipInfo[MAX_SPECULAR_MIPS];
};

layout(std140) uniform Scene3DDraw {
    mat4 u_modelMatrix;
};

layout(std140) uniform Scene3DMaterial {
    vec4 u_baseColorFactor;
    vec4 u_emissiveFactor;
    vec4 u_materialParams;
    vec4 u_alphaParams;
    ivec4 u_materialFlags0;
    ivec4 u_materialFlags1;
};

out vec4 fragColor;

const float PI = 3.14159265358979323846;
const float SPECULAR_AA_STRENGTH = 0.35;
const float SPECULAR_AA_MAX_VARIANCE = 0.18;

bool hasIBL() { return u_frameFlags.x != 0; }
int specularMipCount() { return u_frameFlags.y; }
bool alphaMaskEnabled() { return u_materialFlags0.x != 0; }
bool doubleSidedEnabled() { return u_materialFlags0.y != 0; }
bool hasBaseColorTexture() { return u_materialFlags0.z != 0; }
bool hasMetallicRoughnessTexture() { return u_materialFlags0.w != 0; }
bool hasNormalTexture() { return u_materialFlags1.x != 0; }
bool hasOcclusionTexture() { return u_materialFlags1.y != 0; }
bool hasEmissiveTexture() { return u_materialFlags1.z != 0; }

vec3 srgbToLinear(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(2.2));
}

vec3 linearToSrgb(vec3 c) {
    return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2));
}

vec3 decodeRGBM(vec4 rgbm) {
    return rgbm.rgb * (rgbm.a * u_iblParams.x);
}

vec3 toneMapACES(vec3 color) {
    vec3 a = color * (2.51 * color + 0.03);
    vec3 b = color * (2.43 * color + 0.59) + 0.14;
    return clamp(a / max(b, vec3(0.00001)), vec3(0.0), vec3(1.0));
}

float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 0.00001);
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec2 directionToEquirectUV(vec3 dir) {
    vec3 n = normalize(dir);
    float phi = atan(n.z, n.x);
    float theta = acos(clamp(n.y, -1.0, 1.0));
    float u = fract(phi / (2.0 * PI) + 0.5);
    float v = clamp(theta / PI, 0.0, 1.0);
    return vec2(u, v);
}

vec3 sampleIrradianceIBL(vec3 normal) {
    return decodeRGBM(texture(u_irradianceTexture, directionToEquirectUV(normal)));
}

vec3 sampleSpecularAtlasLevel(vec3 dir, int mipIndex) {
    if (specularMipCount() <= 0) {
        return vec3(0.0);
    }

    int clampedMip = clamp(mipIndex, 0, specularMipCount() - 1);
    int rectWidth = u_specularMipInfo[clampedMip].x;
    int rectHeight = u_specularMipInfo[clampedMip].y;
    int offsetY = u_specularMipInfo[clampedMip].z;
    if (rectWidth <= 0 || rectHeight <= 0) {
        return vec3(0.0);
    }

    ivec2 atlasSize = textureSize(u_specularTexture, 0);
    vec2 rectSize = vec2(float(rectWidth), float(rectHeight));
    vec2 localUv = clamp(directionToEquirectUV(dir), vec2(0.0), vec2(1.0));
    vec2 localPixel = localUv * max(rectSize - 1.0, vec2(0.0)) + 0.5;
    vec2 atlasUv = vec2(
        localPixel.x / float(atlasSize.x),
        (float(offsetY) + localPixel.y) / float(atlasSize.y)
    );
    return decodeRGBM(texture(u_specularTexture, atlasUv));
}

vec3 samplePrefilteredSpecular(vec3 reflectionDir, float roughness) {
    if (specularMipCount() <= 1) {
        return sampleSpecularAtlasLevel(reflectionDir, 0);
    }

    float mip = clamp(roughness, 0.0, 1.0) * float(specularMipCount() - 1);
    int mip0 = int(floor(mip));
    int mip1 = min(mip0 + 1, specularMipCount() - 1);
    float t = mip - float(mip0);
    vec3 c0 = sampleSpecularAtlasLevel(reflectionDir, mip0);
    vec3 c1 = sampleSpecularAtlasLevel(reflectionDir, mip1);
    return mix(c0, c1, t);
}

vec3 getGeometryNormal() {
    vec3 dp1 = dFdx(v_worldPos);
    vec3 dp2 = dFdy(v_worldPos);
    vec3 ng = normalize(cross(dp1, dp2));
    if (doubleSidedEnabled() && !gl_FrontFacing) {
        ng = -ng;
    }
    return ng;
}

mat3 computeTBN(vec3 N) {
    if (dot(v_worldTangent.xyz, v_worldTangent.xyz) > 1e-6) {
        vec3 T = normalize(v_worldTangent.xyz - N * dot(N, v_worldTangent.xyz));
        vec3 B = normalize(cross(N, T)) * v_worldTangent.w;
        return mat3(T, B, N);
    }

    vec3 dp1 = dFdx(v_worldPos);
    vec3 dp2 = dFdy(v_worldPos);
    vec2 duv1 = dFdx(v_uv);
    vec2 duv2 = dFdy(v_uv);

    vec3 dp2perp = cross(dp2, N);
    vec3 dp1perp = cross(N, dp1);
    vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;

    float denom = max(max(dot(T, T), dot(B, B)), 1e-8);
    float invScale = inversesqrt(denom);
    return mat3(T * invScale, B * invScale, N);
}

vec3 resolveNormal() {
    vec3 N = v_worldNormal;
    if (dot(N, N) <= 1e-6) {
        N = getGeometryNormal();
    } else {
        N = normalize(N);
        if (doubleSidedEnabled() && !gl_FrontFacing) {
            N = -N;
        }
    }

    if (!hasNormalTexture()) {
        return N;
    }

    vec3 tangentNormal = texture(u_normalTexture, v_uv).xyz * 2.0 - 1.0;
    tangentNormal.xy *= u_materialParams.z;
    tangentNormal = normalize(tangentNormal);
    return normalize(computeTBN(N) * tangentNormal);
}

float applySpecularAA(float roughness, vec3 normal) {
    vec3 dndx = dFdx(normal);
    vec3 dndy = dFdy(normal);
    float normalVariance = SPECULAR_AA_STRENGTH * (dot(dndx, dndx) + dot(dndy, dndy));
    float kernelRoughness2 = clamp(normalVariance, 0.0, SPECULAR_AA_MAX_VARIANCE);
    float roughness2 = roughness * roughness;
    return clamp(sqrt(roughness2 + kernelRoughness2), 0.045, 1.0);
}

void main() {
    vec4 baseColor = u_baseColorFactor;
    if (hasBaseColorTexture()) {
        vec4 texColor = texture(u_baseColorTexture, v_uv);
        baseColor *= vec4(srgbToLinear(texColor.rgb), texColor.a);
    }

    if (alphaMaskEnabled() && baseColor.a < u_alphaParams.x) {
        discard;
    }

    float metallic = clamp(u_materialParams.x, 0.0, 1.0);
    float roughness = clamp(u_materialParams.y, 0.045, 1.0);
    if (hasMetallicRoughnessTexture()) {
        vec4 mrSample = texture(u_metallicRoughnessTexture, v_uv);
        roughness *= mrSample.g;
        metallic *= mrSample.b;
    }
    roughness = clamp(roughness, 0.045, 1.0);

    float ao = 1.0;
    if (hasOcclusionTexture()) {
        float occ = texture(u_occlusionTexture, v_uv).r;
        ao = mix(1.0, occ, clamp(u_materialParams.w, 0.0, 1.0));
    }

    vec3 emissive = u_emissiveFactor.rgb;
    if (hasEmissiveTexture()) {
        emissive *= srgbToLinear(texture(u_emissiveTexture, v_uv).rgb);
    }

    vec3 N = resolveNormal();
    float specularRoughness = applySpecularAA(roughness, N);
    vec3 V = normalize(u_cameraWorldPos.xyz - v_worldPos);
    vec3 L = normalize(-u_sunDirection.xyz);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 albedo = clamp(baseColor.rgb, vec3(0.0), vec3(1.0));
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float D = distributionGGX(NdotH, specularRoughness);
    float G = geometrySmith(NdotV, NdotL, specularRoughness);
    vec3 F = fresnelSchlick(HdotV, F0);

    vec3 numerator = D * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.0001);
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 diffuse = kD * albedo / PI;

    vec3 radiance = u_sunColorIntensity.rgb * u_sunColorIntensity.a;
    vec3 directLighting = (diffuse + specular) * radiance * NdotL;

    vec3 ambientLighting;
    if (hasIBL()) {
        vec3 irradiance = sampleIrradianceIBL(N);
        vec3 reflectionDir = reflect(-V, N);
        vec3 prefilteredSpecular = samplePrefilteredSpecular(reflectionDir, specularRoughness);
        vec2 brdf = texture(u_brdfLUTTexture, vec2(clamp(NdotV, 0.0, 1.0), specularRoughness)).rg;
        vec3 ambientFresnel = fresnelSchlickRoughness(NdotV, F0, specularRoughness);

        vec3 indirectDiffuse = irradiance * albedo * kD;
        vec3 indirectSpecular = prefilteredSpecular * (ambientFresnel * brdf.x + brdf.y);

        vec3 iblTint = mix(vec3(1.0), max(u_ambientColorIntensity.rgb, vec3(0.0)), clamp(u_ambientColorIntensity.a, 0.0, 1.0));
        ambientLighting = (indirectDiffuse + indirectSpecular) * ao * iblTint * max(u_iblParams.y, 0.0);
    } else {
        float skyFactor = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
        vec3 ambientTint = mix(u_ambientColorIntensity.rgb * 0.45, u_ambientColorIntensity.rgb, skyFactor);
        vec3 ambientFresnel = fresnelSchlickRoughness(NdotV, F0, specularRoughness);
        vec3 ambientDiffuse = kD * albedo * ambientTint * u_ambientColorIntensity.a;
        vec3 ambientSpecular = ambientFresnel * ambientTint * u_ambientColorIntensity.a * mix(0.10, 0.35, 1.0 - specularRoughness);
        ambientLighting = (ambientDiffuse + ambientSpecular) * ao;
    }

    vec3 color = ambientLighting + directLighting + emissive;
    color = toneMapACES(color);
    color = linearToSrgb(color);

    fragColor = vec4(color, baseColor.a);
}

