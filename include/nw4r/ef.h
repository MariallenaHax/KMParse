#pragma once
#include "GX/GXMaterial.h"
class reff {
    enum class ShapeType : uint8_t {
        Point,
        Line,
        Free,
        Billboard,
        Directional,
        Stripe,
    };

    enum class RotType : uint8_t {
        X,
        Y,
        Z,
        XYZ,
    };

    enum class MaterialLightType : uint8_t {
        None,
        Ambient,
        Point,
    };

    struct MaterialLight {
        uint8_t mode;
        MaterialLightType type;
        Color amb;
        Color dif;
        float radius;
        glm::vec3 pos;
    };

    struct MaterialSettings {
        GXMaterial material;

        MaterialLight light;
        glm::mat4 indTexMtx;

        ShapeType shapeType;
        uint8_t shapeOption;
        uint8_t shapeDir;
        RotType shapeAxis;
        uint32_t shapeFlags;

        int8_t pivotX;
        int8_t pivotY;
    };
    struct ParticleSettings {
        Color colors[4];

        float sizeX, sizeY;
        float scaleX, scaleY;

        float rotationX, rotationY, rotationZ;

        float texScale0S, texScale0T;
        float texScale1S, texScale1T;
        float texScale2S, texScale2T;

        float texRotation0, texRotation1, texRotation2;

        float texTranslation0S, texTranslation0T;
        float texTranslation1S, texTranslation1T;
        float texTranslation2S, texTranslation2T;

        uint16_t textureWrapMode;
        uint8_t textureMirror;

        uint8_t alphaRef0, alphaRef1;

        uint8_t rotationRandom0, rotationRandom1, rotationRandom2;

        float rotation0, rotation1, rotation2;

        std::string textureNames[3];
    };
    enum class TrackDataType : uint8_t {
        U8 = 0,
        POSTFIELD = 2,
        F32 = 3,
        TEXTURE = 4,
        CHILD = 5,
        ROTATE = 6,
        FIELD = 7,
        EMITTER_F32 = 11,
    };

    struct TrackInfo {
        uint8_t target;
        TrackDataType dataType;
    };

    TrackInfo parseTrackInfo(const uint8_t* buf, size_t size) {
        assert(buf[0] == 0xAC);
        TrackInfo ti{};
        ti.target = buf[1];
        ti.dataType = static_cast<TrackDataType>(buf[2]);
        return ti;
    }
    enum class VolumeType : uint8_t {
        Circle = 0x00,
        Line = 0x01,
        Cube = 0x05,
        Cylinder = 0x07,
        Sphere = 0x08,
        Point = 0x09,
        Torus = 0x0A,
    };

    enum class EmitterSimFlags : uint32_t {
        Forever = 1 << 2,
    };

    enum class EmitFlags : uint32_t {
        FixedInterval = 0x00000200,
        FixedPosition = 0x00000400,

        Disc_FixedDensity = 0x00010000,
        Disc_SameSize = 0x00020000,
    };

    struct EmitterSettings {
        uint32_t emitterSimFlags;
        VolumeType volumeType;
        EmitFlags emitFlags;

        uint16_t maxFrame;
        uint16_t lifeTime;
        float lifeTimeRndm;
        int8_t particleChildInheritTranslation;

        float rateStepRndm;
        int8_t rateRndm;
        float rate;

        uint16_t startFrame;
        uint16_t prerollTime;
        uint16_t rateStep;
        int8_t particleInheritTranslation;
        int8_t emitterChildInheritTranslation;

        float volumeParams[6];

        uint16_t divNumber;
        int8_t initialVelRatio;
        float momentRndm;

        float initialVelOmni;
        float initialVelAxis;
        float initialVelRndm;
        float initialVelNrm;
        float diffuseVelNrm;

        float initialVelDir;
        float diffuseVelDir;

        glm::vec3 emitterDir;
        glm::vec3 emitterScl;
        glm::vec3 emitterRot;
        glm::vec3 emitterTrs;

        uint8_t lodNear;
        uint8_t lodFar;
        uint8_t lodMinEmit;
        uint8_t lodAlpha;

        uint32_t randomSeed;
    };
    struct EfEffectResourceData {
        std::string name;

        EmitterSettings emitterSettings;
        MaterialSettings materialSettings;
        ParticleSettings particleSettings;

        std::vector<TrackInfo> particleTrackInfos;
        std::vector<TrackInfo> emitterTrackInfos;
    };

    struct BREFF {
        std::string name;
        std::vector<EfEffectResourceData> effects;
    };

    struct BREFTTexture {
        std::string name;
        uint8_t format;
        uint16_t width;
        uint16_t height;
        std::vector<uint8_t> data;
        int mipCount;
        uint8_t paletteFormat;
        std::vector<uint8_t> paletteData;
    };

    struct BREFT {
        std::string name;
        std::vector<BREFTTexture> textures;
    };
    reff::MaterialSettings parseMaterialSettings(bStream::CStream* stream, size_t p);
    reff::EfEffectResourceData parseEffectResource(bStream::CStream* stream, size_t effectDataOffs, size_t effectDataSize, const std::string& name);
    reff::EmitterSettings parseEmitterSettings(bStream::CStream* stream, size_t p);
    reff::ParticleSettings parseParticleSettings(bStream::CStream* stream, size_t p);
    reff::BREFF parseBREFF(bStream::CStream* stream);
    reff::BREFT parseBREFT(bStream::CStream* stream);
};