#pragma once
#include <string>
#include <optional>
#include <vector>
#include <array>
#include "GXEnum.hpp"
#include <glm/glm.hpp>
#include "../geometry/GXGeometryEnums.hpp"
#include <functional>
#include "glad/glad.h"
#include "bstream.h"
inline void hashU8(uint32_t& h, uint8_t v) {
    h ^= v;
    h *= 16777619u;
}

inline void hashU32(uint32_t& h, uint32_t v) {
    hashU8(h, (v >> 0) & 0xFF);
    hashU8(h, (v >> 8) & 0xFF);
    hashU8(h, (v >> 16) & 0xFF);
    hashU8(h, (v >> 24) & 0xFF);
}

template<typename E>
inline void hashEnum(uint32_t& h, E e) {
    hashU32(h, static_cast<uint32_t>(e));
}
struct ColorChannelControl {
    bool lightingEnabled;
    EGXColorSource matColorSource;
    EGXColorSource ambColorSource;
    uint32_t litMask;
    EGXDiffuseFunction diffuseFunction;
    EGXAttenuationFunction attenuationFunction;
};
struct LightChannelControl {
    ColorChannelControl alphaChannel;
    ColorChannelControl colorChannel;
};
struct TexGen {
    EGXTexGenType type;
    EGXTexGenSrc source;
    EGXTexMatrix matrix;
    bool normalize;
    EGXPostTexGenMatrix postMatrix;
    uint8_t texCoordId;
};
struct IndTexStage {
    EGXTexCoordSlot texCoordId;
    EGXTexMapSlot texture;
    EGXIndirectTexScale scaleS;
    EGXIndirectTexScale scaleT;
};
using SwapTable = std::array<EGXSwapMode, 4>;
static const SwapTable TevDefaultSwapTables[4] = {
    { EGXSwapMode::R, EGXSwapMode::G, EGXSwapMode::B, EGXSwapMode::A },
    { EGXSwapMode::R, EGXSwapMode::R, EGXSwapMode::R, EGXSwapMode::A },
    { EGXSwapMode::G, EGXSwapMode::G, EGXSwapMode::G, EGXSwapMode::A },
    { EGXSwapMode::B, EGXSwapMode::B, EGXSwapMode::B, EGXSwapMode::A },
};
struct TevStage {
    EGXCombineColorInput colorInA;
    EGXCombineColorInput colorInB;
    EGXCombineColorInput colorInC;
    EGXCombineColorInput colorInD;
    EGXTevOp colorOp;
    EGXTevBias colorBias;
    EGXTevScale colorScale;
    bool colorClamp;
    EGXTevRegister colorRegId;

    EGXCombineAlphaInput alphaInA;
    EGXCombineAlphaInput alphaInB;
    EGXCombineAlphaInput alphaInC;
    EGXCombineAlphaInput alphaInD;
    EGXTevOp alphaOp;
    EGXTevBias alphaBias;
    EGXTevScale alphaScale;
    bool alphaClamp;
    EGXTevRegister alphaRegId;

    EGXTexCoordSlot texCoordId;
    EGXTexMapSlot texMap;
    EGXRasColorChannelSlot channelId;

    EGXKonstColorSel konstColorSel;
    EGXKonstAlphaSel konstAlphaSel;

    std::optional<SwapTable> rasSwapTable;
    std::optional<SwapTable> texSwapTable;

    EGXIndirectTexStageID indTexStage;
    EGXIndirectTexFormat indTexFormat;
    EGXIndirectTexBias indTexBiasSel;
    EGXIndirectAlphaSel indTexAlphaSel;
    EGXIndirectTexMatrixId indTexMatrix;
    EGXIndirectWrapMode indTexWrapS;
    EGXIndirectWrapMode indTexWrapT;
    bool indTexAddPrev;
    bool indTexUseOrigLOD;
};
struct AlphaTest {
    EGXAlphaOp op;
    EGXCompareType compareA;
    uint8_t referenceA;
    EGXCompareType compareB;
    uint8_t referenceB;
};
struct RopInfo {
    EGXFogType fogType;
    bool fogAdjEnabled;

    bool depthTest;
    EGXCompareType depthFunc;
    bool depthWrite;

    EGXBlendMode blendMode;
    EGXBlendModeControl blendSrcFactor;
    EGXBlendModeControl blendDstFactor;
    EGXLogicOp blendLogicOp;

    std::optional<uint8_t> dstAlpha;

    bool colorUpdate;
    bool alphaUpdate;
};
struct GXMaterial {
    std::string name;

    EGXCullMode cullMode;

    std::vector<LightChannelControl> lightChannels;
    std::vector<TexGen> texGens;

    std::vector<TevStage> tevStages;

    std::vector<IndTexStage> indTexStages;

    AlphaTest alphaTest;
    RopInfo ropInfo;

    std::vector<glm::mat4> texMatrices;
    std::vector<glm::mat4> postTexMatrices;

    bool usePnMtxIdx = false;
    std::vector<bool> useTexMtxIdx;
    bool hasPostTexMtxBlock = false;
    bool hasLightsBlock = false;
    bool hasFogBlock = false;
    bool hasDynamicAlphaTest = false;

    void* userData = nullptr;
};
struct DisplayListRegisters {
    uint32_t bp[0x100]{};
    uint32_t cp[0x100]{};

    uint32_t xf[0x1000]{};

    uint32_t tevKColor[16]{};

    DisplayListRegisters() {
        bp[0xFE] = 0x00FFFFFF;
    }

    bool bpRegIsSet(uint32_t regAddr) const {
        return (bp[regAddr] & (1u << 31)) != 0;
    }

    uint32_t bpGet(uint32_t regAddr) const {
        return bp[regAddr];
    }

    void bpSet(uint32_t regBag) {
        uint32_t regAddr = regBag >> 24;

        uint32_t regWMask = bp[0xFE];

        uint32_t regValue = (bp[regAddr] & ~regWMask) | (regBag & regWMask);

        if (regAddr != 0xFE)
            bp[0xFE] = 0x00FFFFFF;

        bp[regAddr] = (1u << 31) | regValue;

        if (regAddr >= 0xE0 &&
            regAddr <= 0xE0 + 4 * 2)
        {
            uint32_t kci = regAddr - 0xE0;
            uint32_t bank = (regValue >> 23) & 0x01;
            tevKColor[bank * 8 + kci] = regValue;
        }
    }

    void xfSet(uint32_t idx, uint32_t sub, uint32_t v) {
        idx -= 0x1000;
        xf[idx * 0x10 + sub] = v;
    }

    uint32_t xfGet(uint32_t idx, uint32_t sub = 0) const {
        idx -= 0x1000;
        return xf[idx * 0x10 + sub];
    }
};
enum class TexMatrixMode {
    Basic = -1,
    Maya = 0,
    XSI = 1,
    Max = 2,
};
struct Color {
    float r;
    float g;
    float b;
    float a;
};
inline Color colorNewFromRGBA(float r, float g, float b, float a) {
    return Color{ r, g, b, a };
}
class GXMaterialBuilder {
public:
    GXMaterialBuilder(const std::string& name = "")
        : name(name)
    {
        reset();
    }

    void reset() {
        cullMode = EGXCullMode::None;
        lightChannels.clear();
        texGens.clear();
        tevStages.clear();
        indTexStages.clear();

        alphaTest = {};
        setAlphaCompare(EGXCompareType::Always, 0,
            EGXAlphaOp::And,
            EGXCompareType::Always, 0);

        ropInfo = {};
        setFog(EGXFogType::None, false);
        setBlendMode(EGXBlendMode::None,
            EGXBlendModeControl::SrcAlpha,
            EGXBlendModeControl::InverseSrcAlpha,
            EGXLogicOp::Clear);
        setZMode(true, EGXCompareType::LEqual, true);
        setColorUpdate(true);
        setAlphaUpdate(false);

        usePnMtxIdx = true;
        hasDynamicAlphaTest = false;
    }

    void setCullMode(EGXCullMode mode) {
        cullMode = mode;
    }

    TexGen& ensureTexCoordGen(size_t idx) {
        if (idx >= texGens.size())
            texGens.resize(idx + 1);
        return texGens[idx];
    }

    void setTexCoordGen(EGXTexCoordSlot idx,
        EGXTexGenType type,
        EGXTexGenSrc source,
        EGXTexMatrix matrix,
        bool normalize = false,
        EGXPostTexGenMatrix postMatrix = EGXPostTexGenMatrix::PTIdentity)
    {
        TexGen& tg = ensureTexCoordGen(static_cast<size_t>(idx));
        tg.type = type;
        tg.source = source;
        tg.matrix = matrix;
        tg.normalize = normalize;
        tg.postMatrix = postMatrix;
        tg.texCoordId = static_cast<uint8_t>(idx);
    }

    LightChannelControl& ensureLightChannel(size_t idx) {
        if (idx >= lightChannels.size()) {
            lightChannels.resize(idx + 1);

            auto& lc = lightChannels[idx];
            lc.colorChannel = {};
            lc.alphaChannel = {};

            setChanCtrlInternal(lc.colorChannel,
                false, EGXColorSource::Register, EGXColorSource::Register,
                0, EGXDiffuseFunction::None, EGXAttenuationFunction::None);

            setChanCtrlInternal(lc.alphaChannel,
                false, EGXColorSource::Register, EGXColorSource::Register,
                0, EGXDiffuseFunction::None, EGXAttenuationFunction::None);
        }
        return lightChannels[idx];
    }

    void setChanCtrlInternal(ColorChannelControl& chanCtrl,
        bool enable,
        EGXColorSource ambSrc,
        EGXColorSource matSrc,
        uint32_t lightMask,
        EGXDiffuseFunction diffFn,
        EGXAttenuationFunction attnFn)
    {
        chanCtrl.lightingEnabled = enable;
        chanCtrl.ambColorSource = ambSrc;
        chanCtrl.matColorSource = matSrc;
        chanCtrl.litMask = lightMask;
        chanCtrl.diffuseFunction = diffFn;
        chanCtrl.attenuationFunction = attnFn;
    }

    void setChanCtrl(EGXColorChannelId idx,
        bool enable,
        EGXColorSource ambSrc,
        EGXColorSource matSrc,
        uint32_t lightMask,
        EGXDiffuseFunction diffFn,
        EGXAttenuationFunction attnFn)
    {
        LightChannelControl& lc = ensureLightChannel(static_cast<int32_t>(idx) & 1);

        int32_t set = (static_cast<int32_t>(idx) >> 1) + 1;

        if (set & 0x01)
            setChanCtrlInternal(lc.colorChannel, enable, ambSrc, matSrc, lightMask, diffFn, attnFn);

        if (set & 0x02)
            setChanCtrlInternal(lc.alphaChannel, enable, ambSrc, matSrc, lightMask, diffFn, attnFn);
    }
    TevStage& ensureTevStage(int idx) {
        if (idx >= tevStages.size()) {
            tevStages.resize(idx + 1);

            TevStage& s = tevStages[idx];
            s.texCoordId = EGXTexCoordSlot::Null;
            s.texMap = EGXTexMapSlot::Null;
            s.channelId = EGXRasColorChannelSlot::COLOR_ZERO;
        }


        TevStage& s = tevStages[idx];
        return s;
    }

    void setTevOrder(int idx, EGXTexCoordSlot texCoordID,
        EGXTexMapSlot texMap, EGXRasColorChannelSlot channelId) {
        TevStage& s = ensureTevStage(idx);
        s.texCoordId = texCoordID;
        s.texMap = texMap;
        s.channelId = channelId;
    }

    void setTevColorOp(int idx, EGXTevOp colorOp,
        EGXTevBias colorBias, EGXTevScale colorScale,
        bool colorClamp, EGXTevRegister colorRegId) {
        TevStage& s = ensureTevStage(idx);
        s.colorOp = colorOp;
        s.colorBias = colorBias;
        s.colorScale = colorScale;
        s.colorClamp = colorClamp;
        s.colorRegId = colorRegId;
    }

    void setTevAlphaOp(int idx, EGXTevOp alphaOp,
        EGXTevBias alphaBias, EGXTevScale alphaScale,
        bool alphaClamp, EGXTevRegister alphaRegId) {
        TevStage& s = ensureTevStage(idx);
        s.alphaOp = alphaOp;
        s.alphaBias = alphaBias;
        s.alphaScale = alphaScale;
        s.alphaClamp = alphaClamp;
        s.alphaRegId = alphaRegId;
    }

    void setTevColorIn(int idx, EGXCombineColorInput a, EGXCombineColorInput b, EGXCombineColorInput c, EGXCombineColorInput d) {
        TevStage& s = ensureTevStage(idx);
        s.colorInA = a;
        s.colorInB = b;
        s.colorInC = c;
        s.colorInD = d;
    }

    void setTevAlphaIn(int idx, EGXCombineAlphaInput a, EGXCombineAlphaInput b, EGXCombineAlphaInput c, EGXCombineAlphaInput d) {
        TevStage& s = ensureTevStage(idx);
        s.alphaInA = a;
        s.alphaInB = b;
        s.alphaInC = c;
        s.alphaInD = d;
    }

    void setTevKColorSel(int idx, EGXKonstColorSel sel) {
        TevStage& s = ensureTevStage(idx);
        s.konstColorSel = sel;
    }

    void setTevKAlphaSel(int idx, EGXKonstAlphaSel sel) {
        TevStage& s = ensureTevStage(idx);
        s.konstAlphaSel = sel;
    }

    void setTevSwapMode(int idx,
        const SwapTable& rasSwapTable,
        const SwapTable& texSwapTable) {
        TevStage& s = ensureTevStage(idx);
        s.rasSwapTable = rasSwapTable;
        s.texSwapTable = texSwapTable;
    }

    void setTevIndirect(int tevStageIdx,
        EGXIndirectTexStageID indTexStage,
        EGXIndirectTexFormat format,
        EGXIndirectTexBias biasSel,
        EGXIndirectTexMatrixId matrixSel,
        EGXIndirectWrapMode wrapS,
        EGXIndirectWrapMode wrapT,
        bool addPrev,
        bool utcLod,
        EGXIndirectAlphaSel alphaSel) {
        TevStage& s = ensureTevStage(tevStageIdx);
        s.indTexStage = indTexStage;
        s.indTexFormat = format;
        s.indTexBiasSel = biasSel;
        s.indTexAlphaSel = alphaSel;
        s.indTexMatrix = matrixSel;
        s.indTexWrapS = wrapS;
        s.indTexWrapT = wrapT;
        s.indTexAddPrev = addPrev;
        s.indTexUseOrigLOD = utcLod;
    }

    void setTevIndWarp(int tevStageIdx,
        EGXIndirectTexStageID indTexStage,
        bool signedOffsets,
        bool replaceMode,
        EGXIndirectTexMatrixId matrixSel) {
        EGXIndirectTexBias biasSel = signedOffsets
            ? EGXIndirectTexBias::IndBias_ST
            : EGXIndirectTexBias::IndBias_None;
        EGXIndirectWrapMode wrap = replaceMode
            ? EGXIndirectWrapMode::IndWrapMode_0
            : EGXIndirectWrapMode::IndWrapMode_Off;

        setTevIndirect(tevStageIdx, indTexStage,
            EGXIndirectTexFormat::IndFormat_8,
            biasSel, matrixSel,
            wrap, wrap,
            false, false,
            EGXIndirectAlphaSel::IndAlphaSel_Off);
    }

    void setTevDirect(int tevStageIdx) {
        setTevIndirect(tevStageIdx,
            EGXIndirectTexStageID::STAGE0,
            EGXIndirectTexFormat::IndFormat_8,
            EGXIndirectTexBias::IndBias_None,
            EGXIndirectTexMatrixId::IndTexMtx_Off,
            EGXIndirectWrapMode::IndWrapMode_Off,
            EGXIndirectWrapMode::IndWrapMode_Off,
            false, false,
            EGXIndirectAlphaSel::IndAlphaSel_Off);
    }

    IndTexStage& ensureIndTexStage(EGXIndirectTexStageID idx) {
        auto i = static_cast<size_t>(idx);
        if (i >= indTexStages.size())
            indTexStages.resize(i + 1);

        IndTexStage& s = indTexStages[i];
        return s;
    }

    void setIndTexOrder(EGXIndirectTexStageID idx,
        EGXTexCoordSlot texcoord,
        EGXTexMapSlot texmap) {
        IndTexStage& s = ensureIndTexStage(idx);
        s.texCoordId = texcoord;
        s.texture = texmap;
    }

    void setIndTexScale(EGXIndirectTexStageID idx,
        EGXIndirectTexScale scaleS,
        EGXIndirectTexScale scaleT) {
        IndTexStage& s = ensureIndTexStage(idx);
        s.scaleS = scaleS;
        s.scaleT = scaleT;
    }

    void setAlphaCompare(EGXCompareType compareA, uint8_t referenceA,
        EGXAlphaOp op,
        EGXCompareType compareB, uint8_t referenceB)
    {
        alphaTest.compareA = compareA;
        alphaTest.referenceA = referenceA;
        alphaTest.op = op;
        alphaTest.compareB = compareB;
        alphaTest.referenceB = referenceB;
    }

    void setFog(EGXFogType fogType, bool fogAdjEnabled) {
        ropInfo.fogType = fogType;
        ropInfo.fogAdjEnabled = fogAdjEnabled;
    }

    void setBlendMode(EGXBlendMode blendMode,
        EGXBlendModeControl srcFactor,
        EGXBlendModeControl dstFactor,
        EGXLogicOp logicOp = EGXLogicOp::Clear)
    {
        ropInfo.blendMode = blendMode;
        ropInfo.blendSrcFactor = srcFactor;
        ropInfo.blendDstFactor = dstFactor;
        ropInfo.blendLogicOp = logicOp;
    }

    void setZMode(bool depthTest, EGXCompareType depthFunc, bool depthWrite) {
        ropInfo.depthTest = depthTest;
        ropInfo.depthFunc = depthFunc;
        ropInfo.depthWrite = depthWrite;
    }

    void setDstAlpha(std::optional<uint8_t> v) {
        ropInfo.dstAlpha = v;
    }

    void setColorUpdate(bool v) {
        ropInfo.colorUpdate = v;
    }

    void setAlphaUpdate(bool v) {
        ropInfo.alphaUpdate = v;
    }

    void setUsePnMtxIdx(bool v) {
        usePnMtxIdx = v;
    }

    void setDynamicAlphaTest(bool v) {
        hasDynamicAlphaTest = v;
    }

    void setTexGenFromRegisters(const DisplayListRegisters& r, int i) {
        uint32_t v = r.xfGet(0x1040 + i);

        enum class TexProjection : uint32_t {
            ST = 0x00,
            STQ = 0x01,
        };
        enum class TexForm : uint32_t {
            AB11 = 0x00,
            ABC1 = 0x01,
        };
        enum class TexGenTypeLocal : uint32_t {
            REGULAR = 0x00,
            EMBOSS_MAP = 0x01,
            COLOR_STRGBC0 = 0x02,
            COLOR_STRGBC1 = 0x02,
        };
        enum class TexSourceRow : uint32_t {
            GEOM = 0x00,
            NRM = 0x01,
            CLR = 0x02,
            BNT = 0x03,
            BNB = 0x04,
            TEX0 = 0x05,
            TEX1 = 0x06,
            TEX2 = 0x07,
            TEX3 = 0x08,
            TEX4 = 0x09,
            TEX5 = 0x0A,
            TEX6 = 0x0B,
            TEX7 = 0x0C,
        };

        TexProjection proj = static_cast<TexProjection>((v >> 1) & 0x01);
        TexForm      form = static_cast<TexForm>((v >> 2) & 0x01);
        TexGenTypeLocal tgTy = static_cast<TexGenTypeLocal>((v >> 4) & 0x02);
        TexSourceRow  src = static_cast<TexSourceRow>((v >> 7) & 0x0F);
        uint32_t embossSrc = (v >> 12) & 0x07;
        uint32_t embossLgt = (v >> 15) & 0x07;

        EGXTexGenType texGenType;
        EGXTexGenSrc  texGenSrc;

        if (tgTy == TexGenTypeLocal::REGULAR) {
            static const EGXTexGenSrc srcLookup[] = {
                EGXTexGenSrc::Position,
                EGXTexGenSrc::Normal,
                EGXTexGenSrc::Color0,
                EGXTexGenSrc::Binormal,
                EGXTexGenSrc::Tangent,
                EGXTexGenSrc::Tex0,
                EGXTexGenSrc::Tex1,
                EGXTexGenSrc::Tex2,
                EGXTexGenSrc::Tex3,
                EGXTexGenSrc::Tex4,
                EGXTexGenSrc::Tex5,
                EGXTexGenSrc::Tex6,
                EGXTexGenSrc::Tex7,
            };

            texGenType = (proj == TexProjection::ST)
                ? EGXTexGenType::Matrix2x4
                : EGXTexGenType::Matrix3x4;

            texGenSrc = srcLookup[static_cast<uint32_t>(src)];
        }
        else if (tgTy == TexGenTypeLocal::EMBOSS_MAP) {
            texGenType = static_cast<EGXTexGenType>(
                static_cast<uint32_t>(EGXTexGenType::Bump0) + embossLgt);
            texGenSrc = static_cast<EGXTexGenSrc>(
                static_cast<uint32_t>(EGXTexGenSrc::TexCoord0) + embossSrc);
        }
        else {
            texGenType = EGXTexGenType::SRTG;
            texGenSrc = (tgTy == TexGenTypeLocal::COLOR_STRGBC0)
                ? EGXTexGenSrc::Color0
                : EGXTexGenSrc::Color1;
        }

        EGXTexMatrix matrix = EGXTexMatrix::Identity; // TODO: XF_MATRIXINDEX0_ID

        uint32_t dv = r.xfGet(0x1050 + i);
        EGXPostTexGenMatrix postMatrix =
            static_cast<EGXPostTexGenMatrix>((dv & 0xFF) + static_cast<uint32_t>(EGXPostTexGenMatrix::PTTexMtx0));
        bool normalize = ((dv >> 8) & 0x01) != 0;

        setTexCoordGen(static_cast<EGXTexCoordSlot>(i), texGenType, texGenSrc, matrix, normalize, postMatrix);
    }

    void setColorChannelFromRegisters(const DisplayListRegisters& r, int i) {
        uint32_t colorCntrl = r.xfGet(0x100E + i);
        uint32_t alphaCntrl = r.xfGet(0x1010 + i);

        auto setChanCtrlFromReg = [&](ColorChannelControl& dst, uint32_t chanCtrl) {
            EGXColorSource matColorSource =
                static_cast<EGXColorSource>((chanCtrl >> 0) & 0x01);
            bool lightingEnabled = ((chanCtrl >> 1) & 0x01) != 0;
            uint32_t litMaskL = (chanCtrl >> 2) & 0x0F;
            EGXColorSource ambColorSource =
                static_cast<EGXColorSource>((chanCtrl >> 6) & 0x01);
            EGXDiffuseFunction diffuseFunction =
                static_cast<EGXDiffuseFunction>((chanCtrl >> 7) & 0x03);
            bool attnEn = ((chanCtrl >> 9) & 0x01) != 0;
            bool attnSelect = ((chanCtrl >> 10) & 0x01) != 0;
            uint32_t litMaskH = (chanCtrl >> 11) & 0x0F;

            uint32_t litMask = (litMaskH << 4) | litMaskL;

            EGXAttenuationFunction attenuationFunction;
            if (!attnEn)
                attenuationFunction = EGXAttenuationFunction::None;
            else
                attenuationFunction = attnSelect
                ? EGXAttenuationFunction::Spot
                : EGXAttenuationFunction::Spec;

            setChanCtrlInternal(dst, lightingEnabled,
                ambColorSource, matColorSource,
                litMask, diffuseFunction, attenuationFunction);
        };

        LightChannelControl& dstChannel = ensureLightChannel(i);
        setChanCtrlFromReg(dstChannel.colorChannel, colorCntrl);
        setChanCtrlFromReg(dstChannel.alphaChannel, alphaCntrl);
    }
    void setTevStageFromRegisters(const DisplayListRegisters& r, int i) {

        uint32_t tref = r.bp[0x28 + (i >> 1)];

        uint32_t shift = (i & 1) ? 12 : 0;
        EGXTexMapSlot texMap = static_cast<EGXTexMapSlot>((tref >> shift) & 0x07);
        EGXTexCoordSlot texCoord = static_cast<EGXTexCoordSlot>((tref >> (shift + 3)) & 0x07);
        bool enableTex = ((tref >> (shift + 6)) & 0x01) != 0;
        EGXRasColorChannelSlot rasChan = static_cast<EGXRasColorChannelSlot>((tref >> (shift + 7)) & 0x07);

        setTevOrder(i, texCoord, enableTex ? texMap : EGXTexMapSlot::Null, rasChan);

        uint32_t color = r.bp[0xC0 +( i * 2)];
        uint32_t alpha = r.bp[0xC1 + (i * 2)];
        uint32_t ksel = r.bp[0xF6 + (i >> 1)];
        uint32_t indCmd = r.bp[0x10 + i];


        EGXCombineColorInput colorInD = static_cast<EGXCombineColorInput>((color >> 0) & 0x0F);
        EGXCombineColorInput colorInC = static_cast<EGXCombineColorInput>((color >> 4) & 0x0F);
        EGXCombineColorInput colorInB = static_cast<EGXCombineColorInput>((color >> 8) & 0x0F);
        EGXCombineColorInput colorInA = static_cast<EGXCombineColorInput>((color >> 12) & 0x0F);

        EGXTevBias colorBias = static_cast<EGXTevBias>((color >> 16) & 0x03);
        bool colorSub = ((color >> 18) & 0x01) != 0;
        bool colorClamp = ((color >> 19) & 0x01) != 0;
        EGXTevScale colorScale = static_cast<EGXTevScale>((color >> 20) & 0x03);
        EGXTevRegister colorRegId = static_cast<EGXTevRegister>((color >> 22) & 0x03);

        EGXTevOp colorOp = findTevOp(colorBias, colorScale, colorSub);

        uint32_t rswap = (alpha >> 0) & 0x03;
        uint32_t tswap = (alpha >> 2) & 0x03;

        EGXCombineAlphaInput alphaInD = static_cast<EGXCombineAlphaInput>((alpha >> 4) & 0x07);
        EGXCombineAlphaInput alphaInC = static_cast<EGXCombineAlphaInput>((alpha >> 7) & 0x07);
        EGXCombineAlphaInput alphaInB = static_cast<EGXCombineAlphaInput>((alpha >> 10) & 0x07);
        EGXCombineAlphaInput alphaInA = static_cast<EGXCombineAlphaInput>((alpha >> 13) & 0x07);
        EGXTevBias alphaBias = static_cast<EGXTevBias>((alpha >> 16) & 0x03);
        bool alphaSub = ((alpha >> 18) & 0x01) != 0;
        bool alphaClamp = ((alpha >> 19) & 0x01) != 0;
        EGXTevScale alphaScale = static_cast<EGXTevScale>((alpha >> 20) & 0x03);
        EGXTevRegister alphaRegId = static_cast<EGXTevRegister>((alpha >> 22) & 0x03);

        EGXTevOp alphaOp = findTevOp(alphaBias, alphaScale, alphaSub);

        setTevColorIn(i, colorInA, colorInB, colorInC, colorInD);
        setTevColorOp(i, colorOp, colorBias, colorScale, colorClamp, colorRegId);

        setTevAlphaIn(i, alphaInA, alphaInB, alphaInC, alphaInD);
        setTevAlphaOp(i, alphaOp, alphaBias, alphaScale, alphaClamp, alphaRegId);

        EGXKonstColorSel konstColorSel = static_cast<EGXKonstColorSel>(
            ((i & 1) ? (ksel >> 14) : (ksel >> 4)) & 0x1F);
        EGXKonstAlphaSel konstAlphaSel = static_cast<EGXKonstAlphaSel>(
            ((i & 1) ? (ksel >> 19) : (ksel >> 9)) & 0x1F);

        setTevKColorSel(i, konstColorSel);
        setTevKAlphaSel(i, konstAlphaSel);

        EGXIndirectTexStageID indTexStage = static_cast<EGXIndirectTexStageID>((indCmd >> 0) & 0x03);
        EGXIndirectTexFormat indTexFormat = static_cast<EGXIndirectTexFormat>((indCmd >> 2) & 0x03);
        EGXIndirectTexBias indTexBiasSel = static_cast<EGXIndirectTexBias>((indCmd >> 4) & 0x07);
        EGXIndirectAlphaSel indTexAlphaSel = static_cast<EGXIndirectAlphaSel>((indCmd >> 7) & 0x03);
        EGXIndirectTexMatrixId indTexMatrix = static_cast<EGXIndirectTexMatrixId>((indCmd >> 9) & 0x0F);
        EGXIndirectWrapMode indTexWrapS = static_cast<EGXIndirectWrapMode>((indCmd >> 13) & 0x07);
        EGXIndirectWrapMode indTexWrapT = static_cast<EGXIndirectWrapMode>((indCmd >> 16) & 0x07);
        bool indTexUseOrigLOD = ((indCmd >> 19) & 0x01) != 0;
        bool indTexAddPrev = ((indCmd >> 20) & 0x01) != 0;

        setTevIndirect(i, indTexStage, indTexFormat, indTexBiasSel,
            indTexMatrix, indTexWrapS, indTexWrapT,
            indTexAddPrev, indTexUseOrigLOD, indTexAlphaSel);

        uint32_t rasRG = r.bp[0xF6 + (rswap * 2)];
        uint32_t rasBA = r.bp[0xF6 + (rswap * 2) + 1];

        SwapTable rasSwapTable = {
            static_cast<EGXSwapMode>((rasRG >> 0) & 0x03),
            static_cast<EGXSwapMode>((rasRG >> 2) & 0x03),
            static_cast<EGXSwapMode>((rasBA >> 0) & 0x03),
            static_cast<EGXSwapMode>((rasBA >> 2) & 0x03),
        };

        uint32_t texRG = r.bp[0xF6 + (tswap * 2)];
        uint32_t texBA = r.bp[0xF6 + (tswap * 2) + 1];

        SwapTable texSwapTable = {
            static_cast<EGXSwapMode>((texRG >> 0) & 0x03),
            static_cast<EGXSwapMode>((texRG >> 2) & 0x03),
            static_cast<EGXSwapMode>((texBA >> 0) & 0x03),
            static_cast<EGXSwapMode>((texBA >> 2) & 0x03),
        };

        setTevSwapMode(i, rasSwapTable, texSwapTable);
    }

    EGXTevOp findTevOp(EGXTevBias bias, EGXTevScale scale, bool sub) {
        if (bias == EGXTevBias::Compare) {
            switch (scale) {
            case EGXTevScale::Scale_1:
                return sub ? EGXTevOp::Comp_R8_EQ : EGXTevOp::Comp_R8_GT;
            case EGXTevScale::Scale_2:
                return sub ? EGXTevOp::Comp_GR16_EQ : EGXTevOp::Comp_GR16_GT;
            case EGXTevScale::Scale_4:
                return sub ? EGXTevOp::Comp_BGR24_EQ : EGXTevOp::Comp_BGR24_GT;
            case EGXTevScale::Divide_2:
                return sub ? EGXTevOp::Comp_RGB8_EQ : EGXTevOp::Comp_RGB8_GT;
            default:
                return EGXTevOp::Comp_RGB8_GT;
            }
        }
        return sub ? EGXTevOp::Sub : EGXTevOp::Add;
    }
    void setIndTexStageFromRegisters(const DisplayListRegisters& r, int i) {
        uint32_t iref = r.bp[0x27];
        uint32_t ss = r.bp[0x25 + (i >> 2)];

        EGXIndirectTexScale scaleS = static_cast<EGXIndirectTexScale>(
            (ss >> ((0x08 * (i & 1)) + 0x00)) & 0x0F);
        EGXIndirectTexScale scaleT = static_cast<EGXIndirectTexScale>(
            (ss >> ((0x08 * (i & 1)) + 0x04)) & 0x0F);

        EGXTexMapSlot texture = static_cast<EGXTexMapSlot>((iref >> (0x06 * i)) & 0x07);
        EGXTexCoordSlot texCoordId = static_cast<EGXTexCoordSlot>((iref >> (0x06 * i)) & 0x07);

        setIndTexOrder(static_cast<EGXIndirectTexStageID>(i), texCoordId, texture);
        setIndTexScale(static_cast<EGXIndirectTexStageID>(i), scaleS, scaleT);
    }
    void setRopStateFromRegisters(const DisplayListRegisters& r)
    {
        setFog(EGXFogType::None, false);

        if (r.bpRegIsSet(0x41)) {
            uint32_t cm0 = r.bp[0x41];

            uint32_t bmboe = (cm0 >> 0) & 0x01;
            uint32_t bmloe = (cm0 >> 1) & 0x01;

            bool colorUpdate = ((cm0 >> 3) & 0x01) != 0;
            bool alphaUpdate = ((cm0 >> 4) & 0x01) != 0;

            setColorUpdate(colorUpdate);
            setAlphaUpdate(alphaUpdate);

            uint32_t bmbop = (cm0 >> 11) & 0x01;

            EGXBlendMode blendMode =
                bmboe ? (bmbop ? EGXBlendMode::Subtract : EGXBlendMode::Blend) :
                bmloe ? EGXBlendMode::Logic :
                EGXBlendMode::None;

            EGXBlendModeControl blendDstFactor = static_cast<EGXBlendModeControl>((cm0 >> 5) & 0x07);
            EGXBlendModeControl blendSrcFactor = static_cast<EGXBlendModeControl>((cm0 >> 8) & 0x07);
            EGXLogicOp blendLogicOp = static_cast<EGXLogicOp>((cm0 >> 12) & 0x0F);

            setBlendMode(blendMode, blendSrcFactor, blendDstFactor, blendLogicOp);
        }

        if (r.bpRegIsSet(0x40)) {
            uint32_t zm = r.bp[0x40];

            bool depthTest = ((zm >> 0) & 0x01) != 0;
            EGXCompareType depthFunc = static_cast<EGXCompareType>((zm >> 1) & 0x07);
            bool depthWrite = ((zm >> 4) & 0x01) != 0;

            setZMode(depthTest, depthFunc, depthWrite);
        }

        if (r.bpRegIsSet(0xF3)) {
            uint32_t ap = r.bp[0xF3];

            uint8_t refA = (ap >> 0) & 0xFF;
            uint8_t refB = (ap >> 8) & 0xFF;

            EGXCompareType compareA = static_cast<EGXCompareType>((ap >> 16) & 0x07);
            EGXCompareType compareB = static_cast<EGXCompareType>((ap >> 19) & 0x07);
            EGXAlphaOp op = static_cast<EGXAlphaOp>((ap >> 22) & 0x07);

            setAlphaCompare(compareA, refA, op, compareB, refB);
        }

        if (r.bpRegIsSet(0x00)) {
            uint32_t genMode = r.bp[0x00];

            static const EGXCullMode hw2cm[] = {
                EGXCullMode::None,
                EGXCullMode::Back,
                EGXCullMode::Front,
                EGXCullMode::All,
            };

            EGXCullMode cullMode = hw2cm[(genMode >> 14) & 0x03];
            setCullMode(cullMode);
        }
    }
    void setFromRegisters(const DisplayListRegisters& r) {
        uint32_t genMode = r.bp[0x00];

        int numTexGens = (genMode >> 0) & 0x0F;
        for (int i = 0; i < numTexGens; i++)
            setTexGenFromRegisters(r, i);

        int numTevStages = ((genMode >> 10) & 0x0F) + 1;
        for (int i = 0; i < numTevStages; i++)
            setTevStageFromRegisters(r, i);

        int numInds = (genMode >> 16) & 0x07;
        for (int i = 0; i < numInds; i++)
            setIndTexStageFromRegisters(r, i);

        int numColors = r.xfGet(0x1009);
        for (int i = 0; i < numColors; i++)
            setColorChannelFromRegisters(r, i);

        setRopStateFromRegisters(r);
    }
    GXMaterial finish(const std::string& overrideName) {
        std::string finalName = overrideName.empty() ? (name.empty() ? std::string() : name) : overrideName;

        GXMaterial m;
        m.name = finalName;
        m.cullMode = cullMode;
        m.lightChannels = lightChannels;
        m.texGens = texGens;
        m.tevStages = tevStages;
        m.indTexStages = indTexStages;
        m.alphaTest = alphaTest;
        m.ropInfo = ropInfo;
        m.usePnMtxIdx = usePnMtxIdx;
        m.hasDynamicAlphaTest = hasDynamicAlphaTest;

        return m;
    }
    std::string name;

    EGXCullMode cullMode;
    std::vector<LightChannelControl> lightChannels;
    std::vector<TexGen> texGens;
    std::vector<TevStage> tevStages;
    std::vector<IndTexStage> indTexStages;

    AlphaTest alphaTest;
    RopInfo ropInfo;

    bool usePnMtxIdx;
    bool hasDynamicAlphaTest;
};
inline uint32_t RGB565toRGBA8(uint16_t data) {
    uint8_t r = (data & 0xF100) >> 11;
    uint8_t g = (data & 0x07E0) >> 5;
    uint8_t b = (data & 0x001F);

    uint32_t output = 0x000000FF;
    output |= (r << 3) << 24;
    output |= (g << 2) << 16;
    output |= (b << 3) << 8;

    return output;
}

inline uint32_t RGB5A3toRGBA8(uint16_t data) {
    uint8_t r, g, b, a;

    // No alpha bits to extract.
    if (data & 0x8000) {
        a = 0xFF;

        r = (data & 0x7C00) >> 10;
        g = (data & 0x03E0) >> 5;
        b = (data & 0x001F);

        r = (r << (8 - 5)) | (r >> (10 - 8));
        g = (g << (8 - 5)) | (g >> (10 - 8));
        b = (b << (8 - 5)) | (b >> (10 - 8));
    }
    // Alpha bits present.
    else {
        a = (data & 0x7000) >> 12;
        r = (data & 0x0F00) >> 8;
        g = (data & 0x00F0) >> 4;
        b = (data & 0x000F);

        a = (a << (8 - 3)) | (a << (8 - 6)) | (a >> (9 - 8));
        r = (r << (8 - 4)) | r;
        g = (g << (8 - 4)) | g;
        b = (b << (8 - 4)) | b;
    }

    uint32_t output = a;
    output |= r << 24;
    output |= g << 16;
    output |= b << 8;

    return output;
}
inline uint32_t GXWrapToGLWrap(EGXWrapMode gxWrap) {
    switch (gxWrap) {
    case EGXWrapMode::ClampToEdge:
        return GL_CLAMP_TO_EDGE;
    case EGXWrapMode::MirroredRepeat:
        return GL_MIRRORED_REPEAT;
    case EGXWrapMode::Repeat:
        return GL_REPEAT;
    }
}

inline uint32_t GXFilterToGLFilter(EGXFilterMode gxFilter) {
    switch (gxFilter) {
    case EGXFilterMode::Nearest:
        return GL_NEAREST;
    case EGXFilterMode::Linear:
        return GL_LINEAR;
    case EGXFilterMode::NearestMipmapNearest:
        return GL_NEAREST_MIPMAP_NEAREST;
    case EGXFilterMode::NearestMipmapLinear:
        return GL_NEAREST_MIPMAP_LINEAR;
    case EGXFilterMode::LinearMipmapNearest:
        return GL_LINEAR_MIPMAP_NEAREST;
    case EGXFilterMode::LinearMipmapLinear:
        return GL_LINEAR_MIPMAP_LINEAR;
    }
}

inline float GXAnisoToGLAniso(EGXMaxAnisotropy aniso) {
    switch (aniso) {
    case EGXMaxAnisotropy::One:
        return 1.0f;
    case EGXMaxAnisotropy::Two:
        return 2.0f;
    case EGXMaxAnisotropy::Four:
        return 4.0f;
    }

    return 1.0f;
}
inline void DecodeI4(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 8;
    uint32_t numBlocksH = height / 8;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 8; pixelY++) {
                for (int pixelX = 0; pixelX < 8; pixelX += 2) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 8 + pixelX >= width) || (blockY * 8 + pixelY >= height))
                        continue;

                    uint8_t data = stream->readUInt8();

                    // Each byte represents two pixels.
                    uint8_t pixel0 = (data & 0xF0) >> 4;
                    uint8_t pixel1 = (data & 0x0F);

                    uint32_t destIndex = (width * ((blockY * 8) + pixelY) + (blockX * 8) + pixelX) * 4;

                    imageData[destIndex] = pixel0 * 0x11;
                    imageData[destIndex + 1] = pixel0 * 0x11;
                    imageData[destIndex + 2] = pixel0 * 0x11;
                    imageData[destIndex + 3] = pixel0 * 0x11;

                    imageData[destIndex + 4] = pixel1 * 0x11;
                    imageData[destIndex + 5] = pixel1 * 0x11;
                    imageData[destIndex + 6] = pixel1 * 0x11;
                    imageData[destIndex + 7] = pixel1 * 0x11;
                }
            }
        }
    }
}

inline void DecodeI8(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 8;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 8; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 8 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    uint8_t data = stream->readUInt8();

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 8) + pixelX) * 4;

                    imageData[destIndex] = data;
                    imageData[destIndex + 1] = data;
                    imageData[destIndex + 2] = data;
                    imageData[destIndex + 3] = data;
                }
            }
        }
    }
}

inline void DecodeIA4(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 8;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 8; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 8 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    uint8_t data = stream->readUInt8();

                    // Each byte contains alpha and luminance of the current pixel.
                    uint8_t alpha = (data & 0xF0) >> 4;
                    uint8_t luminance = (data & 0x0F);

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 8) + pixelX) * 4;

                    imageData[destIndex] = luminance * 0x11;
                    imageData[destIndex + 1] = luminance * 0x11;
                    imageData[destIndex + 2] = luminance * 0x11;
                    imageData[destIndex + 3] = alpha * 0x11;
                }
            }
        }
    }
}

inline void DecodeIA8(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 4;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 4; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 4 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    // The alpha and luminance values of the current pixel are stored in two bytes.
                    uint8_t alpha = stream->readUInt8();
                    uint8_t luminance = stream->readUInt8();

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 4) + pixelX) * 4;

                    imageData[destIndex] = luminance;
                    imageData[destIndex + 1] = luminance;
                    imageData[destIndex + 2] = luminance;
                    imageData[destIndex + 3] = alpha;
                }
            }
        }
    }
}

inline void DecodeRGB565(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 4;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 4; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 4 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    // RGB values for this pixel are stored in a 16-bit integer.
                    uint16_t data = stream->readUInt16();
                    uint32_t rgba8 = RGB565toRGBA8(data);

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 4) + pixelX) * 4;

                    imageData[destIndex] = (rgba8 & 0xFF000000) >> 24;
                    imageData[destIndex + 1] = (rgba8 & 0x00FF0000) >> 16;
                    imageData[destIndex + 2] = (rgba8 & 0x0000FF00) >> 8;
                    imageData[destIndex + 3] = rgba8 & 0x000000FF;
                }
            }
        }
    }
}

inline void DecodeRGB5A3(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 4;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 4; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 4 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    // RGB values for this pixel are stored in a 16-bit integer.
                    uint16_t data = stream->readUInt16();
                    uint32_t rgba8 = RGB5A3toRGBA8(data);

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 4) + pixelX) * 4;

                    imageData[destIndex] = (rgba8 & 0xFF000000) >> 24;
                    imageData[destIndex + 1] = (rgba8 & 0x00FF0000) >> 16;
                    imageData[destIndex + 2] = (rgba8 & 0x0000FF00) >> 8;
                    imageData[destIndex + 3] = rgba8 & 0x000000FF;
                }
            }
        }
    }
}

inline void DecodeRGBA32(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = width / 4;
    uint32_t numBlocksH = height / 4;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Iterate the pixels in the current block

            // Alpha/red values for current pixel
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 4; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 4 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 4) + pixelX) * 4;

                    imageData[destIndex + 3] = stream->readUInt8();
                    imageData[destIndex] = stream->readUInt8();
                }
            }

            // Green/blue values for current pixel
            for (int pixelY = 0; pixelY < 4; pixelY++) {
                for (int pixelX = 0; pixelX < 4; pixelX++) {
                    // Bounds check to ensure the pixel is within the image.
                    if ((blockX * 4 + pixelX >= width) || (blockY * 4 + pixelY >= height))
                        continue;

                    uint32_t destIndex = (width * ((blockY * 4) + pixelY) + (blockX * 4) + pixelX) * 4;

                    imageData[destIndex + 1] = stream->readUInt8();
                    imageData[destIndex + 2] = stream->readUInt8();
                }
            }
        }
    }
}
inline uint8_t* DecodeCMPRSubBlock(bStream::CStream* stream) {
    uint8_t* data = new uint8_t[4 * 4 * 4]{};

    uint16_t color0 = stream->readUInt16();
    uint16_t color1 = stream->readUInt16();
    uint32_t bits = stream->readUInt32();

    uint32_t colorTable[4]{};
    colorTable[0] = RGB565toRGBA8(color0);
    colorTable[1] = RGB565toRGBA8(color1);

    uint8_t r0, g0, b0, a0, r1, g1, b1, a1;
    r0 = (colorTable[0] & 0xFF000000) >> 24;
    g0 = (colorTable[0] & 0x00FF0000) >> 16;
    b0 = (colorTable[0] & 0x0000FF00) >> 8;
    a0 = (colorTable[0] & 0x000000FF);

    r1 = (colorTable[1] & 0xFF000000) >> 24;
    g1 = (colorTable[1] & 0x00FF0000) >> 16;
    b1 = (colorTable[1] & 0x0000FF00) >> 8;
    a1 = (colorTable[1] & 0x000000FF);

    if (color0 > color1) {
        colorTable[2] |= ((2 * r0 + r1) / 3) << 24;
        colorTable[2] |= ((2 * g0 + g1) / 3) << 16;
        colorTable[2] |= ((2 * b0 + b1) / 3) << 8;
        colorTable[2] |= 0xFF;

        colorTable[3] |= ((r0 + 2 * r1) / 3) << 24;
        colorTable[3] |= ((g0 + 2 * g1) / 3) << 16;
        colorTable[3] |= ((b0 + 2 * b1) / 3) << 8;
        colorTable[3] |= 0xFF;
    }
    else {
        colorTable[2] |= ((r0 + r1) / 2) << 24;
        colorTable[2] |= ((g0 + g1) / 2) << 16;
        colorTable[2] |= ((b0 + b1) / 2) << 8;
        colorTable[2] |= 0xFF;

        colorTable[3] |= ((r0 + 2 * r1) / 3) << 24;
        colorTable[3] |= ((g0 + 2 * g1) / 3) << 16;
        colorTable[3] |= ((b0 + 2 * b1) / 3) << 8;
        colorTable[3] |= 0x00;
    }

    for (int pixelY = 0; pixelY < 4; pixelY++) {
        for (int pixelX = 0; pixelX < 4; pixelX++) {
            uint32_t i = pixelY * 4 + pixelX;
            uint32_t bitOffset = (15 - i) * 2;
            uint32_t di = i * 4;
            uint32_t si = (bits >> bitOffset) & 3;

            data[di + 0] = (colorTable[si] & 0xFF000000) >> 24;
            data[di + 1] = (colorTable[si] & 0x00FF0000) >> 16;
            data[di + 2] = (colorTable[si] & 0x0000FF00) >> 8;
            data[di + 3] = (colorTable[si] & 0x000000FF);
        }
    }

    return data;
}
inline void DecodeCMPR(bStream::CStream* stream, uint16_t width, uint16_t height, uint8_t* imageData) {
    if (imageData == nullptr)
        return;

    uint32_t numBlocksW = (width + 7) / 8;
    uint32_t numBlocksH = (height + 7) / 8;

    // Iterate the blocks in the image
    for (int blockY = 0; blockY < numBlocksH; blockY++) {
        for (int blockX = 0; blockX < numBlocksW; blockX++) {
            // Each block has a set of 2x2 sub-blocks.
            for (int subBlockY = 0; subBlockY < 2; subBlockY++) {
                for (int subBlockX = 0; subBlockX < 2; subBlockX++) {
                    int w = width - (subBlockX * 4 + blockX * 8);
                    int h = height - (subBlockY * 4 + blockY * 8);

                    uint32_t subBlockWidth = (w < 0 ? 0 : (w > 4 ? 4 : w));
                    uint32_t subBlockHeight = (h < 0 ? 0 : (h > 4 ? 4 : h));

                    uint8_t* subBlockData = DecodeCMPRSubBlock(stream);

                    for (int pixelY = 0; pixelY < subBlockHeight; pixelY++) {
                        uint32_t destX = blockX * 8 + subBlockX * 4;
                        uint32_t destY = blockY * 8 + (subBlockY * 4) + pixelY;

                        if (destX >= width || destY >= height)
                            continue;

                        uint32_t destOffset = (destY * width + destX) * 4;
                        memcpy(imageData + destOffset, subBlockData + (pixelY * 4 * 4), subBlockWidth * 4);
                    }

                    delete[] subBlockData;
                }
            }
        }
    }
}

inline GLenum ConvertGXWrap(uint8_t wrap)
{
    switch (wrap)
    {
    case 0: return GL_CLAMP_TO_EDGE;
    case 1: return GL_REPEAT;
    case 2: return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}
inline GLenum ConvertGXMinFilter(uint8_t f)
{
    switch (f)
    {
    case 0: return GL_NEAREST;
    case 1: return GL_LINEAR;
    case 2: return GL_NEAREST_MIPMAP_NEAREST;
    case 3: return GL_LINEAR_MIPMAP_NEAREST;
    case 4: return GL_NEAREST_MIPMAP_LINEAR;
    case 5: return GL_LINEAR_MIPMAP_LINEAR;
    }
    return GL_LINEAR;
}
inline GLenum ConvertGXMagFilter(uint8_t f)
{
    switch (f)
    {
    case 0: return GL_NEAREST;
    case 1: return GL_LINEAR;
    }
    return GL_LINEAR;
}