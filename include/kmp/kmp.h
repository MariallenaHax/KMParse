#pragma once
#include "glm/glm.hpp"
#include "bstream.h"
#include <cstring>
class kmp
{
public:
    struct GfxNodeRendererTransform;
    struct KTPT { glm::vec3 pos; glm::vec3 rot; uint16_t playerIndex; uint16_t padding; GfxNodeRendererTransform* renderer = nullptr; GfxNodeRendererTransform* rendererStartZone = nullptr;bool selected = false;};
    struct ENPT { glm::vec3 pos; float deviation; uint16_t s1; uint8_t s2, s3; bool selected = false;};
    struct ENPH { uint8_t start, num; uint8_t prev[6]; uint8_t next[6]; uint16_t unk; };
    struct ITPT { glm::vec3 pos; float deviation; uint16_t s1, s2; bool selected = false;};
    struct ITPH { uint8_t start, num; uint8_t prev[6]; uint8_t next[6]; uint16_t pad; };

    struct CKPT {
        float x1, z1, x2, z2;
        uint8_t respawn, type, prev, next;
        bool selected = false;
    };

    struct CKPH { uint8_t start, num; uint8_t prev[6]; uint8_t next[6]; uint16_t pad; };

    struct GOBJ {
        uint16_t id;
        uint16_t route;
        glm::vec3 pos;
        glm::vec3 rot;
        glm::vec3 scale;
        uint16_t args[8];
        uint32_t presence;
        bool selected = false;
    };

    struct POTI_Point { glm::vec3 pos; uint16_t s1, s2; };
    struct POTI { uint16_t num; uint8_t s1, s2; std::vector<POTI_Point> points; bool selected = false;};

    struct AREA {
        uint8_t shape, type, cam, priority;
        glm::vec3 pos, rot, scale;
        uint16_t s1, s2;
        uint8_t route, enemy;
        uint16_t pad;
        bool selected = false;
    };

    struct CAME {
        uint8_t type, nextCam, shake, route;
        uint16_t vCam, vZoom, vView;
        uint8_t start, movie;
        glm::vec3 pos, rot;
        float zoomStart, zoomEnd;
        glm::vec3 viewStart, viewEnd;
        float time;
        bool selected = false;
    };

    struct JGPT { glm::vec3 pos, rot; uint16_t id, sound; bool selected = false;};
    struct CNPT { glm::vec3 pos, rot; uint16_t id, effect; bool selected = false;};
    struct MSPT { glm::vec3 pos, rot; uint16_t id, pad; bool selected = false;};

    struct STGI {
        uint8_t lap, pole, dist, flare;
        uint8_t unk1;
        uint8_t flareColor[4];
        uint8_t unk2;
        float speedMod;
    };
    struct KMP {
        std::vector<KTPT> ktpt;
        std::vector<ENPT> enpt;
        std::vector<ENPH> enph;
        std::vector<ITPT> itpt;
        std::vector<ITPH> itph;
        std::vector<CKPT> ckpt;
        std::vector<CKPH> ckph;
        std::vector<GOBJ> gobj;
        std::vector<POTI> poti;
        std::vector<AREA> area;
        std::vector<CAME> came;
        std::vector<JGPT> jgpt;
        std::vector<CNPT> cnpt;
        std::vector<MSPT> mspt;
        STGI stgi;
        uint8_t openingCamera;
        uint8_t previewCamera;
    };
    KMP parseKMP(bStream::CStream* stream);
    void saveKMP(const KMP& kmp, bStream::CStream* s);
    uint32_t CalculateKMPSize(const KMP& kmp);
};
inline bool operator==(const kmp::STGI& a, const kmp::STGI& b)
{
    return
        a.lap == b.lap &&
        a.pole == b.pole &&
        a.dist == b.dist &&
        a.flare == b.flare &&
        a.unk1 == b.unk1 &&
        std::memcmp(a.flareColor, b.flareColor, 4) == 0 &&
        a.unk2 == b.unk2 &&
        a.speedMod == b.speedMod;
}
inline bool operator==(const kmp::KTPT& a, const kmp::KTPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.playerIndex == b.playerIndex &&
        a.padding == b.padding;
}
inline bool operator==(const kmp::ENPT& a, const kmp::ENPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.deviation == b.deviation &&
        a.s1 == b.s1 &&
        a.s2 == b.s2 &&
        a.s3 == b.s3;
}
inline bool operator==(const kmp::ENPH& a, const kmp::ENPH& b)
{
    return
        a.start == b.start &&
        a.num == b.num &&
        std::memcmp(a.prev, b.prev, 6) == 0 &&
        std::memcmp(a.next, b.next, 6) == 0 &&
        a.unk == b.unk;
}
inline bool operator==(const kmp::ITPT& a, const kmp::ITPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.deviation == b.deviation &&
        a.s1 == b.s1 &&
        a.s2 == b.s2;
}
inline bool operator==(const kmp::ITPH& a, const kmp::ITPH& b)
{
    return
        a.start == b.start &&
        a.num == b.num &&
        std::memcmp(a.prev, b.prev, 6) == 0 &&
        std::memcmp(a.next, b.next, 6) == 0 &&
        a.pad == b.pad;
}
inline bool operator==(const kmp::CKPT& a, const kmp::CKPT& b)
{
    return
        a.x1 == b.x1 &&
        a.z1 == b.z1 &&
        a.x2 == b.x2 &&
        a.z2 == b.z2 &&
        a.respawn == b.respawn &&
        a.type == b.type &&
        a.prev == b.prev &&
        a.next == b.next;
}
inline bool operator==(const kmp::CKPH& a, const kmp::CKPH& b)
{
    return
        a.start == b.start &&
        a.num == b.num &&
        std::memcmp(a.prev, b.prev, 6) == 0 &&
        std::memcmp(a.next, b.next, 6) == 0 &&
        a.pad == b.pad;
}
inline bool operator==(const kmp::GOBJ& a, const kmp::GOBJ& b)
{
    return
        a.id == b.id &&
        a.route == b.route &&
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.scale.x == b.scale.x &&
        a.scale.y == b.scale.y &&
        a.scale.z == b.scale.z &&
        std::memcmp(a.args, b.args, sizeof(a.args)) == 0 &&
        a.presence == b.presence;
}
inline bool operator==(const kmp::POTI_Point& a, const kmp::POTI_Point& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.s1 == b.s1 &&
        a.s2 == b.s2;
}

inline bool operator==(const kmp::POTI& a, const kmp::POTI& b)
{
    return
        a.num == b.num &&
        a.s1 == b.s1 &&
        a.s2 == b.s2 &&
        a.points == b.points;
}
inline bool operator==(const kmp::AREA& a, const kmp::AREA& b)
{
    return
        a.shape == b.shape &&
        a.type == b.type &&
        a.cam == b.cam &&
        a.priority == b.priority &&
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.scale.x == b.scale.x &&
        a.scale.y == b.scale.y &&
        a.scale.z == b.scale.z &&
        a.s1 == b.s1 &&
        a.s2 == b.s2 &&
        a.route == b.route &&
        a.enemy == b.enemy &&
        a.pad == b.pad;
}
inline bool operator==(const kmp::CAME& a, const kmp::CAME& b)
{
    return
        a.type == b.type &&
        a.nextCam == b.nextCam &&
        a.shake == b.shake &&
        a.route == b.route &&
        a.vCam == b.vCam &&
        a.vZoom == b.vZoom &&
        a.vView == b.vView &&
        a.start == b.start &&
        a.movie == b.movie &&
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.zoomStart == b.zoomStart &&
        a.zoomEnd == b.zoomEnd &&
        a.viewStart.x == b.viewStart.x &&
        a.viewStart.y == b.viewStart.y &&
        a.viewStart.z == b.viewStart.z &&
        a.viewEnd.x == b.viewEnd.x &&
        a.viewEnd.y == b.viewEnd.y &&
        a.viewEnd.z == b.viewEnd.z &&
        a.time == b.time;
}
inline bool operator==(const kmp::JGPT& a, const kmp::JGPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.id == b.id &&
        a.sound == b.sound;
}

inline bool operator==(const kmp::CNPT& a, const kmp::CNPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.id == b.id &&
        a.effect == b.effect;
}

inline bool operator==(const kmp::MSPT& a, const kmp::MSPT& b)
{
    return
        a.pos.x == b.pos.x &&
        a.pos.y == b.pos.y &&
        a.pos.z == b.pos.z &&
        a.rot.x == b.rot.x &&
        a.rot.y == b.rot.y &&
        a.rot.z == b.rot.z &&
        a.id == b.id &&
        a.pad == b.pad;
}
inline bool operator==(const kmp::KMP& a, const kmp::KMP& b)
{
    return
        a.ktpt == b.ktpt &&
        a.enpt == b.enpt &&
        a.enph == b.enph &&
        a.itpt == b.itpt &&
        a.itph == b.itph &&
        a.ckpt == b.ckpt &&
        a.ckph == b.ckph &&
        a.gobj == b.gobj &&
        a.poti == b.poti &&
        a.area == b.area &&
        a.came == b.came &&
        a.jgpt == b.jgpt &&
        a.cnpt == b.cnpt &&
        a.mspt == b.mspt &&
        a.stgi == b.stgi &&
        a.openingCamera == b.openingCamera &&
        a.previewCamera == b.previewCamera;
}