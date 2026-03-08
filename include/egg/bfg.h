#pragma once
#include "bstream.h"
#include "vector"
#include "glm/glm.hpp"
class fog {
public:
    struct BFGEntry {
        int32_t type;
        float   startZ;
        float   endZ;
        glm::vec4 color;
        uint16_t rangeCorrection;
        uint16_t rangeCenter;
        float   fadeSpeed;
        uint16_t unk18;
        uint16_t unk1A;
    };

    struct BFG {
        std::vector<BFGEntry> entries;
    };
    fog::BFG ParseBFG(bStream::CStream* stream, size_t size);
};