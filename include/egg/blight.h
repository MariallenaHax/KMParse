#pragma once
#include "../nw4r/brres.h"
class blight {
public:
    enum class GXDistAttnFunction : uint8_t {
        OFF = 0,
        GENTLE = 1,
        MEDIUM = 2,
        STEEP = 3,
    };
    enum class EggBinaryLightFlags : uint16_t {
        NONE = 0,
        ENABLE = 1 << 0,
        ENABLE_G3D = 1 << 5,
        ENABLE_GX = 1 << 6,
        USE_CUTOFF = 1 << 7,
        MANUAL_DISTANCE_ATTN = 1 << 8,
        ENABLE_G3D_COLOR = 1 << 9,
        ENABLE_G3D_ALPHA = 1 << 10,
    };

    enum class EggBinaryLightType : uint8_t {
        POINT = 0x00,
        DIRECTIONAL = 0x01,
        SPOT = 0x02,
    };

    struct EggBinaryLightObjectResource {
        EGXSpotFunction spotFunction;
        GXDistAttnFunction distAttnFunction;
        EggBinaryLightType lightType;

        uint16_t ambientLightIndex;
        EggBinaryLightFlags flags;

        glm::vec3 pos;
        glm::vec3 aim;

        glm::vec4 color;
        glm::vec4 specColor;

        float refDist;
        float refBrightness;
        float spotCutoff;
    };

    struct EggBinaryLightResource {
        std::vector<EggBinaryLightObjectResource> lightObjects;
        std::vector<glm::vec4> ambientLights;
    };

    inline static glm::vec3 computeGXDistAttn(float refDist, float refBrightness, GXDistAttnFunction func) {
        if (func == GXDistAttnFunction::OFF || refDist <= 0.0f || refBrightness <= 0.0f)
            return glm::vec3(1.0f, 0.0f, 0.0f); // 1 / (1)

        float k = (1.0f - refBrightness) / refBrightness;
        float a0 = 1.0f;
        float a1 = 0.0f;
        float a2 = 0.0f;

        switch (func) {
        case GXDistAttnFunction::GENTLE:
            a1 = k / refDist;
            break;
        case GXDistAttnFunction::MEDIUM:
            a2 = k / (refDist * refDist);
            break;
        case GXDistAttnFunction::STEEP:
            a1 = 2.0f * k / refDist;
            a2 = k / (refDist * refDist);
            break;
        default:
            break;
        }

        return glm::vec3(a0, a1, a2);
    }

    inline static glm::vec3 computeGXSpotAttn(float spotCutoffDeg, EGXSpotFunction func) {
        if (func == EGXSpotFunction::OFF || spotCutoffDeg <= 0.0f || spotCutoffDeg >= 90.0f)
            return glm::vec3(1.0f, 0.0f, 0.0f); // èÌÇ…1

        float cutoffRad = glm::radians(spotCutoffDeg);
        float c = std::cos(cutoffRad);

        float k0 = 0.0f, k1 = 0.0f, k2 = 0.0f;

        switch (func) {
        case EGXSpotFunction::FLAT:
            k0 = 1.0f; k1 = 0.0f; k2 = 0.0f;
            break;

        case EGXSpotFunction::COS:
            k0 = 0.5f - 0.5f * c;
            k1 = 0.5f;
            k2 = 0.0f;
            break;

        case EGXSpotFunction::COS2:
            k0 = -c * c;
            k1 = 2.0f * c;
            k2 = -1.0f;
            break;

        case EGXSpotFunction::SHARP:
            k0 = 0.0f;
            k1 = 0.0f;
            k2 = 1.0f / (1.0f - c * c);
            break;

        case EGXSpotFunction::RING1:
            k0 = -1.0f / (1.0f - c);
            k1 = 1.0f / (1.0f - c);
            k2 = 0.0f;
            break;

        case EGXSpotFunction::RING2:
            k0 = 0.0f;
            k1 = -1.0f / (1.0f - c);
            k2 = 1.0f / (1.0f - c);
            break;

        default:
            k0 = 1.0f; k1 = 0.0f; k2 = 0.0f;
            break;
        }

        return glm::vec3(k0, k1, k2);
    }
    struct EggBinaryLight {
        EggBinaryLightFlags flags{};
        glm::vec4 color{ 1,1,1,1 };
        EggBinaryLightType lightType{ EggBinaryLightType::DIRECTIONAL };
        glm::vec3 pos{};
        glm::vec3 aim{};
        float spotCutoff{};
        EGXSpotFunction spotFunction{ EGXSpotFunction::OFF };
        float refDist{};
        float refBrightness{};
        GXDistAttnFunction distAttnFunction{ GXDistAttnFunction::OFF };

        void copy(const EggBinaryLightObjectResource& r) {
            flags = r.flags;
            color = r.color;
            lightType = r.lightType;
            pos = r.pos;
            aim = r.aim;
            spotCutoff = r.spotCutoff;
            spotFunction = r.spotFunction;
            refDist = r.refDist;
            refBrightness = r.refBrightness;
            distAttnFunction = r.distAttnFunction;
        }

        void initG3DLightObj(bres::Light& out) const {
            if (!(int(flags) & int(EggBinaryLightFlags::ENABLE)) ||
                !(int(flags) & int(EggBinaryLightFlags::ENABLE_G3D))) {
                out.color = glm::vec4(0.0f);
                out.isDirectional = false;
                out.distAtten = glm::vec3(1.0f, 0.0f, 0.0f);
                out.cosAtten = glm::vec3(1.0f, 0.0f, 0.0f);
                return;
            }

            out.color = color;

            switch (lightType) {
            case EggBinaryLightType::POINT:
                out.position = pos;
                out.direction = glm::vec3(0.0f);
                out.isDirectional = false;
                break;

            case EggBinaryLightType::DIRECTIONAL: {
                glm::vec3 dir = glm::normalize(aim - pos);
                glm::vec3 posInf = -dir * 1e10f; // ñ≥å¿âì
                out.position = posInf;
                out.direction = dir;
                out.isDirectional = true;
                break;
            }

            case EggBinaryLightType::SPOT:
                out.position = pos;
                out.direction = glm::normalize(aim - pos);
                out.isDirectional = false;
                break;
            }

            switch (distAttnFunction) {
            case GXDistAttnFunction::OFF:
                out.distAtten = glm::vec3(1.0f, 0.0f, 0.0f);
                break;

            default:
                out.distAtten = computeGXDistAttn(refDist, refBrightness, distAttnFunction);
                break;
            }

            switch (spotFunction) {
            case EGXSpotFunction::OFF:
                out.cosAtten = glm::vec3(1.0f, 0.0f, 0.0f);
                break;

            default:
                out.cosAtten = computeGXSpotAttn(spotCutoff, spotFunction);
                break;
            }
        }
    };
    struct EggLightManager {
        EggBinaryLightResource res;
        std::vector<EggBinaryLight> lights;

        bres::LightSet lightSet;

        EggLightManager(const EggBinaryLightResource& r)
            : res(r)
        {
            lightSet.lights.resize(res.lightObjects.size());
            lightSet.ambient.resize(res.ambientLights.size());
                lightSet.ambientIndexForLight.resize(res.lightObjects.size());

            for (size_t i = 0; i < res.lightObjects.size(); ++i) {
                EggBinaryLight l;
                l.copy(res.lightObjects[i]);
                lights.push_back(l);

                l.initG3DLightObj(lightSet.lights[i]);

                uint16_t ambIdx = res.lightObjects[i].ambientLightIndex;
                if (ambIdx < res.ambientLights.size())
                    lightSet.ambientIndexForLight[i] = ambIdx;
                else
                    lightSet.ambientIndexForLight[i] = -1;
            }

            for (size_t i = 0; i < res.ambientLights.size(); ++i)
                lightSet.ambient[i] = res.ambientLights[i];
        }
    };
    blight::EggBinaryLightResource ParseBLIGHT(bStream::CStream* stream);
};