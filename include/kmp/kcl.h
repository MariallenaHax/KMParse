#pragma once
#include <bstream.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <cstdint>
#include <memory>
#include <vector>
#include <map>
#include "cache.h"
#include "glad/glad.h"
#pragma pack(push, 1)
struct KCLPrism {
    float mLength;
    uint16_t mPositionIdx;
    uint16_t mDirectionIdx;
    uint16_t mNormal1Idx;
    uint16_t mNormal2Idx;
    uint16_t mNormal3Idx;
    uint16_t mFlags;

};
#pragma pack(pop)
class SKclIO
{
public:
    struct PrismWorld {
        glm::vec3 base;
        glm::vec3 direction;
        glm::vec3 normal1, normal2, normal3;
        float length;
        uint16_t flags;
    };
    struct PrismMeta {
        uint16_t flags;
        const KCLPrism* source;
    };
    struct KCL {
        glm::vec3 v1, v2, v3;
        glm::vec3 normal;
        uint16_t flags;
    };
    std::vector<glm::vec3> mPositions, mNormals;
    std::vector<KCLPrism> mPrisms;
    GLuint vao, vbo;
    SKclIO();
    KCL CreateKCLFromPrisms(KCLPrism prism, const std::vector<glm::vec3>& positions, const std::vector<glm::vec3>& normals);
    GLuint kclProgram = CreateShaderProgram(R"(#version 330 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec4 color;

uniform mat4 u_vp;
out vec3 v_Normal;
out vec4 v_Color;
void main()
{
    gl_Position = u_vp * vec4(pos, 1.0);
    v_Normal = norm;
    v_Color = color;
}
)", R"(
#version 330 core

in vec3 v_Normal;
in vec4 v_Color;
out vec4 FragColor;
void main()
{
    vec3 n = normalize(v_Normal);
    FragColor = v_Color;
}
)");
    ~SKclIO();
    void Reset();
    std::vector<KCL> mKCLs;
    std::vector<KCL> opaqueTris;
    std::vector<KCL> transparentTris;
    struct KCLCategory {
        bool road;
        bool wall;
        bool special;
        bool wall2;
        bool item;
    };
    struct KCLColorEntry {
        KCLCategory cat;
        glm::vec4 color;
        int priority;
        const char* name;
    };
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec4 color;
    };
    struct KclTri
    {
        glm::vec3 v1, v2, v3;
        glm::vec3 normal;
        glm::vec4 color;
    };
    std::vector<KclTri> mTris;
    std::vector<Vertex> verts;
    void Load(bStream::CStream* stream);
    glm::vec4 GetKCLColor(uint16_t flags);
    void Render(glm::mat4 vp, const glm::vec3& camPos);
    PrismWorld ToWorldPrism(const KCLPrism& prism,
        const std::vector<glm::vec3>& positions,
        const std::vector<glm::vec3>& normals)
    {
        PrismWorld world;
        world.base = positions[prism.mPositionIdx];
        world.direction = glm::normalize(normals[prism.mDirectionIdx]);
        world.length = prism.mLength;
        world.normal1 = glm::normalize(normals[prism.mNormal1Idx]);
        world.normal2 = glm::normalize(normals[prism.mNormal2Idx]);
        world.normal3 = glm::normalize(normals[prism.mNormal3Idx]);
        world.flags = prism.mFlags;
        return world;
    }
};