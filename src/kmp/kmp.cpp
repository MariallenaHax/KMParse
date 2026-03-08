#include "kmp/kmp.h"
kmp::KMP kmp::parseKMP(bStream::CStream* s)
{
KMP out;

char magic[4];
s->readBytesTo((uint8_t*)magic, 4);
if (memcmp(magic, "RKMD", 4) != 0)
throw std::runtime_error("Invalid KMP magic");


uint32_t fileLen = s->readUInt32();
uint16_t sectionNum = s->readUInt16();
uint16_t headerLen = s->readUInt16();
uint16_t versionNum = s->readUInt32();

if (sectionNum != 15)
throw std::runtime_error("Unexpected KMP section count");

std::vector<uint32_t> offsets(sectionNum);
for (int i = 0; i < sectionNum; i++)
    offsets[i] = s->readUInt32();

if (s->tell() != headerLen)
throw std::runtime_error("Invalid KMP header length");

static const char* order[15] = {
    "KTPT","ENPT","ENPH","ITPT","ITPH",
    "CKPT","CKPH","GOBJ","POTI","AREA",
    "CAME","JGPT","CNPT","MSPT","STGI"
};

for (int sec = 0; sec < 15; sec++)
{
    uint32_t off = offsets[sec];

   s->seek(off + headerLen);

    char idbuf[4];
    s->readBytesTo((uint8_t*)idbuf, 4);
    std::string id(idbuf, 4);

    uint16_t count = s->readUInt16();
    uint16_t headerData = s->readUInt16();

    if (id == "KTPT") {
        out.ktpt.reserve(count);
        for (int i = 0; i < count; i++) {
            KTPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.playerIndex = s->readUInt16();
            e.padding = s->readUInt16();
            out.ktpt.push_back(e);
        }
    }

    else if (id == "ENPT") {
        out.enpt.reserve(count);
        for (int i = 0; i < count; i++) {
            ENPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.deviation = s->readFloat();
            e.s1 = s->readUInt16();
            e.s2 = s->readUInt8();
            e.s3 = s->readUInt8();
            out.enpt.push_back(e);
        }
    }

    else if (id == "ENPH") {
        out.enph.reserve(count);
        for (int i = 0; i < count; i++) {
            ENPH e;
            e.start = s->readUInt8();
            e.num = s->readUInt8();
            s->readBytesTo(e.prev, 6);
            s->readBytesTo(e.next, 6);
            e.unk = s->readUInt16();
            out.enph.push_back(e);
        }
    }

    else if (id == "ITPT") {
        out.itpt.reserve(count);
        for (int i = 0; i < count; i++) {
            ITPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.deviation = s->readFloat();
            e.s1 = s->readUInt16();
            e.s2 = s->readUInt16();
            out.itpt.push_back(e);
        }
    }

    else if (id == "ITPH") {
        out.itph.reserve(count);
        for (int i = 0; i < count; i++) {
            ITPH e;
            e.start = s->readUInt8();
            e.num = s->readUInt8();
            s->readBytesTo(e.prev, 6);
            s->readBytesTo(e.next, 6);
            e.pad = s->readUInt16();
            out.itph.push_back(e);
        }
    }

    else if (id == "CKPT") {
        out.ckpt.reserve(count);
        for (int i = 0; i < count; i++) {
            CKPT e;
            e.x1 = s->readFloat();
            e.z1 = s->readFloat();
            e.x2 = s->readFloat();
            e.z2 = s->readFloat();
            e.respawn = s->readUInt8();
            e.type = s->readUInt8();
            e.prev = s->readUInt8();
            e.next = s->readUInt8();
            out.ckpt.push_back(e);
        }
    }

    else if (id == "CKPH") {
        out.ckph.reserve(count);
        for (int i = 0; i < count; i++) {
            CKPH e;
            e.start = s->readUInt8();
            e.num = s->readUInt8();
            s->readBytesTo(e.prev, 6);
            s->readBytesTo(e.next, 6);
            e.pad = s->readUInt16();
            out.ckph.push_back(e);
        }
    }

    else if (id == "GOBJ") {
        out.gobj.reserve(count);
        for (int i = 0; i < count; i++) {
            GOBJ g;
            g.id = s->readUInt16();
            g.route = s->readUInt16();
            g.pos.x = s->readFloat();
            g.pos.y = s->readFloat();
            g.pos.z = s->readFloat();
            g.rot.x = s->readFloat();
            g.rot.y = s->readFloat();
            g.rot.z = s->readFloat();
            g.scale.x = s->readFloat();
            g.scale.y = s->readFloat();
            g.scale.z = s->readFloat();
            for (int a = 0; a < 8; a++)
                g.args[a] = s->readUInt16();
            g.presence = s->readUInt32();
            out.gobj.push_back(g);
        }
    }

    else if (id == "POTI") {
        out.poti.reserve(count);
        for (int i = 0; i < count; i++) {
            POTI p;
            p.num = s->readUInt16();
            p.s1 = s->readUInt8();
            p.s2 = s->readUInt8();

            for (int j = 0; j < p.num; j++) {
                POTI_Point pt;
                pt.pos.x = s->readFloat();
                pt.pos.y = s->readFloat();
                pt.pos.z = s->readFloat();
                pt.s1 = s->readUInt16();
                pt.s2 = s->readUInt16();
                p.points.push_back(pt);
            }
            out.poti.push_back(p);
        }
    }

    else if (id == "AREA") {
        out.area.reserve(count);
        for (int i = 0; i < count; i++) {
            AREA e;
            e.shape = s->readUInt8();
            e.type = s->readUInt8();
            e.cam = s->readUInt8();
            e.priority = s->readUInt8();
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.scale.x = s->readFloat();
            e.scale.y = s->readFloat();
            e.scale.z = s->readFloat();
            e.s1 = s->readUInt16();
            e.s2 = s->readUInt16();
            e.route = s->readUInt8();
            e.enemy = s->readUInt8();
            e.pad = s->readUInt16();
            out.area.push_back(e);
        }
    }

    else if (id == "CAME") {
        out.came.reserve(count);
        for (int i = 0; i < count; i++) {
            CAME e;
            e.type = s->readUInt8();
            e.nextCam = s->readUInt8();
            e.shake = s->readUInt8();
            e.route = s->readUInt8();
            e.vCam = s->readUInt16();
            e.vZoom = s->readUInt16();
            e.vView = s->readUInt16();
            e.start = s->readUInt8();
            e.movie = s->readUInt8();
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.zoomStart = s->readFloat();
            e.zoomEnd = s->readFloat();
            e.viewStart.x = s->readFloat();
            e.viewStart.y = s->readFloat();
            e.viewStart.z = s->readFloat();
            e.viewEnd.x = s->readFloat();
            e.viewEnd.y = s->readFloat();
            e.viewEnd.z = s->readFloat();
            e.time = s->readFloat();
            out.openingCamera = headerData >> 8;
            out.previewCamera = headerData & 0xFF;
            out.came.push_back(e);
        }
    }

    else if (id == "JGPT") {
        out.jgpt.reserve(count);
        for (int i = 0; i < count; i++) {
            JGPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.id = s->readUInt16();
            e.sound = s->readUInt16();
            out.jgpt.push_back(e);
        }
    }

    else if (id == "CNPT") {
        out.cnpt.reserve(count);
        for (int i = 0; i < count; i++) {
            CNPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.id = s->readUInt16();
            e.effect = s->readUInt16();
            out.cnpt.push_back(e);
        }
    }

    else if (id == "MSPT") {
        out.mspt.reserve(count);
        for (int i = 0; i < count; i++) {
            MSPT e;
            e.pos.x = s->readFloat();
            e.pos.y = s->readFloat();
            e.pos.z = s->readFloat();
            e.rot.x = s->readFloat();
            e.rot.y = s->readFloat();
            e.rot.z = s->readFloat();
            e.id = s->readUInt16();
            e.pad = s->readUInt16();
            out.mspt.push_back(e);
        }
    }

    else if (id == "STGI") {
        STGI e;
        e.lap = s->readUInt8();
        e.pole = s->readUInt8();
        e.dist = s->readUInt8();
        e.flare = s->readUInt8();
        e.unk1 = s->readUInt8();
        s->readBytesTo(e.flareColor, 4);
        e.unk2 = s->readUInt8();
        e.speedMod = s->readFloat();
        out.stgi = e;
    }
}

return out;
}
void kmp::saveKMP(const KMP& kmp, bStream::CStream* s)
{
    s->writeBytes((uint8_t*)"RKMD", 4);

    s->writeUInt32(0);

    s->writeUInt16(15);
    s->writeUInt16(0x4C);
    s->writeUInt32(0x000009D8);

    size_t sectionOffsetPos = s->tell();
    for (int i = 0; i < 15; i++)
        s->writeUInt32(0);

    uint32_t sectionOffsets[15];

    auto writeSectionHeader = [&](const char* id, uint16_t count, uint16_t headerData) {
        s->writeBytes((uint8_t*)id, 4);
        s->writeUInt16(count);
        s->writeUInt16(headerData);
    };

    sectionOffsets[0] = s->tell() - 0x4C;
    writeSectionHeader("KTPT", kmp.ktpt.size(), 0);
    for (auto& e : kmp.ktpt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeUInt16(e.playerIndex);
        s->writeUInt16(e.padding);
    }

    sectionOffsets[1] = s->tell() - 0x4C;
    writeSectionHeader("ENPT", kmp.enpt.size(), 0);
    for (auto& e : kmp.enpt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.deviation);
        s->writeUInt16(e.s1);
        s->writeUInt8(e.s2);
        s->writeUInt8(e.s3);
    }

    sectionOffsets[2] = s->tell() - 0x4C;
    writeSectionHeader("ENPH", kmp.enph.size(), 0);
    for (auto& e : kmp.enph) {
        s->writeUInt8(e.start);
        s->writeUInt8(e.num);
        s->writeBytes(const_cast<uint8_t*>(e.prev), 6);
        s->writeBytes(const_cast<uint8_t*>(e.next), 6);
        s->writeUInt16(e.unk);
    }

    sectionOffsets[3] = s->tell() - 0x4C;
    writeSectionHeader("ITPT", kmp.itpt.size(), 0);
    for (auto& e : kmp.itpt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.deviation);
        s->writeUInt16(e.s1);
        s->writeUInt16(e.s2);
    }

    sectionOffsets[4] = s->tell() - 0x4C;
    writeSectionHeader("ITPH", kmp.itph.size(), 0);
    for (auto& e : kmp.itph) {
        s->writeUInt8(e.start);
        s->writeUInt8(e.num);
        s->writeBytes(const_cast<uint8_t*>(e.prev), 6);
        s->writeBytes(const_cast<uint8_t*>(e.next), 6);
        s->writeUInt16(e.pad);
    }

    sectionOffsets[5] = s->tell() - 0x4C;
    writeSectionHeader("CKPT", kmp.ckpt.size(), 0);
    for (auto& e : kmp.ckpt) {
        s->writeFloat(e.x1);
        s->writeFloat(e.z1);
        s->writeFloat(e.x2);
        s->writeFloat(e.z2);
        s->writeUInt8(e.respawn);
        s->writeUInt8(e.type);
        s->writeUInt8(e.prev);
        s->writeUInt8(e.next);
    }

    sectionOffsets[6] = s->tell() - 0x4C;
    writeSectionHeader("CKPH", kmp.ckph.size(), 0);
    for (auto& e : kmp.ckph) {
        s->writeUInt8(e.start);
        s->writeUInt8(e.num);
        s->writeBytes(const_cast<uint8_t*>(e.prev), 6);
        s->writeBytes(const_cast<uint8_t*>(e.next), 6);
        s->writeUInt16(e.pad);
    }

    sectionOffsets[7] = s->tell() - 0x4C;
    writeSectionHeader("GOBJ", kmp.gobj.size(), 0);
    for (auto& g : kmp.gobj) {
        s->writeUInt16(g.id);
        s->writeUInt16(g.route);
        s->writeFloat(g.pos.x);
        s->writeFloat(g.pos.y);
        s->writeFloat(g.pos.z);
        s->writeFloat(g.rot.x);
        s->writeFloat(g.rot.y);
        s->writeFloat(g.rot.z);
        s->writeFloat(g.scale.x);
        s->writeFloat(g.scale.y);
        s->writeFloat(g.scale.z);
        for (int a = 0; a < 8; a++)
            s->writeUInt16(g.args[a]);
        s->writeUInt32(g.presence);
    }

    sectionOffsets[8] = s->tell() - 0x4C;

    uint16_t potiHeaderData = 0;
    for (auto& p : kmp.poti)
        potiHeaderData += p.points.size();

    writeSectionHeader("POTI", kmp.poti.size(), potiHeaderData);
    for (auto& p : kmp.poti) {
        s->writeUInt16(p.num);
        s->writeUInt8(p.s1);
        s->writeUInt8(p.s2);
        for (auto& pt : p.points) {
            s->writeFloat(pt.pos.x);
            s->writeFloat(pt.pos.y);
            s->writeFloat(pt.pos.z);
            s->writeUInt16(pt.s1);
            s->writeUInt16(pt.s2);
        }
    }

    sectionOffsets[9] = s->tell() - 0x4C;
    writeSectionHeader("AREA", kmp.area.size(), 0);
    for (auto& e : kmp.area) {
        s->writeUInt8(e.shape);
        s->writeUInt8(e.type);
        s->writeUInt8(e.cam);
        s->writeUInt8(e.priority);
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeFloat(e.scale.x);
        s->writeFloat(e.scale.y);
        s->writeFloat(e.scale.z);
        s->writeUInt16(e.s1);
        s->writeUInt16(e.s2);
        s->writeUInt8(e.route);
        s->writeUInt8(e.enemy);
        s->writeUInt16(e.pad);
    }
    sectionOffsets[10] = s->tell() - 0x4C;
    uint16_t cameraHeaderData = (kmp.openingCamera << 8) | kmp.previewCamera;
    writeSectionHeader("CAME", kmp.came.size(), cameraHeaderData);
    for (auto& e : kmp.came) {
        s->writeUInt8(e.type);
        s->writeUInt8(e.nextCam);
        s->writeUInt8(e.shake);
        s->writeUInt8(e.route);
        s->writeUInt16(e.vCam);
        s->writeUInt16(e.vZoom);
        s->writeUInt16(e.vView);
        s->writeUInt8(e.start);
        s->writeUInt8(e.movie);
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeFloat(e.zoomStart);
        s->writeFloat(e.zoomEnd);
        s->writeFloat(e.viewStart.x);
        s->writeFloat(e.viewStart.y);
        s->writeFloat(e.viewStart.z);
        s->writeFloat(e.viewEnd.x);
        s->writeFloat(e.viewEnd.y);
        s->writeFloat(e.viewEnd.z);
        s->writeFloat(e.time);
    }

    sectionOffsets[11] = s->tell() - 0x4C;
    writeSectionHeader("JGPT", kmp.jgpt.size(), 0);
    for (auto& e : kmp.jgpt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeUInt16(e.id);
        s->writeUInt16(e.sound);
    }

    sectionOffsets[12] = s->tell() - 0x4C;
    writeSectionHeader("CNPT", kmp.cnpt.size(), 0);
    for (auto& e : kmp.cnpt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeUInt16(e.id);
        s->writeUInt16(e.effect);
    }

    sectionOffsets[13] = s->tell() - 0x4C;
    writeSectionHeader("MSPT", kmp.mspt.size(), 0);
    for (auto& e : kmp.mspt) {
        s->writeFloat(e.pos.x);
        s->writeFloat(e.pos.y);
        s->writeFloat(e.pos.z);
        s->writeFloat(e.rot.x);
        s->writeFloat(e.rot.y);
        s->writeFloat(e.rot.z);
        s->writeUInt16(e.id);
        s->writeUInt16(e.pad);
    }

    sectionOffsets[14] = s->tell() - 0x4C;
    writeSectionHeader("STGI", 1, 0);
    {
        const auto& e = kmp.stgi;
        s->writeUInt8(e.lap);
        s->writeUInt8(e.pole);
        s->writeUInt8(e.dist);
        s->writeUInt8(e.flare);
        s->writeUInt8(e.unk1);
        s->writeBytes(const_cast<uint8_t*>(e.flareColor), 4);
        s->writeUInt8(e.unk2);
        s->writeFloat(e.speedMod);
    }

    uint32_t fileLenFinal = s->tell();
    s->seek(4);
    s->writeUInt32(fileLenFinal);

    s->seek(sectionOffsetPos);
    for (int i = 0; i < 15; i++)
        s->writeUInt32(sectionOffsets[i]);
}
uint32_t kmp::CalculateKMPSize(const KMP& kmp)
{
    uint32_t size = 0x4C;

    auto add = [&](uint32_t count, uint32_t entrySize) {
        if (count == 0) return 0u;
        return 8u + count * entrySize;
    };

    size += add(kmp.ktpt.size(), 0x1C);
    size += add(kmp.enpt.size(), 0x14);
    size += add(kmp.enph.size(), 0x10);
    size += add(kmp.itpt.size(), 0x14);
    size += add(kmp.itph.size(), 0x10);
    size += add(kmp.ckpt.size(), 0x14);
    size += add(kmp.ckph.size(), 0x10);
    size += add(kmp.gobj.size(), 0x3C);
    for (auto& p : kmp.poti)
        size += 8 + 4 + p.points.size() * 0x10;
    size += add(kmp.area.size(), 0x34);
    size += add(kmp.came.size(), 0x40);
    size += add(kmp.jgpt.size(), 0x1C);
    size += add(kmp.cnpt.size(), 0x1C);
    size += add(kmp.mspt.size(), 0x1C);
    size += add(1, 0x14);

    return size;
}