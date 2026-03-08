#include "nw4r/brlyt.h"
lyt::RFNT lyt::parseBRFNT(bStream::CStream* stream, const std::string& name) {
    RFNTFINF finf{};
    RFNTTGLP tglp{};
    RFNTCWDHEntry defaultCWDH{};
    bool hasFINF = false, hasTGLP = false;

    RFNT rfnt{};
    rfnt.name = name;
    rfnt.cmap.fill(0xFFFF);

    stream->seek(0x00);
    std::string magic = stream->readString(4);

    uint16_t endianMarker = stream->readUInt16();
    if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(endianMarker);
    assert(endianMarker == 0xFEFF || endianMarker == 0xFFFE);

    uint16_t fileVersion = stream->readUInt16();
    if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(fileVersion);
    assert(fileVersion == 0x0104);

    uint32_t fileLength = stream->readUInt32();
    uint16_t rootSectionOffs = stream->readUInt16();
    uint16_t numSections = stream->readUInt16();

    if (stream->getOrder() == bStream::Endianess::Little)
    {
        bStream::swap32(fileLength);
        bStream::swap16(rootSectionOffs);
        bStream::swap16(numSections);
    }

    size_t tableIdx = rootSectionOffs;
    rfnt.glyphInfo.clear();

    for (int s = 0; s < numSections; s++) {
        stream->seek(tableIdx);
        std::string fourcc = stream->readString(4);
        uint32_t blockSize = stream->readUInt32();
        size_t blockContentsOffs = tableIdx + 0x08;
        if (stream->getOrder() == bStream::Endianess::Little) bStream::swap32(blockSize);
        if (fourcc == "FINF" || fourcc == "FNIF") {
            stream->seek(blockContentsOffs + 0x00);
            uint8_t fontType = stream->readUInt8();
            assert(fontType == static_cast<uint8_t>(RFNTGlyphType::Texture));

            uint8_t advanceHeight = stream->readUInt8();
            uint16_t defaultGlyphIndex = stream->readUInt16();
            int8_t defaultLeftSideBearing = stream->readInt8();
            uint8_t defaultWidth = stream->readUInt8();
            int8_t defaultAdvanceWidth = stream->readInt8();

            defaultCWDH = {
                defaultLeftSideBearing,
                defaultWidth,
                defaultAdvanceWidth,
            };

            uint8_t encoding = stream->readUInt8();
            assert(encoding == static_cast<uint8_t>(RFNTEncoding::UTF16));

            stream->seek(blockContentsOffs + 0x14);
            uint8_t height = stream->readUInt8();
            uint8_t width = stream->readUInt8();
            uint8_t ascent = stream->readUInt8();

            finf.advanceHeight = advanceHeight;
            finf.encoding = RFNTEncoding::UTF16;
            finf.width = width;
            finf.height = height;
            finf.ascent = ascent;
            if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(defaultGlyphIndex);
            finf.defaultGlyphIndex = defaultGlyphIndex;
            hasFINF = true;

        }
        else if (fourcc == "TGLP" || fourcc == "PLGT") {
            stream->seek(blockContentsOffs + 0x00);
            uint8_t glyphCellW = stream->readUInt8();
            uint8_t glyphCellH = stream->readUInt8();
            uint8_t glyphBaseline = stream->readUInt8();
            uint8_t glyphW2 = stream->readUInt8();
            uint32_t texDataSize = stream->readUInt32();
            uint16_t texCount = stream->readUInt16();
            uint16_t texFormatU16 = stream->readUInt16();
            uint16_t textureGlyphNumX = stream->readUInt16();
            uint16_t textureGlyphNumY = stream->readUInt16();
            uint16_t texW = stream->readUInt16();
            uint16_t texH = stream->readUInt16();
            uint32_t texDataOffs = stream->readUInt32();
            if (stream->getOrder() == bStream::Endianess::Little)
            {
                bStream::swap32(texDataSize);
                bStream::swap16(texCount);
                bStream::swap16(texFormatU16);
                bStream::swap16(textureGlyphNumX);
                bStream::swap16(textureGlyphNumY);
                bStream::swap16(texW);
                bStream::swap16(texH);
                bStream::swap32(texDataOffs);
            }
            tglp.glyphBaseline = glyphBaseline;
            tglp.textures.clear();

            rfnt.glyphInfo.clear();
            rfnt.glyphInfo.resize(textureGlyphNumX * textureGlyphNumY * texCount);

            size_t texDataIdx = texDataOffs;
            uint32_t glyphIndex = 0;

            for (int ti = 0; ti < texCount; ti++, texDataIdx += texDataSize) {
                RFNTTGLPTexture tex{};
                tex.name = name + " Texture " + std::to_string(ti);
                tex.width = texW;
                tex.height = texH;
                tex.format = static_cast<uint8_t>(texFormatU16);
                tex.mipCount = 1;

                tex.data.resize(texDataSize);
                stream->seek(texDataIdx);
                stream->readBytesTo(tex.data.data(), texDataSize);

                tglp.textures.push_back(std::move(tex));

                for (int y = 0; y < textureGlyphNumY; y++) {
                    float t0 = (float)(y * (glyphCellH + 1)) / (float)texH;
                    float t1 = (float)((y * (glyphCellH + 1)) + glyphCellH) / (float)texH;
                    for (int x = 0; x < textureGlyphNumX; x++) {
                        float s0 = (float)(x * (glyphCellW + 1)) / (float)texW;
                        float s1b = (float)(x * (glyphCellW + 1));
                        float s1m = 1.0f / (float)texW;

                        GlyphInfo gi{};
                        gi.textureIndex = ti;
                        gi.s0 = s0;
                        gi.t0 = t0;
                        gi.s1b = s1b;
                        gi.s1m = s1m;
                        gi.t1 = t1;
                        gi.cwdh = defaultCWDH;

                        rfnt.glyphInfo[glyphIndex++] = gi;
                    }
                }
            }

            hasTGLP = true;

        }
        else if (fourcc == "CWDH" || fourcc == "HDWC") {
            stream->seek(blockContentsOffs + 0x00);
            uint16_t glyphStart = stream->readUInt16();
            uint16_t glyphEnd = stream->readUInt16();
            // uint32_t cwdhNextOffs = stream->readUInt32();
            if (stream->getOrder() == bStream::Endianess::Little)
            {
                bStream::swap16(glyphStart);
                bStream::swap16(glyphEnd);
            }
            size_t table = blockContentsOffs + 0x08;
            for (uint16_t gi = glyphStart; gi <= glyphEnd; gi++, table += 0x03) {
                stream->seek(table);
                int8_t leftSideBearing = stream->readInt8();
                uint8_t width = stream->readUInt8();
                int8_t advanceWidth = stream->readInt8();
                rfnt.glyphInfo[gi].cwdh = { leftSideBearing, width, advanceWidth };
            }

        }
        else if (fourcc == "CMAP" || fourcc == "PAMC") {
            stream->seek(blockContentsOffs + 0x00);
            uint16_t codeStart = stream->readUInt16();
            uint16_t codeEnd = stream->readUInt16();
            uint16_t kind = stream->readUInt16();
            if (stream->getOrder() == bStream::Endianess::Little)
            {
                bStream::swap16(codeStart);
                bStream::swap16(codeEnd);
                bStream::swap16(kind);
            }
            if ((RFNTCMAPKind)kind == RFNTCMAPKind::Offset) {
                stream->seek(blockContentsOffs + 0x0C);
                uint16_t offset = stream->readUInt16();
                if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(offset);
                for (uint16_t c = codeStart; c <= codeEnd; c++)
                    rfnt.cmap[c] = c - codeStart + offset;

            }
            else if ((RFNTCMAPKind)kind == RFNTCMAPKind::Array) {
                size_t table = blockContentsOffs + 0x0C;
                for (uint16_t c = codeStart; c <= codeEnd; c++, table += 0x02) {
                    stream->seek(table);
                    rfnt.cmap[c] = stream->readUInt16();
                    if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(rfnt.cmap[c]);
                }

            }
            else if ((RFNTCMAPKind)kind == RFNTCMAPKind::Dict) {
                stream->seek(blockContentsOffs + 0x0C);
                uint16_t entryNum = stream->readUInt16();
                if (stream->getOrder() == bStream::Endianess::Little) bStream::swap16(entryNum);
                size_t table = blockContentsOffs + 0x0A;
                for (uint16_t i = 0; i <= entryNum; i++, table += 0x04) {
                    stream->seek(table);
                    uint16_t code = stream->readUInt16();
                    uint16_t glyph = stream->readUInt16();
                    if (stream->getOrder() == bStream::Endianess::Little)
                    {
                        bStream::swap16(code);
                        bStream::swap16(glyph);
                    }
                    rfnt.cmap[code] = glyph;
                }
            }
        }
        else {

        }

        tableIdx += blockSize;
    }

    assert(hasFINF && hasTGLP);

    static_cast<RFNTFINF&>(rfnt) = finf;
    static_cast<RFNTTGLP&>(rfnt) = tglp;

    return rfnt;
}