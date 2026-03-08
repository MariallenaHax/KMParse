#pragma once
#include "GX/GXMaterial.h"
#include <cmath>
#include <variant>
#include <algorithm> 
#include  <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <glad/glad.h>
#include <glfw3.h>
#include "egg/bfg.h"
#include "cache.h"
enum class EGXAttribute3 : uint32_t {
    PositionMatrixIdx = 0,
    Tex0MatrixIdx = 1,
    Tex1MatrixIdx = 2,
    Tex2MatrixIdx = 3,
    Tex3MatrixIdx = 4,
    Tex4MatrixIdx = 5,
    Tex5MatrixIdx = 6,
    Tex6MatrixIdx = 7,
    Tex7MatrixIdx = 8,

    Position = 9,
    Normal = 10,
    Color0 = 11,
    Color1 = 12,

    TexCoord0 = 13,
    TexCoord1 = 14,
    TexCoord2 = 15,
    TexCoord3 = 16,
    TexCoord4 = 17,
    TexCoord5 = 18,
    TexCoord6 = 19,
    TexCoord7 = 20,

    Attribute_Max = 21,
    Null = 0xFF
};
struct VCDInfo {
    enum AttrType : uint8_t {
        NONE = 0,
        DIRECT = 1,
        INDEX8 = 2,
        INDEX16 = 3,
    };

    uint8_t pnmtx;
    uint8_t texmtx[8];

    AttrType pos;
    AttrType nrm;
    AttrType clr0;
    AttrType clr1;
    AttrType tex[8];
};
struct EGX_Array {
    std::vector<uint8_t> buffer;
    size_t offs = 0;
    size_t stride = 0;
};
class bres {
public:
    ~bres();
struct Light {
    glm::vec3 position;     // Point light position
    glm::vec3 direction;    // Directional light direction
    glm::vec4 color;        // RGBA

    glm::vec3 distAtten;    // Distance attenuation (a0, a1, a2)
    glm::vec3 cosAtten;     // Angle attenuation (k0, k1, k2)

    bool isDirectional;     // true = directional, false = point
};
struct LightSet {
    std::vector<Light> lights;
    std::vector<glm::vec4> ambient;
    std::vector<int> ambientIndexForLight;
};

    GLuint brresVAO = 0;

    struct VertexAttribute {
        uint32_t location;
        uint32_t binding;
        uint32_t offset;
        uint32_t componentCount;
        uint32_t stride;
        uint32_t componentType;
        bool normalized;
    };

    struct VertexInputLayout {
        std::vector<VertexAttribute> attributes;
    };

    struct GPUBufferHandle {
        uint32_t id = 0;
        uint32_t size = 0;
    };

    struct VertexBuffer {
        std::vector<uint8_t> data;
        GPUBufferHandle gpu;
    };

    struct IndexBuffer {
        std::vector<uint16_t> indices;
        GPUBufferHandle gpu;
    };
    struct GX_VtxDesc {
    uint8_t type; 
};
    struct VATAttr {
        uint8_t compCnt;
        uint8_t compType;
        uint8_t compShift;
    };
struct SourceVatLayout {
    uint32_t srcVertexSize;
    VATAttr vatFormat[12];
    GX_VtxDesc    vcd[12];
};

    struct Mesh {
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> colors0;
        std::vector<float> colors1;
        std::array<std::vector<float>, 8> texcoords;

        std::vector<uint16_t> indices;
    };

    struct ResDicEntry {
        std::string name;
        uint32_t    offs;
    };

    enum class ResUserDataItemValueType : uint32_t {
        S32 = 0,
        F32 = 1,
        STRING = 2,
    };

    struct ResUserDataItemBase {
        ResUserDataItemValueType userDataType;
        std::string              name;
        uint32_t                 id;
    };

    struct ResUserDataItemNumber : ResUserDataItemBase {
        std::vector<float> value;
    };

    struct ResUserDataItemString : ResUserDataItemBase {
        std::vector<std::string> value;
    };

    using ResUserDataItem = std::variant<ResUserDataItemNumber, ResUserDataItemString>;

    struct ResUserData {
        std::vector<ResUserDataItem> entries;
    };
    struct PLT0 {
        std::string name;
        uint32_t    format;
        std::vector<uint8_t> data;
    };
    struct TEX0 {
        std::string name;
        uint16_t    width;
        uint16_t    height;
        uint32_t    format;
        uint32_t    mipCount;
        float       minLOD;
        float       maxLOD;

        size_t data;
        uint32_t    paletteFormat = 0;
        size_t paletteData;
        GLuint texID;
    };
    enum class MapMode : uint32_t {
        TEXCOORD = 0x00,
        ENV_CAMERA = 0x01,
        PROJECTION = 0x02,
        ENV_LIGHT = 0x03,
        ENV_SPEC = 0x04,
    };

    struct MDL0_TexSrtEntry {
        int        refCamera;
        int        refLight;
        MapMode    mapMode;
        float scaleS;
        float scaleT;
        float rotation;
        float translationS;
        float translationT;
        glm::mat4  srtMtx;
        glm::mat4  effectMtx;
    };

    struct MDL0_MaterialSamplerEntry {
        std::string name;
        std::string namePalette;
        EGXWrapMode wrapS;
        EGXWrapMode  wrapT;
        EGXFilterMode minFilter;
        EGXFilterMode magFilter;
        float         lodBias;
        GLuint glTexID;
        int width, height;
        int texMapSlot;
    };

    struct MDL0_MaterialEntry {
        uint32_t index;
        std::string name;
        bool translucent;
        int  lightSetIdx;
        bool zCompLoc;
        int  fogIdx;

        GXMaterial gxMaterial;

        std::vector<MDL0_MaterialSamplerEntry> samplers;
        std::vector<MDL0_TexSrtEntry>          texSrts;
        std::vector<std::array<float, 8>>      indTexMatrices;

        std::vector<Color> colorAmbRegs;
        std::vector<Color> colorMatRegs;
        std::vector<Color> colorRegisters;
        std::vector<Color> colorConstants;
        GLuint shaderProgram;
        int primaryShapeIdx = -1;
    };
    struct VtxBufferData {
        std::string name;
        uint32_t id;
        EGXComponentCount compCnt;
        EGXComponentType compType;
        uint8_t compShift;
        uint8_t stride;
        uint16_t count;
        std::vector<uint8_t> buffer;
        size_t offs; 
    };

    struct ShapeKey {
        int pos;
        int nrm;
        std::array<int, 8> uv;
        int mat;

        bool operator==(const ShapeKey& o) const {
            return pos == o.pos &&
                nrm == o.nrm &&
                uv == o.uv &&
                mat == o.mat;
        }
    };

    struct ShapeKeyHash {
        size_t operator()(const ShapeKey& k) const {
            size_t h = std::hash<int>()(k.pos);
            h ^= std::hash<int>()(k.nrm + 0x9e3779b9 + (h << 6) + (h >> 2));
            for (int i = 0; i < 8; i++)
                h ^= std::hash<int>()(k.uv[i] + 0x9e3779b9 + (h << 6) + (h >> 2));
            h ^= std::hash<int>()(k.mat + 0x9e3779b9 + (h << 6) + (h >> 2));
            return h;
        }
    };

    struct VATInfo {
        VATAttr pos;
        VATAttr nrm;
        VATAttr clr0;
        VATAttr clr1;
        std::array<VATAttr, 8> tex;
    };
    struct InputVertexBuffers {
        std::vector<VtxBufferData> pos;
        std::vector<VtxBufferData> nrm;
        std::vector<VtxBufferData> clr0;
        std::vector<VtxBufferData> clr1;
        std::vector<VtxBufferData> txc;
    };
    struct VertexLayout {
        uint32_t stride;
        int posOffset = -1;
        int nrmOffset = -1;
        int clr0Offset = -1;
        int clr1Offset = -1;
        std::array<int, 8> texOffset;
        int boneIndexOffset = -1;
        int boneWeightOffset = -1;
    };

    struct LoadedVertexData {
        VertexLayout layout;
        std::vector<uint8_t> vertexBuffer;
        std::vector<uint16_t> indexBuffer;
        uint32_t totalVertexCount = 0;
    };
    struct MDL0;
    struct MDL0_ShapeEntry;
    struct SHP0_EntryData;
    struct Shp0Runtime {
    std::vector<std::vector<glm::vec3>> morphPositions;
    std::vector<std::vector<glm::vec3>> morphNormals;
    std::unordered_map<std::string, int> morphTargetIndex;
    std::vector<float> morphWeights;

    void ResetMorphWeights() {
        std::fill(morphWeights.begin(), morphWeights.end(), 0.0f);
    }

    void SetMorphWeight(const std::string& name, float w) {
        auto it = morphTargetIndex.find(name);
        if (it == morphTargetIndex.end())
            return;
        morphWeights[it->second] = w;
    }
};
    static void InitializeMorphTargets(
        Shp0Runtime& out,
        const MDL0& mdl,
        const MDL0_ShapeEntry& shape,
        const SHP0_EntryData& entry)
    {
        const auto& baseTrack = entry.tracks[0];
        const auto& baseBuf = mdl.inputBuffers.pos[baseTrack.shapeIndex + 1];
        const auto& baseBuf2 = mdl.inputBuffers.nrm[baseTrack.shapeIndex + 1];

        shape.runtime->InitializeFromLoadedData(&baseBuf,&baseBuf2);

        int morphIndex = 0;

        for (auto& track : entry.tracks)
        {
            for (auto& buf : mdl.inputBuffers.pos)
            {
                if (buf.name == track.targetName)
                {
                    out.morphTargetIndex[buf.name] = morphIndex;

                    out.morphPositions.push_back(
                        ShapeRuntime::ComputeDelta(baseBuf, buf)
                    );

                    out.morphNormals.push_back(
                        ShapeRuntime::ComputeDeltaNormals(baseBuf, buf)
                    );

                    out.morphWeights.push_back(0.0f);
                    morphIndex++;
                    break;
                }
            }
        }
    }
struct ShapeRuntime {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ibo = 0;
    VertexLayout layout;
    GLsizei indexCount = 0;
    GLuint texID;
    glm::vec3 boundingCenter = glm::vec3(0.0f);
    float boundingRadius = 1.0f;
    std::array<int, 8> texCoordMap;
    glm::vec3 min;
    glm::vec3 max;

    std::vector<uint8_t> vertexBuffer;
    LoadedVertexData* source = nullptr;
    std::vector<glm::vec3> basePositions;
    std::vector<glm::vec3> baseNormals;

    glm::vec3 center() const { return (min + max) * 0.5f; }

    static glm::vec3 ReadGXVec3(const VtxBufferData& buf, uint32_t index)
    {
        const uint8_t* p = buf.buffer.data() + buf.offs + index * buf.stride;

        float scale = (buf.compShift > 0)
            ? 1.0f / float(1u << buf.compShift)
            : 1.0f;

        float x = 0, y = 0, z = 0;

        switch (buf.compType)
        {
        case EGXComponentType::Unsigned8:
        {
            x = float(p[0]) * scale;
            y = float(p[1]) * scale;
            z = float(p[2]) * scale;
            break;
        }

        case EGXComponentType::Signed8:
        {
            x = float(*reinterpret_cast<const int8_t*>(&p[0])) * scale;
            y = float(*reinterpret_cast<const int8_t*>(&p[1])) * scale;
            z = float(*reinterpret_cast<const int8_t*>(&p[2])) * scale;
            break;
        }

        case EGXComponentType::Unsigned16:
        {
            uint16_t vx = (uint16_t(p[0]) << 8) | p[1];
            uint16_t vy = (uint16_t(p[2]) << 8) | p[3];
            uint16_t vz = (uint16_t(p[4]) << 8) | p[5];
            x = float(vx) * scale;
            y = float(vy) * scale;
            z = float(vz) * scale;
            break;
        }

        case EGXComponentType::Signed16:
        {
            int16_t vx = int16_t((uint16_t(p[0]) << 8) | p[1]);
            int16_t vy = int16_t((uint16_t(p[2]) << 8) | p[3]);
            int16_t vz = int16_t((uint16_t(p[4]) << 8) | p[5]);
            x = float(vx) * scale;
            y = float(vy) * scale;
            z = float(vz) * scale;
            break;
        }

        case EGXComponentType::Float:
        {
            const float* f = reinterpret_cast<const float*>(p);
            x = f[0];
            y = f[1];
            z = f[2];
            break;
        }

        default:
            break;
        }

        return glm::vec3(x, y, z);
    }

void InitializeFromLoadedData(
    const VtxBufferData* posBuf,
    const VtxBufferData* nrmBuf)
{
    if (posBuf)
    {
        size_t count = posBuf->count;
        basePositions.resize(count);

        for (size_t i = 0; i < count; i++)
            basePositions[i] = ReadGXVec3(*posBuf, i);
    }

    if (nrmBuf)
    {
        size_t count = nrmBuf->count;
        baseNormals.resize(count);

        for (size_t i = 0; i < count; i++)
            baseNormals[i] = ReadGXVec3(*nrmBuf, i);
    }
}
    static std::vector<glm::vec3> ComputeDelta(
        const VtxBufferData& base,
        const VtxBufferData& morph)
    {
        if (base.count != morph.count)
            return {};

        uint32_t count = base.count;
        std::vector<glm::vec3> out(count);

        for (uint32_t i = 0; i < count; i++) {
            glm::vec3 b = ReadGXVec3(base, i);
            glm::vec3 m = ReadGXVec3(morph, i);
            out[i] = m - b;
        }

        return out;
    }
    static std::vector<glm::vec3> ComputeDeltaNormals(
        const VtxBufferData& base,
        const VtxBufferData& morph)
    {
        if (base.count != morph.count)
            return {};

        uint32_t count = base.count;
        std::vector<glm::vec3> out(count);

        for (uint32_t i = 0; i < count; i++) {
            glm::vec3 b = ReadGXVec3(base, i);
            glm::vec3 m = ReadGXVec3(morph, i);
            out[i] = m - b;
        }

        return out;
    }
        void ApplyMorphs(const Shp0Runtime& shpRt)
{
    size_t count = basePositions.size();
    std::vector<glm::vec3> blendedPos = basePositions;
    std::vector<glm::vec3> blendedNrm = baseNormals;

    for (size_t m = 0; m < shpRt.morphPositions.size(); ++m) {
        float w = shpRt.morphWeights[m];

        for (size_t i = 0; i < count; i++) {
            blendedPos[i] += shpRt.morphPositions[m][i] * w;
        }

        if (m < shpRt.morphNormals.size() && !shpRt.morphNormals[m].empty()) {
            for (size_t i = 0; i < count; i++) {
                blendedNrm[i] += shpRt.morphNormals[m][i] * w;
            }
        }
    }

    uint8_t* ptr = vertexBuffer.data();
    for (size_t i = 0; i < count; i++) {
        memcpy(ptr + layout.posOffset, &blendedPos[i], sizeof(glm::vec3));
        if (layout.nrmOffset >= 0 && !baseNormals.empty()) {
            memcpy(ptr + layout.nrmOffset, &blendedNrm[i], sizeof(glm::vec3));
        }
        ptr += layout.stride;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        vertexBuffer.size(),
        vertexBuffer.data());
}

    };

    struct MDL0_ShapeEntry {
    std::string name;
    int mtxIdx;
    int id;

    int16_t idVtxPos;
    int16_t idVtxNrm;
    int16_t idVtxClr0;
    int16_t idVtxClr1;
    int16_t idVtxTxc0;
    int16_t idVtxTxc1;
    int16_t idVtxTxc2;
    int16_t idVtxTxc3;
    int16_t idVtxTxc4;
    int16_t idVtxTxc5;
    int16_t idVtxTxc6;
    int16_t idVtxTxc7;
    int16_t idVtxFurVec;
    int16_t idVtxFurPos;
    int32_t mtxSetOffs;

    uint32_t numVertices;
    uint32_t numPolygons;

    std::vector<uint16_t> indexBuffer;

    uint32_t primDLOffs;
    uint32_t primDLSize;
    uint32_t primDLCmdSize;

    std::array<VATInfo, 8> vat;
    std::array<VCDInfo, 8> vcd;

    uint32_t prePrimDLOffs = 0;
    uint32_t prePrimDLSize = 0;
    uint32_t prePrimDLCmdSize = 0;
    InputVertexBuffers InputBuffers;
    int nodeid;
    std::shared_ptr<ShapeRuntime> runtime;
    LoadedVertexData vtxdata;
    std::vector<std::unique_ptr<Shp0Runtime>> shpRuntimes;
};
    enum class NodeFlags : uint32_t {
        SRT_IDENTITY = 0x00000001,
        TRANS_ZERO = 0x00000002,
        ROT_ZERO = 0x00000004,
        SCALE_ONE = 0x00000008,
        SCALE_HOMO = 0x00000010,
        VISIBLE = 0x00000100,
        REFER_BB_ANCESTOR = 0x00000400,
    };

    enum class BillboardMode : uint32_t {
        NONE = 0,
        BILLBOARD,
        PERSP_BILLBOARD,
        ROT,
        PERSP_ROT,
        Y,
        PERSP_Y,
    };
    struct AABB {
        float minX, minY, minZ;
        float maxX, maxY, maxZ;

        AABB()
            : minX(0), minY(0), minZ(0),
            maxX(0), maxY(0), maxZ(0) {
        }

        AABB(float minX, float minY, float minZ,
            float maxX, float maxY, float maxZ)
            : minX(minX), minY(minY), minZ(minZ),
            maxX(maxX), maxY(maxY), maxZ(maxZ) {
        }
        glm::vec3 center() const {
            return glm::vec3(
                (minX + maxX) * 0.5f,
                (minY + maxY) * 0.5f,
                (minZ + maxZ) * 0.5f
            );
        }
        void transform(const glm::mat4& m) {
            glm::vec3 corners[8] = {
                {minX, minY, minZ},
                {minX, minY, maxZ},
                {minX, maxY, minZ},
                {minX, maxY, maxZ},
                {maxX, minY, minZ},
                {maxX, minY, maxZ},
                {maxX, maxY, minZ},
                {maxX, maxY, maxZ},
            };

            glm::vec3 newMin(FLT_MAX, FLT_MAX, FLT_MAX);
            glm::vec3 newMax(-FLT_MAX, -FLT_MAX, -FLT_MAX);

            for (int i = 0; i < 8; i++) {
                glm::vec4 w = m * glm::vec4(corners[i], 1.0f);
                glm::vec3 p = glm::vec3(w);

                if (p.x < newMin.x) newMin.x = p.x;
                if (p.y < newMin.y) newMin.y = p.y;
                if (p.z < newMin.z) newMin.z = p.z;

                if (p.x > newMax.x) newMax.x = p.x;
                if (p.y > newMax.y) newMax.y = p.y;
                if (p.z > newMax.z) newMax.z = p.z;
            }

            minX = newMin.x; minY = newMin.y; minZ = newMin.z;
            maxX = newMax.x; maxY = newMax.y; maxZ = newMax.z;
        }
    };
    struct MDL0_NodeEntry {
        std::string name;
        uint32_t id;
        uint32_t mtxId;
        NodeFlags flags;
        BillboardMode billboardMode;
        uint32_t billboardRefNodeId;
        glm::mat4 modelMatrix;
        std::optional<AABB> bbox;
        bool visible;
        int parentNodeId;
        glm::mat4 forwardBindPose;
        glm::mat4 inverseBindPose;
        std::optional<ResUserData> userData;

        glm::vec3 scale;
        glm::vec3 rotate;
        glm::vec3 translate;
glm::mat4 computeLocalMatrix() const
{
    glm::mat4 M(1.0f);

    M = glm::scale(M, scale);

    M = glm::rotate(M, rotate.z, glm::vec3(0, 0, 1));
    M = glm::rotate(M, rotate.y, glm::vec3(0, 1, 0));
    M = glm::rotate(M, rotate.x, glm::vec3(1, 0, 0));

    M = glm::translate(M, translate);

    return M;
}
        glm::mat4 computeLocalMatrixBillboard(
            const glm::mat4& cameraMatrix
        ) const
        {
            glm::mat4 M = computeLocalMatrix();

            if (billboardMode == BillboardMode::NONE)
                return M;

            glm::mat3 camRot = glm::mat3(cameraMatrix);

            glm::vec3 nodePos = glm::vec3(M[3]);

            glm::mat4 B = glm::mat4(1.0f);

            switch (billboardMode) {

            case BillboardMode::BILLBOARD:
   
                B = glm::mat4(camRot);
                break;

            case BillboardMode::PERSP_BILLBOARD:
            {
                glm::vec3 camPos = glm::vec3(cameraMatrix[3]);
                glm::vec3 look = glm::normalize(camPos - nodePos);
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), look));
                glm::vec3 up = glm::cross(look, right);

                B[0] = glm::vec4(right, 0);
                B[1] = glm::vec4(up, 0);
                B[2] = glm::vec4(look, 0);
            }
            break;

            case BillboardMode::ROT:
            {
                glm::vec3 up(0, 1, 0);
                glm::vec3 look = glm::normalize(glm::vec3(cameraMatrix[3]) - nodePos);
                glm::vec3 right = glm::normalize(glm::cross(up, look));
                glm::vec3 newLook = glm::cross(right, up);

                B[0] = glm::vec4(right, 0);
                B[1] = glm::vec4(up, 0);
                B[2] = glm::vec4(newLook, 0);
            }
            break;

            case BillboardMode::PERSP_ROT:
            {
                glm::vec3 camPos = glm::vec3(cameraMatrix[3]);
                glm::vec3 look = glm::normalize(camPos - nodePos);
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), look));
                glm::vec3 up = glm::cross(look, right);

                B[0] = glm::vec4(right, 0);
                B[1] = glm::vec4(glm::vec3(0, 1, 0), 0);
                B[2] = glm::vec4(look, 0);
            }
            break;

            case BillboardMode::Y:
            {
                glm::vec3 up(0, 1, 0);
                glm::vec3 look = glm::normalize(glm::vec3(cameraMatrix[3]) - nodePos);
                glm::vec3 right = glm::normalize(glm::cross(up, look));
                glm::vec3 newLook = glm::cross(right, up);

                B[0] = glm::vec4(right, 0);
                B[1] = glm::vec4(up, 0);
                B[2] = glm::vec4(newLook, 0);
            }
            break;

            case BillboardMode::PERSP_Y:
            {
                glm::vec3 camPos = glm::vec3(cameraMatrix[3]);
                glm::vec3 look = glm::normalize(camPos - nodePos);
                glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), look));
                glm::vec3 up = glm::vec3(0, 1, 0);

                B[0] = glm::vec4(right, 0);
                B[1] = glm::vec4(up, 0);
                B[2] = glm::vec4(look, 0);
            }
            break;
            }

            B[3] = M[3];

            return B;
        }
    };
    enum class ByteCodeOp : uint8_t {
        NOP = 0x00,
        RET = 0x01,
        NODEDESC = 0x02,
        NODEMIX = 0x03,
        DRAW = 0x04,
        EVPMTX = 0x05,
        MTXDUP = 0x06,
    };
    struct NodeDescOp {
        ByteCodeOp op = ByteCodeOp::NODEDESC;
        uint16_t nodeId;
        uint16_t parentMtxId;
    };

    struct MtxDupOp {
        ByteCodeOp op = ByteCodeOp::MTXDUP;
        uint16_t toMtxId;
        uint16_t fromMtxId;
    };

    struct NodeMixOp_ {
        ByteCodeOp op = ByteCodeOp::NODEMIX;
        uint16_t dstMtxId;
        std::vector<uint16_t> blendMtxIds;
        std::vector<float> weights;
    };

    struct EvpMtxOp {
        ByteCodeOp op = ByteCodeOp::EVPMTX;
        uint16_t mtxId;
        uint16_t nodeId;
    };
    struct DrawOp {
        uint16_t matId;
        uint16_t shpId;
        uint16_t nodeId;
    };
    struct MDL0_VertexBuffer {
        std::vector<uint8_t> data;  
        uint32_t stride;          
        uint32_t count;         
        uint32_t offs;           
    };
    using NodeTreeOp = std::variant<NodeDescOp, MtxDupOp>;
    using NodeMixOp = std::variant<NodeMixOp_, EvpMtxOp>;
    struct MDL0_SceneGraph {
        std::vector<NodeTreeOp> nodeTreeOps;
        std::vector<NodeMixOp>  nodeMixOps;
        std::vector<DrawOp>     drawOpaOps;
        std::vector<DrawOp>     drawXluOps;
    };
    struct MDL0 {
        const uint8_t* basePtr;
        size_t fileSize;

        std::string name;
        std::optional<AABB> bbox;
        std::vector<MDL0_MaterialEntry> materials;
        std::vector<MDL0_ShapeEntry> shapes;
        std::vector<MDL0_NodeEntry> nodes;
        std::vector<glm::mat4> worldMatrices;

        MDL0_SceneGraph sceneGraph;

        uint32_t numWorldMtx;
        uint32_t numViewMtx;
        bool needNrmMtxArray;
        bool needTexMtxArray;

        std::vector<ResDicEntry> vtxPosResDic;
        std::vector<ResDicEntry> vtxNrmResDic;
        std::vector<ResDicEntry> vtxClrResDic;
        std::vector<ResDicEntry> vtxTxcResDic;


        std::vector<int32_t> mtxIdToNodeId;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<glm::mat4> boneMatrices;
        InputVertexBuffers inputBuffers;
        void updateWorldMatrices(const glm::mat4& camWorld);
        void updateBoneMatrices();
    };
    struct BitMap {
        int numBits = 0;
        std::vector<uint32_t> words;

        BitMap() = default;

        explicit BitMap(int bits)
            : numBits(bits)
        {
            int numWords = (bits + 31) / 32;
            words.resize(numWords, 0);
        }

        inline bool getBit(int index) const {
            if (index < 0 || index >= numBits)
                return false;
            int wordIndex = index >> 5;  
            int bitIndex = index & 31;    
            return (words[wordIndex] >> bitIndex) & 1;
        }

        inline void setBit(int index, bool value) {
            if (index < 0 || index >= numBits)
                return;
            int wordIndex = index >> 5;
            int bitIndex = index & 31;
            uint32_t mask = 1u << bitIndex;

            if (value)
                words[wordIndex] |= mask;
            else
                words[wordIndex] &= ~mask;
        }

        inline void setWord(int wordIndex, uint32_t value) {
            if (wordIndex < 0 || wordIndex >= (int)words.size())
                return;
            words[wordIndex] = value;
        }
    };

    enum class LoopMode {
        ONCE = 0,
        REPEAT = 1,
    };

    struct AnimationBase {
        std::string name;
        float duration;
        LoopMode loopMode;
    };
    enum class AnimationTrackType {
        LINEAR,
        HERMITE,
    };
    struct FloatKeyHermite {
        float frame;
        float value;
        float tangent;
    };
    struct FloatTrackLinear {
        AnimationTrackType type = AnimationTrackType::LINEAR;
        std::vector<float> frames;
    };
    struct FloatTrackHermite {
        AnimationTrackType type = AnimationTrackType::HERMITE;
        std::vector<FloatKeyHermite> frames;
    };
    using FloatAnimationTrack = std::variant<FloatTrackLinear, FloatTrackHermite>;
    class AnimationController {
    public:
        void SetTimeSeconds(float t) { mTimeSeconds = t; }
        void AddTimeSeconds(float dt) { mTimeSeconds += dt; }

        void SetFramerate(float fps) { mFPS = fps; }

        float getTimeInFrames() const {
            return mTimeSeconds * mFPS;
        }

    private:
        float mTimeSeconds = 0.0f;
        float mFPS = 60.0f; 
    };



    enum class TexMtxIndex : uint32_t {
        TEX0 = 0,
        TEX1,
        TEX2,
        TEX3,
        TEX4,
        TEX5,
        TEX6,
        TEX7,
        IND0,
        IND1,
        IND2,
        COUNT,
    };

    struct SRT0_TexData {
        std::optional<FloatAnimationTrack> scaleS;
        std::optional<FloatAnimationTrack> scaleT;
        std::optional<FloatAnimationTrack> rotation;
        std::optional<FloatAnimationTrack> translationS;
        std::optional<FloatAnimationTrack> translationT;
    };

    struct SRT0_MatData {
        std::string materialName;
        std::vector<SRT0_TexData> texAnimations; // index = TexMtxIndex
    };

    struct SRT0 : AnimationBase {
        TexMatrixMode texMtxMode;
        std::vector<SRT0_MatData> matAnimations;
    };
    struct MaterialInstance;
    class SRT0TexMtxAnimator {
    public:
        AnimationController* mController;
        SRT0* mSrt0;
        const SRT0_TexData* mTexData;
        int mTexMtxIdx;

        MaterialInstance* mMaterial;

        SRT0TexMtxAnimator(AnimationController* controller,
            SRT0* srt0,
            const SRT0_TexData* texData,
            MaterialInstance* material,
            int texMtxIdx)
            : mController(controller),
            mSrt0(srt0),
            mTexData(texData),
            mMaterial(material),
            mTexMtxIdx(texMtxIdx)
        {
        }


        void CalcTexMtx(glm::mat4& dst);

        void CalcIndTexMtx(glm::mat4& dst);
    private:

        void _CalcTexMtx(glm::mat4& dst, TexMatrixMode mode);
    };

    struct PAT0_TexFrameData {
        float frame;
        uint16_t texIndex;
        uint16_t palIndex;
    };

    struct PAT0_TexData {
        std::vector<PAT0_TexFrameData> animationTrack;
        bool texIndexValid = false;
        bool palIndexValid = false;
    };

    struct PAT0_MatData {
        std::string materialName;
        std::vector<PAT0_TexData> texAnimations;
    };

    struct PAT0 : AnimationBase {
        std::vector<PAT0_MatData> matAnimations;
                std::vector<std::string> texNames;
    };


struct SHP0_MorphTrack {
    int   shapeIndex;
    std::string targetName;
    FloatAnimationTrack keys;
};

struct SHP0_EntryData {
    std::string vertexNodeName;
    std::vector<SHP0_MorphTrack> tracks;
};

struct SHP0 : AnimationBase {
    std::vector<SHP0_EntryData> entries;
};

struct SHP0MorphAnimator {
    AnimationController* controller;
    const SHP0_EntryData* entry;
    ShapeRuntime* runtime;
    Shp0Runtime* shpRt;

    void Apply() {
        float frame = controller->getTimeInFrames();

        shpRt->ResetMorphWeights();

        for (auto& track : entry->tracks) {
            float w = SampleFloatTrack(track.keys, frame);
            shpRt->SetMorphWeight(track.targetName, w);
        }

        runtime->ApplyMorphs(*shpRt);
    }
};


    template<typename T>
    const T& FindFrameData(const std::vector<T>& frames, float frame) {
        if (frames.size() == 1)
            return frames[0];

        int idx0 = static_cast<int>(frames.size());
        while (--idx0 > 0) {
            if (frame >= frames[idx0].frame)
                break;
        }

        return frames[idx0];
    }
    enum class AnimatableColor : uint32_t {
        MAT0, MAT1, AMB0, AMB1,
        C0, C1, C2,
        K0, K1, K2, K3,
        COUNT,
    };
    struct CLR0_ColorData {
        uint32_t mask;
        std::vector<uint32_t> frames; 
    };

    struct CLR0_MatData {
        std::string materialName;
        std::vector<CLR0_ColorData> clrAnimations; 
    };

    struct CLR0 : AnimationBase {
        std::vector<CLR0_MatData> matAnimations;
    };
    class CLR0ColorAnimator {
    public:
        CLR0ColorAnimator(AnimationController* controller,
            const CLR0* clr0,
            const CLR0_ColorData* clrData)
            : mController(controller), mCLR0(clr0), mClrData(clrData) {
        }

        void CalcColor(Color& dst, const Color& orig);
        AnimationController* mController;
    private:
        const CLR0* mCLR0;
        const CLR0_ColorData* mClrData;
    };

    struct CHR0_NodeData {
        std::string nodeName;

        std::optional<FloatAnimationTrack> scaleX, scaleY, scaleZ;
        std::optional<FloatAnimationTrack> rotationX, rotationY, rotationZ;
        std::optional<FloatAnimationTrack> translationX, translationY, translationZ;
    };

    struct CHR0 : AnimationBase {
        std::vector<CHR0_NodeData> nodeAnimations;
    };
    enum class TrackFormat : uint8_t {
        CONSTANT = 0,
        _32 = 1,
        _48 = 2,
        _96 = 3,
        FRM_8 = 4,
        FRM_16 = 5,
        FRM_32 = 6,
    };
    struct FinalVertex {
        glm::vec4 pos{};
        glm::vec3 nrm{};
        glm::vec4 colors[2]{};
        glm::vec3 uvs[8]{};
        float posMtx = 0.0f;
        float texMtx[8]{};
    };

    class CHR0NodesAnimator {
    public:
        CHR0NodesAnimator(AnimationController* controller,
            const CHR0* chr0,
            const std::vector<CHR0_NodeData>& nodeData)
            : mController(controller), mCHR0(chr0), mNodeData(nodeData),
            disabled(nodeData.size(), false) {
        }

        bool CalcModelMtx(glm::mat4& dst, size_t nodeId);

        std::vector<bool> disabled;

    private:
        AnimationController* mController;
        const CHR0* mCHR0;
        std::vector<CHR0_NodeData> mNodeData;
    };
    std::unique_ptr<CHR0NodesAnimator> BindCHR0Animator(
        AnimationController* controller,
        const CHR0& chr0,
        const std::vector<MDL0_NodeEntry>& nodes)
    {
        std::vector<CHR0_NodeData> nodeData(nodes.size());

        for (const auto& anim : chr0.nodeAnimations) {
            auto it = std::find_if(nodes.begin(), nodes.end(),
                [&](const MDL0_NodeEntry& n) { return n.name == anim.nodeName; });

            if (it != nodes.end())
                nodeData[it->id] = anim;
        }

        if (std::all_of(nodeData.begin(), nodeData.end(),
            [](const CHR0_NodeData& d) { return d.nodeName.empty(); }))
            return nullptr;

        return std::make_unique<CHR0NodesAnimator>(controller, &chr0, nodeData);
    }

    struct RRES {
        std::shared_ptr<std::vector<uint8_t>> buffer;
        const uint8_t* basePtr;
        std::vector<PLT0> plt0;
        std::vector<TEX0> tex0;
        std::vector<MDL0> mdl0;
        std::vector<SRT0> srt0;
        std::vector<PAT0> pat0;
        std::vector<CLR0> clr0;
        std::vector<CHR0> chr0;
        std::vector<SHP0> shp0;
    };
    struct UnifiedVertex {
        glm::vec3 pos;
        glm::vec3 nrm;
        glm::vec2 uv[8];
    };

    std::vector<UnifiedVertex> unifiedVertices;
    std::vector<uint16_t> unifiedIndices;
    struct LoadedVertexDraw {
    uint32_t indexOffset = 0;
    uint32_t indexCount  = 0;

    std::array<uint16_t, 10> posMatrixTable{};
    std::array<uint16_t, 10> texMatrixTable{};
};
    struct SourceLayout {
    uint32_t srcVertexSize;   
        int posOffset = -1;
        int nrmOffset = -1;
        int clr0Offset = -1;
        int clr1Offset = -1;
        std::array<int, 8> texOffset; 
    };

struct GX_Array {
    const uint8_t* data;
    uint32_t stride;
    uint32_t count;
    size_t offs;
};

    struct DrawCall {
        uint8_t primType;
        uint8_t vertexFormat;
        uint32_t srcOffs;
        uint32_t vertexCount;
        uint16_t drawIndex;
    };
    struct ParsedDL {
        const uint8_t* dlView = nullptr;

        std::vector<DrawCall> drawCalls;

        std::vector<uint16_t> indexBuffer;

        uint32_t totalVertexCount = 0;

        uint32_t totalIndexCount = 0;

        std::vector<LoadedVertexDraw> draws;
    };

    enum class CT {
        POS_XY = 0,
        POS_XYZ = 1,
        NRM_XYZ = 0,
        NRM_NBT = 1,
        NRM_NBT3 = 2,
        CLR_RGB = 0,
        CLR_RGBA = 1,
        TEX_S = 0,
        TEX_ST = 1,
    };
        static inline uint32_t getAttributeComponentByteSizeRaw(EGXComponentType compType)
        {
            switch (compType)
            {
            case EGXComponentType::Unsigned8:
            case EGXComponentType::Signed8:
            case EGXComponentType::RGBA8:
                return 1;

            case EGXComponentType::Unsigned16:
            case EGXComponentType::Signed16:
                return 2;

            case EGXComponentType::Float:
                return 4;

            default:
                throw std::runtime_error("Invalid EGXComponentType");
            }
        }
            static int getAttributeComponentByteSize(const VATAttr& fmt) {
        return getAttributeComponentByteSizeRaw((EGXComponentType)fmt.compType);
    }

    static int getFormatCompFlagsComponentCount(int flags)
    {
        return flags;
    }

    VertexLayout buildVertexLayout(const VATInfo& vat, const VCDInfo& vcd) {
        VertexLayout layout{};
        int offset = 0;
layout.posOffset  = -1;
layout.nrmOffset  = -1;
layout.clr0Offset = -1;
layout.clr1Offset = -1;
for (int i = 0; i < 8; i++)
    layout.texOffset[i] = -1;

if (vcd.pos != 0) {
    layout.posOffset = offset;
    offset += 3 * sizeof(float);
}

if (vcd.nrm != 0) {
    layout.nrmOffset = offset;
    offset += 3 * sizeof(float);
}

if (vcd.clr0 != 0) {
    layout.clr0Offset = offset;
    offset += 4 * sizeof(float);
}

if (vcd.clr1 != 0) {
    layout.clr1Offset = offset;
    offset += 4 * sizeof(float);
}

for (int t = 0; t < 8; t++) {
    if (vcd.tex[t] != 0) {
        layout.texOffset[t] = offset;
        offset += 2 * sizeof(float);
    }
}

layout.boneIndexOffset = offset;
offset += sizeof(uint32_t) * 4;

layout.boneWeightOffset = offset;
offset += sizeof(float) * 4;

layout.stride = offset;
        return layout;
    }
    inline int getAttributeComponentCount(const VATAttr& fmt)
    {
        switch (fmt.compCnt) {
        case 0: 
            return 3;

        case 1:
            return 2;

        case 2: 
            return 9;

        case 3: 
            return 3;

        case 4: 
            return 3;

        case 5:
            return 4;

        default:
            return 0;
        }
    }
    struct InputBuffers {
        std::vector<GX_Array> pos;
        std::vector<GX_Array> nrm;
        std::vector<GX_Array> clr;
        std::array<std::vector<GX_Array>, 8> tex;
    };
    struct FogBlock {
        glm::vec4 color;
        float startZ;
        float endZ;
        float nearZ;
        float farZ;
        float rangeAdjCenter;
        float rangeAdjScale;
    };
    struct Transform {
        glm::vec3 translation{ 0,0,0 };
        glm::vec3 rotation{ 0,0,0 };
        glm::vec3 scale{ 1,1,1 };

        glm::mat4 ToMat4() const {
            glm::mat4 m(1.0f);
            m = glm::translate(m, translation);
            m = glm::rotate(m, glm::radians(rotation.x), { 1,0,0 });
            m = glm::rotate(m, glm::radians(rotation.y), { 0,1,0 });
            m = glm::rotate(m, glm::radians(rotation.z), { 0,0,1 });
            m = glm::scale(m, scale);
            return m;
        }
    };
    struct ModelInstance;
    struct Packet {
        uint32_t sortKey;
        MDL0_ShapeEntry* shape;
        MDL0_NodeEntry* node;
        ModelInstance* instance;
        int materialIndex;
        int nodeId;

        float depth;
        bool translucent;
        LightSet localLights;
        void SetLights(const LightSet& ls);

        void Render(const glm::mat4& view, const glm::mat4& proj, const glm::vec3 cameraPos);

    };
  struct PAT0TexAnimator {
    AnimationController* controller;
    PAT0* pat0;
    const PAT0_TexData* texData;
    int texMapID;
    std::vector<GLuint> textureIDs;
    int currentTextureIndex = -1;
    std::string originalTexName;

    PAT0TexAnimator(AnimationController* c,
        PAT0* p,
        const PAT0_TexData* t,
        const std::unordered_map<std::string, TEX0*>& texByName,
        int texMapID_,std::string originalTexName)
        : controller(c), pat0(p), texData(t), texMapID(texMapID_)
    {
        for (const auto& texName : pat0->texNames) {
            GLuint id = 0;

            auto it = texByName.find(texName);
            if (it != texByName.end() && it->second)
                id = it->second->texID;

            textureIDs.push_back(id);
        }
    }



        const PAT0_TexFrameData& FindFrameData(
            const std::vector<PAT0_TexFrameData>& track,
            int animFrame)
        {
            static PAT0_TexFrameData dummy{ 0, (uint16_t)-1 };

            if (track.empty())
                return dummy;

            if (animFrame <= track.front().frame)
                return track.front();

            if (animFrame >= track.back().frame)
                return track.back();

            for (size_t i = 1; i < track.size(); i++) {
                if (animFrame < track[i].frame)
                    return track[i - 1];
            }

            return track.back();
        }

        void FillTexture(GLuint& outTexID) {
            if (!texData->texIndexValid)
                return;

            float frame = controller->getTimeInFrames();
            int animFrame = GetAnimFrame(*pat0, frame);

            const auto& frameData = FindFrameData(texData->animationTrack, animFrame);
            currentTextureIndex = frameData.texIndex;

            if (currentTextureIndex >= 0 &&
                currentTextureIndex < (int)textureIDs.size())
            {
                outTexID = textureIDs[currentTextureIndex];
            }
        }
    };
    struct MaterialInstance {
        MDL0_MaterialEntry* material;
        ModelInstance* modelInstance;

        std::vector<glm::mat4> texSrtMtx;
        std::vector<glm::mat3> indTexSrtMtx;
std::array<std::vector<std::unique_ptr<SRT0TexMtxAnimator>>, 11> srt0Animators;
std::array<std::vector<std::unique_ptr<PAT0TexAnimator>>, 8>  pat0Animators;
std::array<std::vector<std::unique_ptr<CLR0ColorAnimator>>, 11> clr0Animators;
std::vector<std::unique_ptr<SHP0MorphAnimator>> shp0Animators;


        MaterialInstance(MDL0_MaterialEntry* mat, ModelInstance* inst)
            : material(mat), modelInstance(inst)
        {
texSrtMtx.resize(mat->texSrts.size()); 
indTexSrtMtx.resize(3);
for (int i = 0; i < mat->texSrts.size(); i++) 
{ 
    texSrtMtx[i] = mat->texSrts[i].srtMtx; 
}
        }

        const SRT0_TexData* FindAnimationData_SRT0(
            const SRT0& srt0,
            const std::string& materialName,
            TexMtxIndex texMtxIndex)
        {
            auto it = std::find_if(
                srt0.matAnimations.begin(), srt0.matAnimations.end(),
                [&](const SRT0_MatData& m) { return m.materialName == materialName; });

            if (it == srt0.matAnimations.end())
                return nullptr;

            size_t idx = static_cast<size_t>(texMtxIndex);
            if (idx >= it->texAnimations.size())
                return nullptr;

            return &it->texAnimations[idx];
        }
        const PAT0_TexData* FindAnimationData_PAT0(
            const PAT0& pat0,
            const std::string& materialName,
            int texIndex)
        {
            auto it = std::find_if(
                pat0.matAnimations.begin(), pat0.matAnimations.end(),
                [&](const PAT0_MatData& m) { return m.materialName == materialName; });

            if (it == pat0.matAnimations.end())
                return nullptr;

            size_t idx = static_cast<size_t>(texIndex);
            if (idx >= it->texAnimations.size())
                return nullptr;

            return &it->texAnimations[idx];
        }
        VtxBufferData* FindShapeByName(const std::string& name)
        {
            for (auto& shape : modelInstance->model->inputBuffers.pos) {
                if (shape.name == name)
                    return &shape;
            }
            return nullptr;
        }
 void BindSRT0(AnimationController* controller, SRT0* srt0) {
    int matIndex = -1;
    for (int m = 0; m < srt0->matAnimations.size(); m++) {
        if (srt0->matAnimations[m].materialName == material->name) {
            matIndex = m;
            break;
        }
    }

    if (matIndex < 0) {
        for (auto& a : srt0Animators)
        return;
    }

    for (int i = 0; i < (int)TexMtxIndex::COUNT; i++) {

        const SRT0_TexData* texData =
            FindAnimationData_SRT0(*srt0, material->name, (TexMtxIndex)i);

if (texData) {
srt0Animators[i].push_back(
    std::make_unique<SRT0TexMtxAnimator>(
        controller,
        srt0,
        texData,
        this, 
        i
    )
);
}
    }
}

 void BindPAT0(AnimationController* controller,
     PAT0* pat0,
     const std::unordered_map<std::string, TEX0*>& texByName) {
     for (int texMapID = 0; texMapID < 8; texMapID++) {
         if (!pat0) {
             continue;
         }

         const PAT0_TexData* patData =
             FindAnimationData_PAT0(*pat0, material->name, texMapID);
         if (!patData || !patData->texIndexValid) continue;
         uint16_t initialTexIndex = patData->animationTrack[0].texIndex;
         std::string originalTexName = pat0->texNames[initialTexIndex];
         if (patData && patData->texIndexValid) {
             pat0Animators[texMapID].push_back(
                 std::make_unique<PAT0TexAnimator>(
                     controller, pat0, patData, texByName, texMapID, originalTexName
                 )
             );
         }
     }
 }
 
     void BindCLR0(AnimationController * controller, CLR0 * clr0) {
         for (int i = 0; i < (int)AnimatableColor::COUNT; i++) {
             if (!clr0) {
                 continue;
             }

             const CLR0_ColorData* clrData =
                 FindAnimationData_CLR0(*clr0, material->name, (AnimatableColor)i);

             if (clrData) {
                 clr0Animators[i].push_back(
                     std::make_unique<CLR0ColorAnimator>(controller, clr0, clrData)
                 );
             }

         }
     }

     void BindSHP0(AnimationController* controller, SHP0* shp0,MDL0& mdl0)
{
    if (!shp0)
        return;

    for (auto& entry : shp0->entries)
    {
        VtxBufferData* buf = FindShapeByName(entry.vertexNodeName);
        if (!buf) continue;

        MDL0_ShapeEntry* shapeEntry = nullptr;
        for (auto& shape : modelInstance->model->shapes) {
            if (shape.runtime && shape.idVtxPos == buf->id) {
                shapeEntry = &shape;
                break;
            }
        }
        if (!shapeEntry) continue;

        shapeEntry->shpRuntimes.push_back(std::make_unique<Shp0Runtime>());
        Shp0Runtime* shpRt = shapeEntry->shpRuntimes.back().get();

        InitializeMorphTargets(
            *shpRt,
            mdl0,
            *shapeEntry,
            entry
        );

        shp0Animators.push_back(
            std::make_unique<SHP0MorphAnimator>(
                controller,
                &entry,
                shapeEntry->runtime.get(),
                shpRt
            )
        );
    }
}

glm::mat4 calcProjectionPostTexMtx(
    const glm::mat4& clipFromView,
    float flipYScale)
{
    float scaleS = 0.5f;
    float scaleT = -0.5f;
    float transS = 0.5f;
    float transT = 0.5f;

    glm::mat4 dst(1.0f);

dst[0][0] = clipFromView[0][0] * scaleS;
dst[1][0] = clipFromView[0][1] * scaleS;
dst[2][0] = clipFromView[0][2] * scaleS;
dst[3][0] = clipFromView[0][3] * scaleS + transS;

dst[0][1] = clipFromView[1][0] * scaleT;
dst[1][1] = clipFromView[1][1] * scaleT;
dst[2][1] = clipFromView[1][2] * scaleT;
dst[3][1] = clipFromView[1][3] * scaleT + transT;


    dst[2][0] = clipFromView[2][0];
    dst[2][1] = clipFromView[2][1];
    dst[2][2] = clipFromView[2][2];
    dst[2][3] = clipFromView[2][3];

    dst[3][0] = clipFromView[3][0];
    dst[3][1] = clipFromView[3][1];
    dst[3][2] = clipFromView[3][2];
    dst[3][3] = clipFromView[3][3];

    return dst;
}
glm::mat4 calcEnvCameraPostTexMtx(
    const MDL0_TexSrtEntry& texSrt,
    const glm::mat4& viewMatrix,
    const glm::mat4& modelMatrix,
    float flipYScale)
{
    glm::mat3 normalMtx = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
    glm::mat3 view3 = glm::mat3(viewMatrix);
    glm::mat3 srt = glm::mat3(texSrt.effectMtx);

    glm::mat4 post(
        0.5f, 0.0f, 0.0f, 0.0f,
        0.0f, -0.5f * flipYScale, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 1.0f, 1.0f
    );

    glm::mat4 final =
        post *
        glm::mat4(srt) *
        glm::mat4(view3) *
        glm::mat4(normalMtx);

    return final;
}
void calcIndTexMatrix(glm::mat3& dst, int indIdx) {
    const auto& material = this->material;

    int texMtxIdx = 8 + indIdx;

    float a, b, c, d, tx, ty, scale;

    if (!srt0Animators[texMtxIdx].empty()) {
        glm::mat4 tmp;
        srt0Animators[texMtxIdx][0]->CalcIndTexMtx(tmp);

a  = tmp[0][0];
b  = tmp[1][0];
c  = tmp[0][1];
d  = tmp[1][1];
tx = tmp[0][2];
ty = tmp[1][2];
scale = tmp[0][3];
        
    }
    else {
        const auto& mtx = material->indTexMatrices[indIdx];

        a = mtx[0];
        c = mtx[1];
        tx = mtx[2];
        scale = mtx[3];

        b = mtx[4];
        d = mtx[5];
        ty = mtx[6];
    }

dst = glm::mat3(
    a, b, 0.0f,   // col0
    c, d, 0.0f,   // col1
    tx, ty, scale // col2
);

}
int GetTexMtxIndex(EGXTexMatrix m) {
    int raw = (int)m;

    if (raw == 30 || raw == 33 || raw == 36 || raw == 39 ||
        raw == 42 || raw == 45 || raw == 48 || raw == 51 ||
        raw == 54 || raw == 57)
    {
        return (raw - 30) / 3;
    }
    return -1;
}
void calcTexAnimMatrix(glm::mat4& dstPost,int texMtxIdx)
{
    if (srt0Animators[texMtxIdx][0]) {
        srt0Animators[texMtxIdx][0]->CalcTexMtx(dstPost);
    }
}
void CalcTexMatrix(MDL0_MaterialEntry& material, int texMtxIdx, const glm::mat4& cameraProj, glm::mat4 view, glm::mat4 model) {
    if (srt0Animators[texMtxIdx].empty())
        return;
    auto& texSrt = material.texSrts[texMtxIdx];
    float flipYScale = 1.0f;
    glm::mat4& dstPost = material.gxMaterial.postTexMatrices[texMtxIdx];

    if (texSrt.mapMode == MapMode::TEXCOORD) {
        calcTexAnimMatrix(dstPost, texMtxIdx);
        return;
    }
    else if (texSrt.mapMode == MapMode::PROJECTION) {
        dstPost = calcProjectionPostTexMtx(cameraProj, flipYScale);
    }
    else if (texSrt.mapMode == MapMode::ENV_CAMERA) {
        dstPost = calcEnvCameraPostTexMtx(texSrt,view, model, flipYScale);
    }
    else {
        dstPost = glm::mat4(1.0f);
    }
    glm::mat4 srt;
    calcTexAnimMatrix(srt, texMtxIdx);
    std::swap(srt[2], srt[3]);
dstPost = srt * dstPost;
}

void Update(float time, glm::mat4 cameraProj, glm::mat4 view, glm::mat4 model) {
    for (int i = 0; i < 8; i++)
        CalcTexMatrix(*material, i,cameraProj,view,model);


    for (int ind = 0; ind < 3; ind++)
        calcIndTexMatrix(indTexSrtMtx[ind], ind);

    for (int i = 0; i < 8; i++) {
        if (pat0Animators[i].empty())
            continue;

        GLuint texID = 0;
        for (auto& anim : pat0Animators[i])
            anim->FillTexture(texID);

        int gxSlot = i;

for (auto& sampler : material->samplers) {
    if (sampler.texMapSlot == gxSlot) {

        sampler.glTexID = texID;

        glActiveTexture(GL_TEXTURE0 + gxSlot);
        glBindTexture(GL_TEXTURE_2D, texID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ConvertGXWrap((uint8_t)sampler.wrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ConvertGXWrap((uint8_t)sampler.wrapT));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ConvertGXMinFilter((uint8_t)sampler.minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ConvertGXMagFilter((uint8_t)sampler.magFilter));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, sampler.lodBias / 16.0f);

        if ((uint8_t)sampler.minFilter >= 2)
            glGenerateMipmap(GL_TEXTURE_2D);

        break;
    }
}

}




    auto apply = [&](AnimatableColor kind, Color& dst) {
        const int idx = static_cast<int>(kind);
        if (idx < 0 || idx >= static_cast<int>(clr0Animators.size()))
            return;

        Color orig = dst;
        Color current = orig;
        for (auto& anim : clr0Animators[idx]) {
            anim->CalcColor(current, orig);
        }

        dst = current;
    };

    if (material->colorMatRegs.size() > 0)
        apply(AnimatableColor::MAT0, material->colorMatRegs[0]);
    if (material->colorMatRegs.size() > 1)
        apply(AnimatableColor::MAT1, material->colorMatRegs[1]);

    if (material->colorAmbRegs.size() > 0)
        apply(AnimatableColor::AMB0, material->colorAmbRegs[0]);
    if (material->colorAmbRegs.size() > 1)
        apply(AnimatableColor::AMB1, material->colorAmbRegs[1]);

    if (material->colorConstants.size() > 0)
        apply(AnimatableColor::K0, material->colorConstants[0]);
    if (material->colorConstants.size() > 1)
        apply(AnimatableColor::K1, material->colorConstants[1]);
    if (material->colorConstants.size() > 2)
        apply(AnimatableColor::K2, material->colorConstants[2]);
    if (material->colorConstants.size() > 3)
        apply(AnimatableColor::K3, material->colorConstants[3]);

    if (material->colorRegisters.size() > 1)
        apply(AnimatableColor::C0, material->colorRegisters[1]);
    if (material->colorRegisters.size() > 2)
        apply(AnimatableColor::C1, material->colorRegisters[2]);
    if (material->colorRegisters.size() > 3)
        apply(AnimatableColor::C2, material->colorRegisters[3]);

           for (auto& shp0 : shp0Animators)
           {
               shp0->Apply();
           }
        }
    };
    struct ModelInstance
    {
        std::shared_ptr<bres::MDL0> model;
        glm::mat4 referenceFrame;
        Transform transform;

        std::vector<std::unique_ptr<MaterialInstance>> materialInstances;

        ModelInstance(std::shared_ptr<bres::MDL0> mdl)
            : model(std::move(mdl)), referenceFrame(1.0f)
        {
            for (auto& mat : model->materials) {
                materialInstances.push_back(
                    std::make_unique<MaterialInstance>(&mat, this)
                );
            }
        }
        MaterialInstance* GetMaterialInstance(int materialIndex);
        std::vector<std::unique_ptr<SRT0>> loadedSRT0s;
        std::vector<std::unique_ptr<CLR0>> loadedCLR0s;
        std::vector<std::unique_ptr<PAT0>> loadedPAT0s;
        std::vector<std::unique_ptr<CHR0>> loadedCHR0s;
        std::vector<std::unique_ptr<SHP0>> loadedSHP0s;
        void BindSRT0(AnimationController* controller, SRT0* srt0) {
            for (auto& inst : materialInstances)
                inst->BindSRT0(controller, srt0);
        }
        void BindPAT0(AnimationController* controller, PAT0* pat0, const std::unordered_map<std::string, TEX0*>& texByName) {
            for (auto& inst : materialInstances)
                inst->BindPAT0(controller, pat0, texByName);
        }
        void BindCLR0(AnimationController* controller, CLR0* clr0) {
            for (auto& inst : materialInstances)
                inst->BindCLR0(controller, clr0);
        }

        void BindSHP0(AnimationController* controller, SHP0* shp0) {
            for (auto& inst : materialInstances)
                inst->BindSHP0(controller, shp0,*model);
        }

        void Update(float time,glm::mat4 cameraProj, glm::mat4 view, glm::mat4 model) {
            for (auto& inst : materialInstances)
                inst->Update(time, cameraProj,view,model);
        }
        glm::mat4 GetModelMatrix() const {
            return referenceFrame * transform.ToMat4();
        }
        void GatherRenderPackets(std::vector<Packet>& packets, const glm::mat4& view, const glm::vec3& cameraPos);
    };
    uint16_t readIndex(bStream::CStream* s, VCDInfo::AttrType t) {
        switch (t) {
        case VCDInfo::NONE:
            return 0;
        case VCDInfo::INDEX8: {
            uint8_t v = s->readUInt8();
            return v;
        }
        case VCDInfo::INDEX16: {
            uint16_t v = s->readUInt16();
            return v;
        }
        case VCDInfo::DIRECT:
            return 0;
        default:
            return 0xFFFF;
        }
    }
    struct InstancePacketList {
        float modelDepth;
        std::vector<Packet> packets;
    };

inline float readComp(const uint8_t*& p, const VATAttr& a)
{
    float scale = (a.compShift > 0) ? 1.0f / float(1u << a.compShift) : 1.0f;

    switch (a.compType) {
    case 0: { // U8
        uint8_t v = *p++;
        return float(v) * scale;
    }
    case 1: { // S8
        int8_t v = *reinterpret_cast<const int8_t*>(p);
        p += 1;
        return float(v) * scale;
    }
    case 2: { // U16
        uint16_t v = (uint16_t(p[0]) << 8) | p[1];
        p += 2;
        return float(v) * scale;
    }
    case 3: { // S16
        int16_t v = int16_t((uint16_t(p[0]) << 8) | p[1]);
        p += 2;
        return float(v) * scale;
    }
    case 4: { // float32
        uint32_t u =
            (uint32_t(p[0]) << 24) |
            (uint32_t(p[1]) << 16) |
            (uint32_t(p[2]) << 8) |
            (uint32_t(p[3]) << 0);
        p += 4;
        float v;
        memcpy(&v, &u, 4);
        return v;
    }
    }
    return 0.0f;
}

inline void decodeVec3(float* dst, const GX_Array& arr, uint16_t idx, const VATAttr& a)
{
    const uint8_t* p = arr.data + idx * arr.stride;

    float x = readComp(p, a);
    float y = readComp(p, a);
    float z = (a.compCnt == 1) ? readComp(p, a) : 0.0f;

    dst[0] = x;
    dst[1] = y;
    dst[2] = z;
}

inline void decodeVec2(float* dst, const GX_Array& arr, uint16_t idx, const VATAttr& a)
{
    const uint8_t* p = arr.data + idx * arr.stride;

    dst[0] = readComp(p, a);
    dst[1] = readComp(p, a);
}

    void decodeVertex(
        uint8_t* dst,
        bStream::CStream* s,
        const VCDInfo& vcd,
        const VATInfo& vat,
        const std::array<GX_Array, 21>& arrays,
        const VertexLayout& layout,int16_t sda, const LoadedVertexDraw& draw)
    {
        uint8_t pnmtxIdx = 0;
        int uvIndex = sda;

        if (vcd.pnmtx == VCDInfo::AttrType::DIRECT)
            pnmtxIdx = s->readUInt8();

        uint8_t texMtxIdx[8]{};
        for (int i = 0; i < 8; ++i) {
            if (vcd.texmtx[i] == VCDInfo::AttrType::DIRECT)
                texMtxIdx[i] = s->readUInt8();
        }

        uint16_t idxPos = readIndex(s, vcd.pos);
        uint16_t idxNrm = readIndex(s, vcd.nrm);
        uint16_t idxClr0 = readIndex(s, vcd.clr0);
        uint16_t idxClr1 = readIndex(s, vcd.clr1);

uint16_t idxTex[8]{};
for (int i = 0; i < 8; ++i) {
    if (vcd.tex[i] == VCDInfo::AttrType::NONE)
        idxTex[i] = 0;
    else
        idxTex[i] = readIndex(s, vcd.tex[i]);
}

        uint8_t* base = reinterpret_cast<uint8_t*>(dst);

        if (layout.posOffset >= 0 && vcd.pos != VCDInfo::AttrType::NONE) {
            float* pos = reinterpret_cast<float*>(base + layout.posOffset);
            decodeVec3(pos,
                arrays[(int)EGXAttribute::Position],
                idxPos, vat.pos);
        }

        if (layout.nrmOffset >= 0 && vcd.nrm != VCDInfo::AttrType::NONE) {
            float* nrm = reinterpret_cast<float*>(base + layout.nrmOffset);
            decodeVec3(nrm,
                arrays[(int)EGXAttribute::Normal],
                idxNrm, vat.nrm);
        }

        if (layout.clr0Offset >= 0 && vcd.clr0 != VCDInfo::AttrType::NONE) {
            const GX_Array& arr = arrays[(int)EGXAttribute::Color0];
            const uint8_t* cp = arr.data + idxClr0 * arr.stride;
            float* c = reinterpret_cast<float*>(base + layout.clr0Offset);


switch (vat.clr0.compType) {
case (int)EGXComponentType::RGB565: {
uint16_t v = (cp[0] << 8) | cp[1];
    c[0] = ((v >> 11) & 0x1F) / 31.0f;
    c[1] = ((v >> 5)  & 0x3F) / 63.0f;
    c[2] = ( v        & 0x1F) / 31.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGB8: {
    c[0] = cp[0] / 255.0f;
    c[1] = cp[1] / 255.0f;
    c[2] = cp[2] / 255.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGBX8: {
    c[0] = cp[0] / 255.0f;
    c[1] = cp[1] / 255.0f;
    c[2] = cp[2] / 255.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGBA4: {
uint16_t v = (cp[0] << 8) | cp[1];
    c[0] = ((v >> 12) & 0x0F) / 15.0f;
    c[1] = ((v >> 8)  & 0x0F) / 15.0f;
    c[2] = ((v >> 4)  & 0x0F) / 15.0f;
    c[3] = ( v        & 0x0F) / 15.0f;
    break;
}
case (int)EGXComponentType::RGBA6: {
    uint32_t v = (cp[0] << 16) | (cp[1] << 8) | cp[2];
    c[0] = ((v >> 18) & 0x3F) / 63.0f;
    c[1] = ((v >> 12) & 0x3F) / 63.0f;
    c[2] = ((v >> 6)  & 0x3F) / 63.0f;
    c[3] = ( v        & 0x3F) / 63.0f;
    break;
}
case (int)EGXComponentType::RGBA8: {
    uint8_t r = cp[0];
    uint8_t g = cp[1];
    uint8_t b = cp[2];
    uint8_t a = cp[3];

    c[0] = r / 255.0f;
    c[1] = g / 255.0f;
    c[2] = b / 255.0f;
    c[3] = a / 255.0f;
    break;
}
}
        }

        if (layout.clr1Offset >= 0 && vcd.clr1 != VCDInfo::AttrType::NONE) {
            const GX_Array& arr = arrays[(int)EGXAttribute::Color1];
            const uint8_t* cp = arr.data + idxClr1 * arr.stride;
            float* c = reinterpret_cast<float*>(base + layout.clr1Offset);


switch (vat.clr1.compType) {
case (int)EGXComponentType::RGB565: {
uint16_t v = (cp[0] << 8) | cp[1];
    c[0] = ((v >> 11) & 0x1F) / 31.0f;
    c[1] = ((v >> 5)  & 0x3F) / 63.0f;
    c[2] = ( v        & 0x1F) / 31.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGB8: {
    c[0] = cp[0] / 255.0f;
    c[1] = cp[1] / 255.0f;
    c[2] = cp[2] / 255.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGBX8: {
    c[0] = cp[0] / 255.0f;
    c[1] = cp[1] / 255.0f;
    c[2] = cp[2] / 255.0f;
    c[3] = 1.0f;
    break;
}
case (int)EGXComponentType::RGBA4: {
uint16_t v = (cp[0] << 8) | cp[1];
    c[0] = ((v >> 12) & 0x0F) / 15.0f;
    c[1] = ((v >> 8)  & 0x0F) / 15.0f;
    c[2] = ((v >> 4)  & 0x0F) / 15.0f;
    c[3] = ( v        & 0x0F) / 15.0f;
    break;
}
case (int)EGXComponentType::RGBA6: {
    uint32_t v = (cp[0] << 16) | (cp[1] << 8) | cp[2];
    c[0] = ((v >> 18) & 0x3F) / 63.0f;
    c[1] = ((v >> 12) & 0x3F) / 63.0f;
    c[2] = ((v >> 6)  & 0x3F) / 63.0f;
    c[3] = ( v        & 0x3F) / 63.0f;
    break;
}
case (int)EGXComponentType::RGBA8: {
    uint8_t r = cp[0];
    uint8_t g = cp[1];
    uint8_t b = cp[2];
    uint8_t a = cp[3];

    c[0] = r / 255.0f;
    c[1] = g / 255.0f;
    c[2] = b / 255.0f;
    c[3] = a / 255.0f;
    break;
}
}
        }

for (int i = 0; i < 8; ++i) {
    if (layout.texOffset[i] < 0) continue;
    if (vcd.tex[i] == VCDInfo::AttrType::NONE) continue;

    float* tex = reinterpret_cast<float*>(base + layout.texOffset[i]);

    decodeVec2(
        tex,
        arrays[(int)EGXAttribute::TexCoord0 + i],
        idxTex[i],
        vat.tex[i]
    );
}
if (layout.boneIndexOffset >= 0 && layout.boneWeightOffset >= 0)
{
    
    uint32_t* outIdx = reinterpret_cast<uint32_t*>(base + layout.boneIndexOffset);
    float*    outW   = reinterpret_cast<float*>(base + layout.boneWeightOffset);

    int pmi = pnmtxIdx;
    uint32_t mtx = draw.posMatrixTable[pmi];

    if (mtx == 0xFFFF)
        mtx = 0;

    outIdx[0] = mtx;
    outW[0]   = 1.0f;

    for (int i = 1; i < 4; i++) {
        outIdx[i] = 0;
        outW[i]   = 0.0f;
    }
}
    }
    LoadedVertexData runVertices(
        const VATInfo& vat,
        const std::array<VCDInfo, 8>& vcds,
        const std::array<GX_Array, 21>& arrays,
        const ParsedDL& dl,
        const VertexLayout& layout,
        bStream::CStream* stream,
        size_t dlBase,int16_t sda)
    {
        LoadedVertexData out{};
        out.layout = layout;
        out.totalVertexCount = dl.totalVertexCount;

        size_t floatsPerVertex = layout.stride;
        out.vertexBuffer.resize(out.totalVertexCount * floatsPerVertex);
        uint32_t dstFloatOffs = 0;

        out.indexBuffer.reserve(dl.totalIndexCount);

        uint32_t baseVertex = 0;

        for (size_t drawIndex = 0; drawIndex < dl.drawCalls.size(); ++drawIndex) {
            const DrawCall& dc = dl.drawCalls[drawIndex];
            const LoadedVertexDraw& draw = dl.draws[dc.drawIndex];

            uint32_t vc = dc.vertexCount;



            switch (dc.primType) {
            case 0x90:
                for (uint32_t i = 0; i < vc; ++i)
                    out.indexBuffer.push_back(baseVertex + i);
                break;

            case 0x98:
                for (uint32_t i = 0; i + 2 < vc; ++i) {
                    if (i & 1) {
                        out.indexBuffer.push_back(baseVertex + i + 1);
                        out.indexBuffer.push_back(baseVertex + i);
                        out.indexBuffer.push_back(baseVertex + i + 2);
                    }
                    else {
                        out.indexBuffer.push_back(baseVertex + i);
                        out.indexBuffer.push_back(baseVertex + i + 1);
                        out.indexBuffer.push_back(baseVertex + i + 2);
                    }
                }
                break;

            case 0xA0:
                for (uint32_t i = 1; i + 1 < vc; ++i) {
                    out.indexBuffer.push_back(baseVertex + 0);
                    out.indexBuffer.push_back(baseVertex + i);
                    out.indexBuffer.push_back(baseVertex + i + 1);
                }
                break;

            case 0x80: {
                for (uint32_t i = 0; i + 3 < vc; i += 4) {
                    out.indexBuffer.push_back(baseVertex + i + 0);
                    out.indexBuffer.push_back(baseVertex + i + 1);
                    out.indexBuffer.push_back(baseVertex + i + 2);

                    out.indexBuffer.push_back(baseVertex + i + 0);
                    out.indexBuffer.push_back(baseVertex + i + 2);
                    out.indexBuffer.push_back(baseVertex + i + 3);
                }
                break;
            }
            case 0x88: {
                for (uint32_t i = 0; i + 3 < vc; i += 2) {
                    out.indexBuffer.push_back(baseVertex + i + 0);
                    out.indexBuffer.push_back(baseVertex + i + 1);
                    out.indexBuffer.push_back(baseVertex + i + 2);

                    out.indexBuffer.push_back(baseVertex + i + 1);
                    out.indexBuffer.push_back(baseVertex + i + 3);
                    out.indexBuffer.push_back(baseVertex + i + 2);
                }
                break;
            }
            }

            baseVertex += vc;

            const VCDInfo& vcd = vcds[dc.vertexFormat];

            size_t drawCallPos = dlBase + dc.srcOffs;
            stream->seek(drawCallPos);

            for (uint32_t i = 0; i < dc.vertexCount; ++i) {
                uint8_t* dst = out.vertexBuffer.data() + dstFloatOffs;

                size_t cur = stream->tell();

                decodeVertex(dst, stream, vcd, vat, arrays, layout,sda,draw);

                dstFloatOffs += floatsPerVertex;

            }

        }
        return out;
    };

glm::mat4 MVP;
    std::shared_ptr<RRES> rres;
    std::vector<std::shared_ptr<MDL0>> allLoadedModels;
    bool init = false;
    GLuint id;
    GLuint depthTextureID = 0;
    GLuint zbufferTex = 0;
    std::vector<Packet> packets;
    std::unordered_map<std::string, TEX0*> texByName;
    std::unique_ptr<AnimationController> animationController =
        std::make_unique<AnimationController>();
    int MapChannelIdToColorIndex(EGXRasColorChannelSlot chan);
    std::string GetTevColorInput(EGXCombineColorInput in, int i);
    std::string GetTevAlphaInput(EGXCombineAlphaInput in, int i);
    std::string ApplyBias(const std::string& expr, EGXTevBias bias);
    std::string ApplyScale(const std::string& expr, EGXTevScale scale);
    std::string ApplyClamp(const std::string& expr, bool clamp);
    std::string ApplyTevOpColor(const std::string& a, const std::string& b, const std::string& c, const std::string& d, EGXTevOp op, EGXTevBias bias, EGXTevScale scale, bool clamp,EGXCombineColorInput inputC);
    int DecodeKonstColorSel(uint8_t sel);
    std::string ApplyTevOpAlpha(const std::string& a, const std::string& b, const std::string& c, const std::string& d, EGXTevOp op, EGXTevBias bias, EGXTevScale scale, bool clamp,EGXCombineAlphaInput inputC);
    std::string ApplySwapTable2(const std::string& src, const SwapTable& table);
    int DecodeKonstAlphaSel(uint8_t raw);
    std::string GenerateTevStageGLSL(const TevStage& s, const GXMaterial& mat, int i);
    float ConvertIndScale(EGXIndirectTexScale s);
    int ConvertIndWrap(EGXIndirectWrapMode w);
    std::string GenerateIndTexStageGLSL(const IndTexStage& s, int stageIndex, const TevStage& a, int tevIndex);
    std::string GLSLCompare(const std::string& a, const std::string& b, EGXCompareType c);
    std::string GLSLAlphaOp(const std::string& a, const std::string& b, EGXAlphaOp op);
    std::string GenerateAlphaTestGLSL(const AlphaTest& at);
    std::string GenerateFogGLSL();
    std::string GenerateTevPipelineGLSL(const GXMaterial& mat, const std::vector<Color>& colorConstants);
    std::string GenerateUniformsGLSL(const GXMaterial& mat, int numTextures);
    std::string GenerateFragmentShader(const GXMaterial& mat, int numTextures, const std::vector<Color>& kColors);
    int GetTexMtxIndex(EGXTexMatrix m);
    int GetPostTexMtxIndex(EGXPostTexGenMatrix m);
    std::string GenerateTevTexCoordWrapN(const std::string& coord, EGXIndirectWrapMode wrap);
    std::string GenerateTevTexCoordWrap(const TevStage& stage, const GXMaterial& material);
    std::string GenerateTexGenGLSL(const TexGen& tg, int index, const ShapeRuntime& runtime);
    std::string GenerateVertexShader(const GXMaterial& gxMat, const std::vector<MDL0_TexSrtEntry>& texSrts,const ShapeRuntime& runtime);
    static glm::mat4 computeModelMatrixSRT(float sx, float sy, float sz, float rx, float ry, float rz, float tx, float ty, float tz);
    std::string SwapModeToGLSL(EGXSwapMode m);
    std::string ApplySwapTable(const std::string& src, const SwapTable& table);
    static uint32_t getAttributeFormatCompFlagsRaw(EGXAttribute3 attr, EGXComponentCount compCnt);
    static void calcTexMtx_Basic(glm::mat4& dst, float scaleS, float scaleT, float rotation, float translationS, float translationT);
    static void calcTexMtx_Maya(glm::mat4& dst, float scaleS, float scaleT, float rotation, float translationS, float translationT);
    static void calcTexMtx_XSI(glm::mat4& dst, float scaleS, float scaleT, float rotation, float translationS, float translationT);
    static void calcTexMtx_Max(glm::mat4& dst, float scaleS, float scaleT, float rotation, float translationS, float translationT);
    static void calcTexMtx(glm::mat4& dst, TexMatrixMode mode, float scaleS, float scaleT, float rotation, float translationS, float translationT);
    std::string ReadString(bStream::CStream* base, size_t p);
    std::vector<bres::ResDicEntry> ParseResDic(bStream::CStream* stream, uint32_t tableOffs);
    std::optional<bres::ResUserData> ParseUserData(bStream::CStream* stream, size_t sectionSize, uint32_t offs);
    bres::PLT0 ParsePLT0(bStream::CStream* stream, size_t p);
    bres::TEX0 ParseTEX0(bStream::CStream* stream, size_t p);
    void ParseMDL0_TevEntry(bStream::CStream* stream, DisplayListRegisters& r, uint32_t numStagesCheck,size_t p);
    bres::MDL0_MaterialEntry ParseMDL0_MaterialEntry(bStream::CStream* stream, uint32_t version, size_t p);
    bres::VtxBufferData parseMDL0_VtxData(bStream::CStream* stream, size_t mdl0Size, uint32_t entryOffs, EGXAttribute3 vtxAttrib, size_t p);
    std::vector<bres::VtxBufferData> parseInputBufferSet(bStream::CStream* stream, size_t size, EGXAttribute3 vtxAttrib, const std::vector<ResDicEntry>& resDic, size_t p);
    bres::InputVertexBuffers parseInputVertexBuffers(bStream::CStream* stream, size_t size, const std::vector<ResDicEntry>& posDic, const std::vector<ResDicEntry>& nrmDic, const std::vector<ResDicEntry>& clrDic, const std::vector<ResDicEntry>& txcDic,size_t p);
    const bres::ResDicEntry* FindResDicEntry(const std::vector<ResDicEntry>& dic, const std::string& name);
    bres::MDL0_ShapeEntry parseMDL0_ShapeEntry(bStream::CStream* stream, const InputVertexBuffers& mdl0Base,size_t p, size_t ps);
    bres::MDL0_NodeEntry parseMDL0_NodeEntry(size_t entryPtr, size_t nodeBasePtr, size_t mdl0Base, bStream::CStream* stream);
    std::vector<bres::NodeTreeOp> parseMDL0_NodeTreeBytecode(bStream::CStream* stream, size_t size, size_t p);
    const bres::ResDicEntry* findResDic(const std::vector<ResDicEntry>& dic, const std::string& name);
    std::vector<bres::NodeMixOp> parseMDL0_NodeMixBytecode(bStream::CStream* stream, size_t size, size_t p);
    std::vector<bres::DrawOp> parseMDL0_DrawBytecode(bStream::CStream* stream, size_t size, size_t p);
    bres::MDL0_SceneGraph parseMDL0_SceneGraph(bStream::CStream* stream, size_t fileSize, const std::vector<ResDicEntry>& byteCodeResDic,size_t p);
    static uint32_t directAttrSize(EGXAttribute3 attr, const VATAttr& fmt);
    static int getIndexNumComponents(EGXAttribute3 attr, const VATAttr& fmt);
    static uint32_t getIndexNumComponents(int attrIndex, const VATAttr& fmt);
    static uint32_t getAttributeByteSizeRaw(int attrIndex, const VATAttr& fmt);
    uint32_t computeSrcVertexSize(const VATInfo& vat, const VCDInfo& vcd);
    bres::ParsedDL parseDisplayList(size_t p, bStream::CStream* stream, uint32_t primDLSize, const std::array<VCDInfo, 8>& vcds, VATInfo& vat);
    bres::MDL0 parseMDL0(bStream::CStream* stream, size_t fileSize,size_t p);
    static float  SampleFloatAnimationTrackLinear(const FloatTrackLinear& track, float frame);
    static float GetPointHermite(float p0, float p1, float s0, float s1, float t);
    static float HermiteInterpolate(const FloatKeyHermite& k0, const FloatKeyHermite& k1, float frame);
    static float SampleFloatAnimationTrackHermite(const FloatTrackHermite& track, float frame);
    static float SampleFloatTrack(const FloatAnimationTrack& track, float frame);
    static float GetAnimFrame(const AnimationBase& anim, float frame);
    static float Hermite(float p0, float p1, float s0, float s1, float t);
    bool SampleAnimationTrackBoolean(const BitMap& frames, float animFrame);
    bres::FloatAnimationTrack ParseAnimationTrackC8(bStream::CStream* stream, int numKeyframes, size_t p);
    bres::FloatAnimationTrack ParseAnimationTrackC16(bStream::CStream* stream, int numKeyframes, size_t p);
    bres::FloatAnimationTrack ParseAnimationTrackC32(bStream::CStream* stream, int numKeyframes, size_t p);
    bres::FloatAnimationTrack ParseAnimationTrackF32(bStream::CStream* stream, size_t p);
    bres::FloatAnimationTrack ParseAnimationTrackF48(bStream::CStream* stream, size_t p);
    bres::FloatAnimationTrack ParseAnimationTrackF96(bStream::CStream* stream, size_t p);
    bres::FloatAnimationTrack MakeConstantAnimationTrack(float value);
    bres::FloatAnimationTrack ParseAnimationTrackF96OrConst(bStream::CStream* stream,bool isConstant, size_t p);
    static float  SampleFloatAnimationTrack(const FloatAnimationTrack& track, float frame);
    bres::SRT0_TexData ParseSRT0_TexData(bStream::CStream* stream, size_t p);
    bres::SRT0_MatData ParseSRT0_MatData(bStream::CStream* stream, size_t p);
    bres::SRT0 ParseSRT0(bStream::CStream* stream, size_t fileSize, size_t p);
    std::vector<bres::PAT0_TexFrameData> ParsePAT0_TexFrameTrack(bStream::CStream* stream, size_t size, size_t p);
    bres::PAT0_MatData ParsePAT0_MatData(bStream::CStream* stream, size_t p);
    bres::PAT0 ParsePAT0(bStream::CStream* stream, size_t p);
    std::vector<uint32_t> ParseAnimationTrackColor(bStream::CStream* stream, uint16_t numKeyframes, bool isConstant, size_t p);
    static uint32_t SampleAnimationTrackColor(const std::vector<uint32_t>& frames, float frame);
    static uint32_t ColorToRGBA8(const Color& c);
    bres::CLR0_MatData ParseCLR0_MatData(bStream::CStream* stream,uint16_t numKeyframes,size_t p);
    static const bres::CLR0_ColorData* FindAnimationData_CLR0(const CLR0& clr0, const std::string& materialName, AnimatableColor color);
    VCDInfo buildVCD(uint32_t vcdL, uint32_t vcdH);
    bres::CLR0 ParseCLR0(bStream::CStream* stream, size_t p);
    bres::CHR0_NodeData ParseCHR0_NodeData(bStream::CStream* stream,uint16_t numKeyframes, size_t p);
    bres::CHR0 ParseCHR0(bStream::CStream* stream, size_t p);
    int ToTexMtxIndex(int m);
    int ToPostMtxIndex(int m);
    GLenum toGLType(EGXComponentType t);
    bool toNormalized(EGXComponentType t);
    uint32_t RGB565toRGBA8(uint16_t data);
    std::vector<std::string> ParseSHP0Section1_VertexNames(bStream::CStream* stream, size_t base, uint32_t stringListOffset);
    bres::SHP0 ParseSHP0(bStream::CStream* stream, size_t p);
    uint32_t RGB5A3toRGBA8(uint16_t data);
    GLuint UploadTEX0Texture(bStream::CStream* stream, const TEX0& tex);
    bres::ShapeRuntime buildShapeData(const LoadedVertexData& vtx);
    void SetLights(const LightSet& ls);
    bres::RRES ParseRRES(bStream::CStream* stream, size_t size);
    GLuint BuildMaterialShader(const MDL0_MaterialEntry& mat,const ShapeRuntime& runtime);
    void clearinstance();
    void LoadAllAnimations();
    void Loader(bStream::CStream* stream, int modelId, glm::vec3 pos, glm::vec3 rot, glm::vec3 scale);
    void Update(float deltaTime,glm::mat4 cameraProj, glm::mat4 view);
    void Renderer(float windowWidth, float windowHeight, glm::mat4 view, glm::mat4 proj, fog::BFG a, bool b);
    void UploadFogUniforms(GLuint prog, const fog::BFGEntry& fog)
    {
        glUseProgram(prog);

        glUniform1i(glGetUniformLocation(prog, "u_FogType"), fog.type);
        glUniform1i(glGetUniformLocation(prog, "u_FogAdjEnabled"), fog.rangeCorrection != 0);

        glUniform4f(glGetUniformLocation(prog, "u_FogColor"),
            fog.color.r, fog.color.g, fog.color.b, 1.0f);

glUniform1f(glGetUniformLocation(prog, "u_FogStartZ"), fog.startZ);
glUniform1f(glGetUniformLocation(prog, "u_FogEndZ"), fog.endZ);


        glUniform1f(glGetUniformLocation(prog, "u_FogRangeAdjCenter"),
            float(fog.rangeCenter));

        float scale = (fog.rangeCorrection != 0) ? 0.5f : 1.0f;
        float scale2 = scale / (fog.endZ - fog.startZ);

        glUniform1f(glGetUniformLocation(prog, "u_FogRangeAdjScale"), scale2);
    }
};