#include "egg/blight.h"
blight::EggBinaryLightResource blight::ParseBLIGHT(bStream::CStream* stream) {
    EggBinaryLightResource res;

    stream->seek(0x10);
    uint16_t lightObjCount = stream->readUInt16();
    uint16_t ambientLightCount = stream->readUInt16();

    size_t lightObjTableIdx = 0x28;
    for (int i = 0; i < lightObjCount; ++i) {
        EggBinaryLightObjectResource obj{};

        stream->seek(lightObjTableIdx + 0x10);
        obj.spotFunction = static_cast<EGXSpotFunction>(stream->readUInt8());
        obj.distAttnFunction = static_cast<GXDistAttnFunction>(stream->readUInt8());
        obj.lightType = static_cast<EggBinaryLightType>(stream->readUInt8());

        obj.ambientLightIndex = stream->readUInt16();
        obj.flags = static_cast<EggBinaryLightFlags>(stream->readUInt16());

        obj.pos = glm::vec3(
            stream->readFloat(),
            stream->readFloat(),
            stream->readFloat()
        );
        obj.aim = glm::vec3(
            stream->readFloat(),
            stream->readFloat(),
            stream->readFloat()
        );

        float intensity = stream->readFloat();
        float r = stream->readUInt8() / 255.0f;
        float g = stream->readUInt8() / 255.0f;
        float b = stream->readUInt8() / 255.0f;
        float a = stream->readUInt8() / 255.0f;
        obj.color = glm::vec4(
            glm::clamp(intensity * r, 0.0f, 1.0f),
            glm::clamp(intensity * g, 0.0f, 1.0f),
            glm::clamp(intensity * b, 0.0f, 1.0f),
            a
        );

        uint32_t spec = stream->readUInt32();
        obj.specColor = glm::vec4(
            ((spec >> 24) & 0xFF) / 255.0f,
            ((spec >> 16) & 0xFF) / 255.0f,
            ((spec >> 8) & 0xFF) / 255.0f,
            ((spec >> 0) & 0xFF) / 255.0f
        );

        obj.spotCutoff = stream->readFloat();
        obj.refDist = stream->readFloat();
        obj.refBrightness = stream->readFloat();

        res.lightObjects.push_back(obj);
        lightObjTableIdx += 0x50;
    }

    size_t ambientLightTableIdx = lightObjTableIdx;
    for (int i = 0; i < ambientLightCount; ++i) {
        stream->seek(ambientLightTableIdx);
        uint32_t c = stream->readUInt32();
        glm::vec4 col(
            ((c >> 24) & 0xFF) / 255.0f,
            ((c >> 16) & 0xFF) / 255.0f,
            ((c >> 8) & 0xFF) / 255.0f,
            ((c >> 0) & 0xFF) / 255.0f
        );
        res.ambientLights.push_back(col);
        ambientLightTableIdx += 0x08;
    }

    return res;
}