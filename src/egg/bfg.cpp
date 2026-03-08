#include "egg/bfg.h"
    inline glm::vec4 RGBA8_to_vec4(uint32_t n) {
        float r = float((n >> 24) & 0xFF) / 255.0f;
        float g = float((n >> 16) & 0xFF) / 255.0f;
        float b = float((n >> 8) & 0xFF) / 255.0f;
        float a = float((n >> 0) & 0xFF) / 255.0f;
        return glm::vec4(r, g, b, a);
    }

    fog::BFG fog::ParseBFG(bStream::CStream* stream,size_t size)
    {
        const size_t entrySize = 0x1C;
        size_t entryCount = size / entrySize;

        BFG out{};
        out.entries.reserve(entryCount);

        for (size_t i = 0; i < entryCount; i++) {
            size_t base = i * entrySize;
            stream->seek(base);
            BFGEntry e{};
            e.type = stream->readInt32();
            e.startZ = stream->readFloat();
            e.endZ = stream->readFloat();
            e.color = RGBA8_to_vec4(stream->readUInt32());
            e.rangeCorrection = stream->readUInt16();
            e.rangeCenter = stream->readUInt16();
            e.fadeSpeed = stream->readFloat();
            e.unk18 = stream->readUInt16();
            e.unk1A = stream->readUInt16();

            out.entries.push_back(e);
        }

        return out;
    }