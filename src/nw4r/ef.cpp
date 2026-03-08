#include "nw4r/ef.h"
static Color FromRGBA8(uint32_t rgba) {
    return {
        ((rgba >> 24) & 0xFF) / 255.0f,
        ((rgba >> 16) & 0xFF) / 255.0f,
        ((rgba >> 8) & 0xFF) / 255.0f,
        ((rgba >> 0) & 0xFF) / 255.0f
    };
}
reff::MaterialSettings reff::parseMaterialSettings(bStream::CStream* stream, size_t p) {
    MaterialSettings ms{};
    GXMaterialBuilder mb;

    stream->seek(p + 0x00);
    uint16_t flags = stream->readUInt16();

    uint8_t alphaCmp0 = stream->readUInt8();
    uint8_t alphaCmp1 = stream->readUInt8();
    uint8_t alphaOp = stream->readUInt8();

    mb.setAlphaCompare((EGXCompareType)alphaCmp0, 0, (EGXAlphaOp)alphaOp, (EGXCompareType)alphaCmp1, 0);

    for (int i = 0; i < 3; i++) {
        bool useTexture = ((flags >> (4 + i)) & 0x01) != 0;
        if (!useTexture)
            continue;

        bool useTexProj = ((flags >> (7 + i)) & 0x01) != 0;
        if (useTexProj) {
            mb.setTexCoordGen((EGXTexCoordSlot)i,
                EGXTexGenType::Matrix3x4, EGXTexGenSrc::Position, EGXTexMatrix(30 + i * 3),
                false, EGXPostTexGenMatrix(64 + i * 3));
        }
        else {
            mb.setTexCoordGen((EGXTexCoordSlot)i,
                EGXTexGenType::Matrix2x4, EGXTexGenSrc::Tex0, EGXTexMatrix(30 + i * 3));
        }
    }

    uint8_t tevStageCount = stream->readUInt8();
    stream->readUInt8();
    uint8_t indStageFlags = stream->readUInt8();

    for (int i = 0; i < tevStageCount; i++) {
        stream->seek(p + 0x08 + i);
        uint8_t whichTexture = stream->readUInt8();
        if (whichTexture < 2)
            mb.setTevOrder(i, (EGXTexCoordSlot)(0 + whichTexture), (EGXTexMapSlot)(0 + whichTexture), EGXRasColorChannelSlot::COLOR0A0);
        else
            mb.setTevOrder(i, EGXTexCoordSlot::Null, EGXTexMapSlot::Null, EGXRasColorChannelSlot::COLOR_ZERO);

        // colorIn
        stream->seek(p + 0x08 + 0x04 + i * 0x04);
        uint8_t colorInA = stream->readUInt8();
        uint8_t colorInB = stream->readUInt8();
        uint8_t colorInC = stream->readUInt8();
        uint8_t colorInD = stream->readUInt8();
        mb.setTevColorIn(i, (EGXCombineColorInput)colorInA, (EGXCombineColorInput)colorInB, (EGXCombineColorInput)colorInC, (EGXCombineColorInput)colorInD);

        // colorOp
        stream->seek(p + 0x08 + 0x14 + i * 0x05);
        uint8_t colorOp = stream->readUInt8();
        uint8_t colorBias = stream->readUInt8();
        uint8_t colorScale = stream->readUInt8();
        bool colorClamp = stream->readUInt8() != 0;
        uint8_t colorRegId = stream->readUInt8();
        mb.setTevColorOp(i, (EGXTevOp)colorOp, (EGXTevBias)colorBias, (EGXTevScale)colorScale, colorClamp, (EGXTevRegister)colorRegId);

        // alphaIn
        stream->seek(p + 0x08 + 0x28 + i * 0x04);
        uint8_t alphaInA = stream->readUInt8();
        uint8_t alphaInB = stream->readUInt8();
        uint8_t alphaInC = stream->readUInt8();
        uint8_t alphaInD = stream->readUInt8();
        mb.setTevAlphaIn(i, (EGXCombineAlphaInput)alphaInA, (EGXCombineAlphaInput)alphaInB, (EGXCombineAlphaInput)alphaInC, (EGXCombineAlphaInput)alphaInD);

        // alphaOp
        stream->seek(p + 0x08 + 0x38 + i * 0x05);
        uint8_t alphaOp2 = stream->readUInt8();
        uint8_t alphaBias = stream->readUInt8();
        uint8_t alphaScale = stream->readUInt8();
        bool alphaClamp = stream->readUInt8() != 0;
        uint8_t alphaRegId = stream->readUInt8();
        mb.setTevAlphaOp(i, (EGXTevOp)alphaOp2, (EGXTevBias)alphaBias, (EGXTevScale)alphaScale, alphaClamp, (EGXTevRegister)alphaRegId);

        // KColor / KAlpha sel
        stream->seek(p + 0x08 + 0x4C + i);
        uint8_t colorSel = stream->readUInt8();
        mb.setTevKColorSel(i, (EGXKonstColorSel)colorSel);

        stream->seek(p + 0x08 + 0x50 + i);
        uint8_t alphaSel = stream->readUInt8();
        mb.setTevKAlphaSel(i, (EGXKonstAlphaSel)alphaSel);
    }

    // Blend
    stream->seek(p + 0x5C);
    uint8_t blendMode = stream->readUInt8();
    uint8_t blendSrc = stream->readUInt8();
    uint8_t blendDst = stream->readUInt8();
    uint8_t blendOp = stream->readUInt8();
    mb.setBlendMode((EGXBlendMode)blendMode, (EGXBlendModeControl)blendSrc, (EGXBlendModeControl)blendDst, (EGXLogicOp)blendOp);

    // Depth
    uint8_t colorRas = stream->readUInt8(); // 0x60
    uint8_t colorTev0 = stream->readUInt8();
    uint8_t colorTev1 = stream->readUInt8();
    uint8_t colorTev2 = stream->readUInt8();
    uint8_t colorKTev0 = stream->readUInt8();
    uint8_t colorKTev1 = stream->readUInt8();
    uint8_t colorKTev2 = stream->readUInt8();
    uint8_t colorKTev3 = stream->readUInt8();

    uint8_t alphaRas = stream->readUInt8();
    uint8_t alphaTev0 = stream->readUInt8();
    uint8_t alphaTev1 = stream->readUInt8();
    uint8_t alphaTev2 = stream->readUInt8();
    uint8_t alphaKTev0 = stream->readUInt8();
    uint8_t alphaKTev1 = stream->readUInt8();
    uint8_t alphaKTev2 = stream->readUInt8();
    uint8_t alphaKTev3 = stream->readUInt8();

    bool depthTest = (flags & 0x01) != 0;
    bool depthWrite = (flags & 0x02) != 0;
    uint8_t depthFunc = stream->readUInt8(); // 0x70
    mb.setZMode(depthTest, (EGXCompareType)depthFunc, depthWrite);

    uint8_t alphaFlickType = stream->readUInt8(); // 0x71
    uint16_t alphaFlickPhase = stream->readUInt16(); // 0x72
    uint8_t alphaFlickRndm = stream->readUInt8();  // 0x74
    uint8_t alphaFlickStrength = stream->readUInt8();  // 0x75

    // Light
    stream->seek(p + 0x78);
    uint8_t lightMode = stream->readUInt8();
    uint8_t lightType = stream->readUInt8();
    stream->seek(p + 0x7C);
    Color lightAmb = FromRGBA8(stream->readUInt32());
    Color lightDif = FromRGBA8(stream->readUInt32());
    float lightRadius = stream->readFloat();
    float lightPosX = stream->readFloat();
    float lightPosY = stream->readFloat();
    float lightPosZ = stream->readFloat();

    ms.light.mode = lightMode;
    ms.light.type = static_cast<MaterialLightType>(lightType);
    ms.light.amb = lightAmb;
    ms.light.dif = lightDif;
    ms.light.radius = lightRadius;
    ms.light.pos = { lightPosX, lightPosY, lightPosZ };

    // Indirect tex matrix
    float m00 = stream->readFloat(); // 0x94
    float m01 = stream->readFloat();
    float m02 = stream->readFloat();
    float m10 = stream->readFloat();
    float m11 = stream->readFloat();
    float m12 = stream->readFloat();
    float scalePow = (float)std::pow(2.0f, (float)stream->readInt8()); // 0xAC

    glm::mat4 ind(1.0f);
    ind[0][0] = m00 * scalePow;
    ind[0][1] = m01 * scalePow;
    ind[0][2] = m02 * scalePow;
    ind[0][3] = scalePow;

    ind[1][0] = m10 * scalePow;
    ind[1][1] = m11 * scalePow;
    ind[1][2] = m12 * scalePow;
    ind[1][3] = 0.0f;

    ms.indTexMtx = ind;

    int8_t pivotX = stream->readInt8();
    int8_t pivotY = stream->readInt8(); 
    ms.pivotX = pivotX;
    ms.pivotY = pivotY;

    stream->seek(p + 0xB0);
    ms.shapeType = static_cast<ShapeType>(stream->readUInt8());
    ms.shapeOption = stream->readUInt8();
    ms.shapeDir = stream->readUInt8();
    ms.shapeAxis = static_cast<RotType>(stream->readUInt8());
    ms.shapeFlags = stream->readUInt32();
    float shapeZOffset = stream->readFloat();

    ms.material = mb.finish("effect");
    return ms;
}
reff::EfEffectResourceData reff::parseEffectResource(
    bStream::CStream* stream,
    size_t effectDataOffs,
    size_t effectDataSize,
    const std::string& name)
{
    EfEffectResourceData out;
    out.name = name;

    size_t offs = effectDataOffs + 0x04;

    // --- EmitterSettings ---
    stream->seek(offs);
    uint32_t emitterSettingsSize = stream->readUInt32();
    offs += 0x04;

    out.emitterSettings = parseEmitterSettings(stream, offs);
    offs += 0x94;

    // --- MaterialSettings ---
    out.materialSettings = parseMaterialSettings(stream, offs);
    offs += 0xBC;

    // TS: offs -= 0x04
    offs -= 0x04;

    // --- ParticleSettings ---
    stream->seek(offs);
    uint32_t particleSettingsSize = stream->readUInt32();
    offs += 0x04;

    out.particleSettings = parseParticleSettings(stream, offs);
    offs += particleSettingsSize;

    // --- Particle Track Table ---
    stream->seek(offs);
    uint16_t particleTrackTableCount = stream->readUInt16();
    offs += 0x04; // skip 4 bytes

    size_t particleTrackSizeTableOffs = offs;
    offs += particleTrackTableCount * 4;

    // --- Emitter Track Table ---
    stream->seek(offs);
    uint16_t emitterTrackTableCount = stream->readUInt16();
    offs += 0x04;

    size_t emitterTrackSizeTableOffs = offs;
    offs += emitterTrackTableCount * 4;

    // --- Particle Track Infos ---
    for (int i = 0; i < particleTrackTableCount; i++) {
        stream->seek(particleTrackSizeTableOffs + i * 4);
        uint32_t trackSize = stream->readUInt32();

        // Read track block into a temporary buffer
        std::vector<uint8_t> trackBuf(trackSize);
        stream->seek(offs);
        stream->readBytesTo(trackBuf.data(), trackSize);

        out.particleTrackInfos.push_back(
            parseTrackInfo(trackBuf.data(), trackSize)
        );

        offs += trackSize;
    }

    // --- Emitter Track Infos ---
    for (int i = 0; i < emitterTrackTableCount; i++) {
        stream->seek(emitterTrackSizeTableOffs + i * 4);
        uint32_t trackSize = stream->readUInt32();

        std::vector<uint8_t> trackBuf(trackSize);
        stream->seek(offs);
        stream->readBytesTo(trackBuf.data(), trackSize);

        out.emitterTrackInfos.push_back(
            parseTrackInfo(trackBuf.data(), trackSize)
        );

        offs += trackSize;
    }

    // TS: assert(offs === buffer.byteLength)
    assert(offs == effectDataOffs + effectDataSize);

    return out;
}
reff::EmitterSettings reff::parseEmitterSettings(bStream::CStream* stream, size_t p) {
    EmitterSettings es{};

    stream->seek(p + 0x00);
    es.emitterSimFlags = stream->readUInt32();

    uint32_t volumeTypeAndEmitFlags = stream->readUInt32();
    es.volumeType = static_cast<VolumeType>(volumeTypeAndEmitFlags & 0xFF);
    es.emitFlags = static_cast<EmitFlags>(volumeTypeAndEmitFlags >> 8);

    es.maxFrame = stream->readUInt16();
    es.lifeTime = stream->readUInt16();
    es.lifeTimeRndm = stream->readInt8() / 100.0f;
    es.particleChildInheritTranslation = stream->readInt8();

    es.rateStepRndm = stream->readInt8() / 100.0f;
    es.rateRndm = stream->readInt8();
    es.rate = stream->readFloat();

    es.startFrame = stream->readUInt16();
    es.prerollTime = stream->readUInt16();
    es.rateStep = stream->readUInt16();
    es.particleInheritTranslation = stream->readInt8();
    es.emitterChildInheritTranslation = stream->readInt8();

    // volumeParams[6]
    for (int i = 0; i < 6; i++)
        es.volumeParams[i] = stream->readFloat();

    es.divNumber = stream->readUInt16();
    es.initialVelRatio = stream->readInt8();
    es.momentRndm = stream->readInt8() / 100.0f;

    es.initialVelOmni = stream->readFloat();
    es.initialVelAxis = stream->readFloat();
    es.initialVelRndm = stream->readFloat();
    es.initialVelNrm = stream->readFloat();
    es.diffuseVelNrm = stream->readFloat();

    es.initialVelDir = stream->readFloat();
    es.diffuseVelDir = stream->readFloat();

    float dirX = stream->readFloat();
    float dirY = stream->readFloat();
    float dirZ = stream->readFloat();
    es.emitterDir = { dirX, dirY, dirZ };

    float sclX = stream->readFloat();
    float sclY = stream->readFloat();
    float sclZ = stream->readFloat();
    es.emitterScl = { sclX, sclY, sclZ };

    float rotX = stream->readFloat();
    float rotY = stream->readFloat();
    float rotZ = stream->readFloat();
    es.emitterRot = { rotX, rotY, rotZ };

    float trsX = stream->readFloat();
    float trsY = stream->readFloat();
    float trsZ = stream->readFloat();
    es.emitterTrs = { trsX, trsY, trsZ };

    es.lodNear = stream->readUInt8();
    es.lodFar = stream->readUInt8();
    es.lodMinEmit = stream->readUInt8();
    es.lodAlpha = stream->readUInt8();

    es.randomSeed = stream->readUInt32();

    return es;
}
reff::ParticleSettings reff::parseParticleSettings(bStream::CStream* stream,size_t p) {
    ParticleSettings ps{};
    stream->seek(p);
    ps.colors[0] = FromRGBA8(stream->readUInt32());
    ps.colors[1] = FromRGBA8(stream->readUInt32());
    ps.colors[2] = FromRGBA8(stream->readUInt32());
    ps.colors[3] = FromRGBA8(stream->readUInt32());

    ps.sizeX = stream->readFloat();
    ps.sizeY = stream->readFloat();
    ps.scaleX = stream->readFloat();
    ps.scaleY = stream->readFloat();

    ps.rotationX = stream->readFloat();
    ps.rotationY = stream->readFloat();
    ps.rotationZ = stream->readFloat();

    ps.texScale0S = stream->readFloat();
    ps.texScale0T = stream->readFloat();
    ps.texScale1S = stream->readFloat();
    ps.texScale1T = stream->readFloat();
    ps.texScale2S = stream->readFloat();
    ps.texScale2T = stream->readFloat();

    ps.texRotation0 = stream->readFloat();
    ps.texRotation1 = stream->readFloat();
    ps.texRotation2 = stream->readFloat();

    ps.texTranslation0S = stream->readFloat();
    ps.texTranslation0T = stream->readFloat();
    ps.texTranslation1S = stream->readFloat();
    ps.texTranslation1T = stream->readFloat();
    ps.texTranslation2S = stream->readFloat();
    ps.texTranslation2T = stream->readFloat();

    stream->seek(p+0x74);
    ps.textureWrapMode = stream->readUInt16();
    ps.textureMirror = stream->readUInt8();

    ps.alphaRef0 = stream->readUInt8();
    ps.alphaRef1 = stream->readUInt8();

    ps.rotationRandom0 = stream->readUInt8();
    ps.rotationRandom1 = stream->readUInt8();
    ps.rotationRandom2 = stream->readUInt8();

    ps.rotation0 = stream->readFloat();
    ps.rotation1 = stream->readFloat();
    ps.rotation2 = stream->readFloat();

    size_t offs = 0x88;
    for (int i = 0; i < 3; i++) {
        stream->seek(p + offs);
        uint16_t len = stream->readUInt16();
        offs += 2;
        ps.textureNames[i] = stream->readString(len);
        offs += len;
    }

    return ps;
}
reff::BREFF reff::parseBREFF(bStream::CStream* stream) {
    stream->readUInt32(); // 'REFF'
    uint16_t endianMarker = stream->readUInt16();
    assert(endianMarker == 0xFEFF || endianMarker == 0xFFFE);
    assert(endianMarker != 0xFFFE); // big endian

    uint16_t fileVersion = stream->readUInt16();
    uint32_t fileLength = stream->readUInt32();
    uint16_t rootSectionOffs = stream->readUInt16();
    uint16_t numSections = stream->readUInt16();

    stream->seek(rootSectionOffs + 0x08);
    uint32_t effectTableOffs =
        rootSectionOffs + 0x08 + stream->readUInt32();

    stream->seek(rootSectionOffs + 0x14);
    uint16_t nameLen = stream->readUInt16();
    stream->seek(rootSectionOffs + 0x18);
    std::string name = stream->readString(nameLen);

    stream->seek(effectTableOffs + 0x04);
    uint16_t effectTableCount = stream->readUInt16();

    BREFF result;
    result.name = name;

    size_t effectTableIdx = effectTableOffs + 0x08;

    for (int i = 0; i < effectTableCount; i++) {
        stream->seek(effectTableIdx);
        uint16_t effectNameLen = stream->readUInt16();
        effectTableIdx += 2;

        stream->seek(effectTableIdx);
        std::string effectName = stream->readString(effectNameLen);
        effectTableIdx += effectNameLen;

        stream->seek(effectTableIdx);
        uint32_t rel = stream->readUInt32();
        uint32_t effectDataOffs = effectTableOffs + rel;
        uint32_t effectDataSize = stream->readUInt32();
        effectTableIdx += 8;

        EfEffectResourceData eff =
            parseEffectResource(stream, effectDataOffs, effectDataSize, effectName);

        result.effects.push_back(std::move(eff));
    }

    return result;
}
reff::BREFT reff::parseBREFT(bStream::CStream* stream) {
    stream->seek(0x04);
    uint16_t endianMarker = stream->readUInt16();
    assert(endianMarker == 0xFEFF || endianMarker == 0xFFFE);
    assert(endianMarker != 0xFFFE); // big endian

    uint16_t fileVersion = stream->readUInt16();
    uint32_t fileLength = stream->readUInt32();
    uint16_t rootSectionOffs = stream->readUInt16();
    uint16_t numSections = stream->readUInt16();

    stream->seek(rootSectionOffs + 0x08);
    uint32_t textureTableRel = stream->readUInt32();
    uint32_t textureTableOffs = rootSectionOffs + 0x08 + textureTableRel;

    stream->seek(rootSectionOffs + 0x14);
    uint16_t nameLen = stream->readUInt16();
    stream->seek(rootSectionOffs + 0x18);
    std::string name = stream->readString(nameLen);

    stream->seek(textureTableOffs + 0x04);
    uint16_t textureCount = stream->readUInt16();

    BREFT result;
    result.name = name;

    size_t idx = textureTableOffs + 0x08;

    for (int i = 0; i < textureCount; i++) {
        stream->seek(idx);
        uint16_t texNameLen = stream->readUInt16();
        idx += 2;

        stream->seek(idx);
        std::string texName = stream->readString(texNameLen);
        idx += texNameLen;

        stream->seek(idx);
        uint32_t rel = stream->readUInt32();
        uint32_t texDataOffs = textureTableOffs + rel;
        uint32_t texDataSize = stream->readUInt32();
        idx += 8;

        stream->seek(texDataOffs + 0x04);
        uint16_t width = stream->readUInt16();
        uint16_t height = stream->readUInt16();

        stream->seek(texDataOffs + 0x08);
        uint32_t dataSize = stream->readUInt32();

        stream->seek(texDataOffs + 0x0C);
        uint8_t format = stream->readUInt8();
        uint8_t paletteFormat = stream->readUInt8();
        uint16_t paletteEntries = stream->readUInt16();
        uint32_t paletteSize = stream->readUInt32();

        uint8_t mipmap = stream->readUInt8();
        int mipCount = mipmap ? 999 : 1;

        stream->seek(texDataOffs + 0x20);
        std::vector<uint8_t> texData(dataSize);
        stream->readBytesTo(texData.data(), dataSize);

        std::vector<uint8_t> palData;
        if (paletteSize != 0) {
            palData.resize(paletteSize);
            stream->readBytesTo(palData.data(), paletteSize);
        }

        BREFTTexture tex;
        tex.name = texName;
        tex.format = format;
        tex.width = width;
        tex.height = height;
        tex.data = std::move(texData);
        tex.mipCount = mipCount;
        tex.paletteFormat = paletteFormat;
        tex.paletteData = std::move(palData);

        result.textures.push_back(std::move(tex));
    }

    return result;
}