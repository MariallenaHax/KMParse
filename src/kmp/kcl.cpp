#include "kmp/kcl.h"
#include <algorithm>

SKclIO::SKclIO() {

}

SKclIO::KCL SKclIO::CreateKCLFromPrisms(
    KCLPrism prism,
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec3>& normals
) {
    PrismWorld world = ToWorldPrism(prism, positions, normals);

    glm::vec3 CrossA = glm::cross(world.normal1, world.direction);
    glm::vec3 CrossB = glm::cross(world.normal2, world.direction);

    glm::vec3 v1 = world.base;
    glm::vec3 v2 = world.base + CrossB * (world.length / glm::dot(CrossB, world.normal3));
    glm::vec3 v3 = world.base + CrossA * (world.length / glm::dot(CrossA, world.normal3));

    KCL tri;
    tri.v1 = v1;
    tri.v2 = v2;
    tri.v3 = v3;
    tri.normal = world.normal3;
    tri.flags = prism.mFlags;

    return tri;
}
SKclIO::~SKclIO()
{

}
void SKclIO::Reset()
{
    mPositions.clear();
    mNormals.clear();
    mPrisms.clear();
    mKCLs.clear();
    verts.clear();

    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
}

void SKclIO::Load(bStream::CStream* stream) {
    uint32_t positionDataOffs = stream->readUInt32();
    uint32_t normDataOffs = stream->readUInt32();
    uint32_t prismDataOffs = stream->readUInt32();
    uint32_t blockDataOffs = stream->readUInt32();
    float prismThickness = stream->readFloat();
    glm::vec3 areaMinPos = { stream->readFloat(), stream->readFloat(), stream->readFloat() };
    glm::vec3 coordMask = { stream->readUInt32(), stream->readUInt32(), stream->readUInt32() };
    glm::vec3 coordShift = { stream->readUInt32(), stream->readUInt32(), stream->readUInt32() };
    float sphereRadius = stream->readFloat();
    mPositions.reserve(normDataOffs - positionDataOffs);
    mNormals.reserve((prismDataOffs + 0x10) - normDataOffs);

    stream->seek(positionDataOffs);
    for (int i = 0; i < (normDataOffs - positionDataOffs) / 12; i++) {
        mPositions.push_back({ stream->readFloat(), stream->readFloat(), stream->readFloat() });
    }

    stream->seek(normDataOffs);
    for (int i = 0; i < ((prismDataOffs + 0x10) - normDataOffs) / 12; i++) {
        mNormals.push_back({ stream->readFloat(), stream->readFloat(), stream->readFloat() });
    }

    stream->seek(prismDataOffs + 0x10);
    size_t prismCount = (blockDataOffs - (prismDataOffs + 0x10)) / 16;
    for (size_t i = 0; i < prismCount; ++i) {
        KCLPrism prism;
        prism.mLength = stream->readFloat();
        prism.mPositionIdx = stream->readUInt16();
        prism.mDirectionIdx = stream->readUInt16();
        prism.mNormal1Idx = stream->readUInt16();
        prism.mNormal2Idx = stream->readUInt16();
        prism.mNormal3Idx = stream->readUInt16();
        prism.mFlags = stream->readUInt16();
        mPrisms.push_back(prism);
    }
    for (auto& prism : mPrisms) {
        mKCLs.push_back(CreateKCLFromPrisms(prism, mPositions, mNormals));
    }

    for (auto& tri : mKCLs) {
        glm::vec4 col = GetKCLColor(tri.flags);

        mTris.push_back({
            tri.v1,
            tri.v2,
            tri.v3,
            tri.normal,
            col
            });
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(Vertex),
        verts.data(),
        GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, pos)
    );

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        1, 3, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, normal)
    );

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(
        2, 4, GL_FLOAT, GL_FALSE,
        sizeof(Vertex),
        (void*)offsetof(Vertex, color)
    );
}
static const std::vector<SKclIO::KCLColorEntry> KCL_COLORS = {
    { {1,0,0,0,0}, {1.0,1.0,1.0,1.0}, 0, "Road" },
    { {1,0,0,0,0}, {1.0,0.9,0.8,1.0}, 0, "Slippery Road (sand/dirt)" },
    { {1,0,0,0,0}, {0.0,0.8,0.0,1.0}, 0, "Weak Off-Road" },
    { {1,0,0,0,0}, {0.0,0.6,0.0,1.0}, 0, "Off-Road" },
    { {1,0,0,0,0}, {0.0,0.4,0.0,1.0}, 0, "Heavy Off-Road" },
    { {1,0,0,0,0}, {0.8,0.9,1.0,1.0}, 0, "Slippery Road (ice)" },
    { {1,0,0,0,0}, {1.0,0.5,0.0,1.0}, 0, "Boost Panel" },
    { {1,0,0,0,0}, {1.0,0.6,0.0,1.0}, 0, "Boost Ramp" },
    { {1,0,0,0,0}, {1.0,0.8,0.0,1.0}, 0, "Slow Ramp" },

    { {0,0,0,0,1}, {0.9,0.9,1.0,0.5}, 2, "Item Road" },

    { {1,0,0,0,0}, {0.7,0.1,0.1,1.0}, 0, "Solid Fall" },
    { {1,0,0,0,0}, {0.0,0.5,1.0,1.0}, 0, "Moving Water" },

    { {0,0,0,1,0}, {0.6,0.6,0.6,1.0}, 0, "Wall" },
    { {0,1,0,1,0}, {0.0,0.0,0.6,0.8}, 3, "Invisible Wall" },
    { {0,0,0,0,1}, {0.6,0.6,0.7,0.5}, 2, "Item Wall" },
    { {0,0,0,1,0}, {0.6,0.6,0.6,1.0}, 0, "Wall 2" },

    { {1,0,0,0,0}, {0.8,0.0,0.0,0.8}, 4, "Fall Boundary" },

    { {0,0,1,0,0}, {1.0,0.0,0.5,0.8}, 1, "Cannon Activator" },
    { {0,0,1,0,0}, {0.5,0.0,1.0,0.5}, 1, "Force Recalculation" },

    { {1,0,0,0,0}, {0.0,0.3,1.0,1.0}, 0, "Half-pipe Ramp" },

    { {0,1,0,1,0}, {0.8,0.4,0.0,0.8}, 1, "Player-Only Wall" },

    { {1,0,0,0,0}, {0.9,0.9,1.0,1.0}, 0, "Moving Road" },
    { {1,0,0,0,0}, {0.9,0.7,1.0,1.0}, 0, "Sticky Road" },
    { {1,0,0,0,0}, {1.0,1.0,1.0,1.0}, 0, "Road 2" },

    { {0,0,1,0,0}, {1.0,0.0,1.0,0.8}, 1, "Sound Trigger" },
    { {0,1,0,1,0}, {0.4,0.6,0.4,0.8}, 1, "Weak Wall" },
    { {0,0,1,0,0}, {0.8,0.0,1.0,0.8}, 1, "Effect Trigger" },
    { {0,0,1,0,0}, {1.0,0.0,1.0,0.5}, 1, "Item State Modifier" },

    { {0,1,0,1,0}, {0.0,0.6,0.0,0.8}, 3, "Half-pipe Invis Wall" },

    { {1,0,0,0,0}, {0.9,0.9,1.0,1.0}, 0, "Rotating Road" },
    { {0,0,0,1,0}, {0.8,0.7,0.8,1.0}, 0, "Special Wall" },
    { {0,1,0,1,0}, {0.0,0.0,0.6,0.8}, 3, "Invisible Wall 2" },
};
glm::vec4 SKclIO::GetKCLColor(uint16_t flags)
{
    uint16_t basicType = flags & 0x1F;
    if (basicType >= 32)
        return { 1,1,1,1 };

    return KCL_COLORS[basicType].color;
}
void SKclIO::Render(glm::mat4 vp, const glm::vec3& camPos)
{
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    std::vector<Vertex> opaqueVerts;
    std::vector<std::pair<float, KclTri>> transparentTris;

    opaqueVerts.reserve(mTris.size() * 3);

    for (auto& t : mTris) {
        if (t.color.a >= 1.0f) {
            opaqueVerts.push_back({ t.v1, t.normal, t.color });
            opaqueVerts.push_back({ t.v2, t.normal, t.color });
            opaqueVerts.push_back({ t.v3, t.normal, t.color });
        }
        else {
            glm::vec3 center = (t.v1 + t.v2 + t.v3) / 3.0f;
            float dist = glm::distance(camPos, center);
            transparentTris.emplace_back(dist, t);
        }
    }

    std::sort(transparentTris.begin(), transparentTris.end(),
        [](auto& a, auto& b) { return a.first > b.first; });

    std::vector<Vertex> transparentVerts;
    transparentVerts.reserve(transparentTris.size() * 3);
    for (auto& [d, t] : transparentTris) {
        transparentVerts.push_back({ t.v1, t.normal, t.color });
        transparentVerts.push_back({ t.v2, t.normal, t.color });
        transparentVerts.push_back({ t.v3, t.normal, t.color });
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
        opaqueVerts.size() * sizeof(Vertex),
        opaqueVerts.data(),
        GL_DYNAMIC_DRAW);

    glUseProgram(kclProgram);
    glUniformMatrix4fv(glGetUniformLocation(kclProgram, "u_vp"), 1, GL_FALSE, glm::value_ptr(vp));
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, opaqueVerts.size());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBufferData(GL_ARRAY_BUFFER,
        transparentVerts.size() * sizeof(Vertex),
        transparentVerts.data(),
        GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, transparentVerts.size());

    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
}