#include "nw4r/brres.h"
#include  <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <variant>
#include <array>
#include <glm/gtc/type_ptr.hpp> 
#include <sstream>
#include <iomanip>
std::vector<bres::LightSet> LightSets;
std::vector<std::shared_ptr<bres::ModelInstance>> instance;
    glm::mat4 bres::computeModelMatrixSRT(
    float sx, float sy, float sz,
    float rx, float ry, float rz,
    float tx, float ty, float tz)
{
    glm::mat4 M(1.0f);

    M = glm::scale(M, glm::vec3(sx, sy, sz));

    M = glm::rotate(M, rz, glm::vec3(0, 0, 1));
    M = glm::rotate(M, ry, glm::vec3(0, 1, 0));
    M = glm::rotate(M, rx, glm::vec3(1, 0, 0));

    M = glm::translate(M, glm::vec3(tx, ty, tz));

    return M;
}

    float LerpPeriodic(float k0, float k1, float t, float kp = 180.0f) {
        float ga = std::fmod(k1 - k0, kp);
        float g = std::fmod(2 * ga, kp) - ga;
        return k0 + g * t;
    }
    int bres::MapChannelIdToColorIndex(EGXRasColorChannelSlot chan)
    {
        switch (chan)
        {
        case EGXRasColorChannelSlot::COLOR0A0:
            return 0;

        case EGXRasColorChannelSlot::COLOR1A1:
            return 1;

        case EGXRasColorChannelSlot::ALPHA_BUMP:
        case EGXRasColorChannelSlot::ALPHA_BUMP_N:
        case EGXRasColorChannelSlot::COLOR_ZERO:
            return -1;
        }

        return 0;
    }
std::string bres::GetTevColorInput(EGXCombineColorInput in, int i)
{
    switch (in) {
    case EGXCombineColorInput::ColorPrev: return "prev.rgb";
    case EGXCombineColorInput::AlphaPrev: return "prev.aaa";
    case EGXCombineColorInput::C0: return "tevReg0.rgb";
    case EGXCombineColorInput::A0: return "tevReg0.aaa";
    case EGXCombineColorInput::C1: return "tevReg1.rgb";
    case EGXCombineColorInput::A1: return "tevReg1.aaa";
    case EGXCombineColorInput::C2: return "tevReg2.rgb";
    case EGXCombineColorInput::A2: return "tevReg2.aaa";
    case EGXCombineColorInput::TexColor: return "stageTexColor" + std::to_string(i) + ".rgb";
    case EGXCombineColorInput::TexAlpha: return "stageTexColor" + std::to_string(i) + ".aaa";
    case EGXCombineColorInput::RasColor: return "rasColor.rgb";
    case EGXCombineColorInput::RasAlpha: return "rasColor.aaa";
    case EGXCombineColorInput::One: return "vec3(1.0)";
    case EGXCombineColorInput::Half: return "vec3(128.0/255.0)";
    case EGXCombineColorInput::Konst: return "konstColor";
    case EGXCombineColorInput::Zero: return "vec3(0.0)";
    }
    return "vec3(0.0)";
}
std::string bres::GetTevAlphaInput(EGXCombineAlphaInput in, int i)
{
    switch (in) {
    case EGXCombineAlphaInput::AlphaPrev: return "prev.a";
    case EGXCombineAlphaInput::A0: return "tevReg0.a";
    case EGXCombineAlphaInput::A1: return "tevReg1.a";
    case EGXCombineAlphaInput::A2: return "tevReg2.a";
    case EGXCombineAlphaInput::TexAlpha: return "stageTexColor" + std::to_string(i) + ".a";
    case EGXCombineAlphaInput::RasAlpha: return "rasColor.a";
    case EGXCombineAlphaInput::Konst: return "konstAlpha";
    case EGXCombineAlphaInput::Zero: return "0.0";
    case EGXCombineAlphaInput::One: return "1.0";
    }
    return "0.0";
}
std::string bres::ApplyBias(const std::string& expr, EGXTevBias bias)
{
    switch (bias)
    {
    case EGXTevBias::Zero:
        return expr;

    case EGXTevBias::AddHalf:
        return expr + " + 0.5";

    case EGXTevBias::SubHalf:
        return expr + " - 0.5";
    }

    return expr;
}
std::string bres::ApplyScale(const std::string& expr, EGXTevScale scale)
{
    switch (scale)
    {
    case EGXTevScale::Scale_1:
        return expr;

    case EGXTevScale::Scale_2:
        return expr + " * 2.0";

    case EGXTevScale::Scale_4:
        return expr + " * 4.0";

    case EGXTevScale::Divide_2:
        return expr + " * 0.5";
    }

    return expr;
}
    std::string bres::ApplyClamp(const std::string& expr, bool clamp) {
        if (clamp)
            return "clamp(" + expr + ", 0.0, 1.0)";
        return expr;
    }
        bool IsLerpInput(EGXCombineColorInput c)
{
    switch (c)
    {
    case EGXCombineColorInput::AlphaPrev:
    case EGXCombineColorInput::A0:
    case EGXCombineColorInput::A1:
    case EGXCombineColorInput::A2:
    case EGXCombineColorInput::TexAlpha:
    case EGXCombineColorInput::RasAlpha:
    case EGXCombineColorInput::Konst:
        return true;

    default:
        return false;
    }
}
    bool IsLerpInputAlpha(EGXCombineAlphaInput c)
    {
        switch (c)
        {
        case EGXCombineAlphaInput::AlphaPrev:
        case EGXCombineAlphaInput::A0:
        case EGXCombineAlphaInput::A1:
        case EGXCombineAlphaInput::A2:
        case EGXCombineAlphaInput::TexAlpha:
        case EGXCombineAlphaInput::RasAlpha:
        case EGXCombineAlphaInput::Konst:
            return true;

        default:
            return false;
        }
    }
std::string bres::ApplyTevOpColor(
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    EGXTevOp op,
    EGXTevBias bias,
    EGXTevScale scale,
    bool clamp,EGXCombineColorInput ff)
{
    std::string expr;

    switch (op)
    {
case EGXTevOp::Add:
    expr = "(" + d + " + ((1.0 - " + c + ") * " + a + " + " + c + " * " + b + "))";
    break;

case EGXTevOp::Sub:
    expr = "(" + d + " - ((1.0 - " + c + ") * " + a + " + " + c + " * " + b + "))";
    break;
    case EGXTevOp::Comp_R8_GT:
expr = "((" + a + ".r > " + b + ".r) ? " + c + " : " + "0" + ") + " + d;
        break;

    case EGXTevOp::Comp_R8_EQ:
        expr = "((" + a + ".r == " + b + ".r) ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_GR16_GT:
        expr =
            "((int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) > "
            "(int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_GR16_EQ:
        expr =
            "((int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) == "
            "(int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_BGR24_GT:
        expr =
            "((int(" + a + ".b * 255.0) * 65536 + int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) > "
            "(int(" + b + ".b * 255.0) * 65536 + int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_BGR24_EQ:
        expr =
            "((int(" + a + ".b * 255.0) * 65536 + int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) == "
            "(int(" + b + ".b * 255.0) * 65536 + int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
case EGXTevOp::Comp_RGB8_GT:
    expr =
        "((int(" + a + ".r * 255.0) * 65536 + "
             "int(" + a + ".g * 255.0) * 256 + "
             "int(" + a + ".b * 255.0)) > "
         "(int(" + b + ".r * 255.0) * 65536 + "
             "int(" + b + ".g * 255.0) * 256 + "
             "int(" + b + ".b * 255.0))"
        " ? " + c + " : " + "0" + ") + " + d;
    break;

case EGXTevOp::Comp_RGB8_EQ:
    expr =
        "((int(" + a + ".r * 255.0) * 65536 + "
             "int(" + a + ".g * 255.0) * 256 + "
             "int(" + a + ".b * 255.0)) == "
         "(int(" + b + ".r * 255.0) * 65536 + "
             "int(" + b + ".g * 255.0) * 256 + "
             "int(" + b + ".b * 255.0))"
        " ? " + c + " : " + "0" + ") + " + d;
    break;

    default:
        expr = d;
        break;
    }

    expr = ApplyBias(expr, bias);
    expr = ApplyScale(expr, scale);
    expr = ApplyClamp(expr, clamp);
expr = "TevOverflow(" + expr + ")";

    return expr;
}
int bres::DecodeKonstColorSel(uint8_t sel) {
    switch (sel) {
    case 0x00: return 11; // 1.0
    case 0x01: return 10; // 7/8
    case 0x02: return 9;  // 6/8
    case 0x03: return 8;  // 5/8
    case 0x04: return 7;  // 4/8
    case 0x05: return 6;  // 3/8
    case 0x06: return 5;  // 2/8
    case 0x07: return 4;  // 1/8

    case 0x0C: return 0;  // K0
    case 0x0D: return 1;  // K1
    case 0x0E: return 2;  // K2
    case 0x0F: return 3;  // K3

    case 0x10: return 12; // K0_R
    case 0x11: return 13; // K1_R
    case 0x12: return 14; // K2_R
    case 0x13: return 15; // K3_R

    case 0x14: return 16; // K0_G
    case 0x15: return 17; // K1_G
    case 0x16: return 18; // K2_G
    case 0x17: return 19; // K3_G

    case 0x18: return 20; // K0_B
    case 0x19: return 21; // K1_B
    case 0x1A: return 22; // K2_B
    case 0x1B: return 23; // K3_B

    case 0x1C: return 24; // K0_A
    case 0x1D: return 25; // K1_A
    case 0x1E: return 26; // K2_A
    case 0x1F: return 27; // K3_A
    }
    return 0;
}
std::string bres::ApplyTevOpAlpha(
    const std::string& a,
    const std::string& b,
    const std::string& c,
    const std::string& d,
    EGXTevOp op,
    EGXTevBias bias,
    EGXTevScale scale,
    bool clamp,EGXCombineAlphaInput ff)
{
    std::string expr;

    switch (op)
    {
case EGXTevOp::Add:
        expr = "(" + d + " + ((1.0 - " + c + ") * " + a + " + " + c + " * " + b + "))";
    break;
    case EGXTevOp::Sub:
    expr = "(" + d + " - ((1.0 - " + c + ") * " + a + " + " + c + " * " + b + "))";
        break;
    case EGXTevOp::Comp_R8_GT:
expr = "((" + a + ".r > " + b + ".r) ? " + c + " : " + "0" + ") + " + d;
        break;

    case EGXTevOp::Comp_R8_EQ:
        expr = "((" + a + " == " + b + ") ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_GR16_GT:
        expr =
            "((int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) > "
            "(int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;

    case EGXTevOp::Comp_GR16_EQ:
        expr =
            "((int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) == "
            "(int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
    case EGXTevOp::Comp_BGR24_GT:
        expr =
            "((int(" + a + ".b * 255.0) * 65536 + int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) > "
            "(int(" + b + ".b * 255.0) * 65536 + int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;

    case EGXTevOp::Comp_BGR24_EQ:
        expr =
            "((int(" + a + ".b * 255.0) * 65536 + int(" + a + ".g * 255.0) * 256 + int(" + a + ".r * 255.0)) == "
            "(int(" + b + ".b * 255.0) * 65536 + int(" + b + ".g * 255.0) * 256 + int(" + b + ".r * 255.0))"
            " ? " + c + " : " + "0" + ") + " + d;
        break;
case EGXTevOp::Comp_RGB8_GT:
    expr =
        "((int(" + a + ".r * 255.0) * 65536 + "
             "int(" + a + ".g * 255.0) * 256 + "
             "int(" + a + ".b * 255.0)) > "
         "(int(" + b + ".r * 255.0) * 65536 + "
             "int(" + b + ".g * 255.0) * 256 + "
             "int(" + b + ".b * 255.0))"
        " ? " + c + " : " + "0" + ") + " + d;
    break;

case EGXTevOp::Comp_RGB8_EQ:
    expr =
        "((int(" + a + ".r * 255.0) * 65536 + "
             "int(" + a + ".g * 255.0) * 256 + "
             "int(" + a + ".b * 255.0)) == "
         "(int(" + b + ".r * 255.0) * 65536 + "
             "int(" + b + ".g * 255.0) * 256 + "
             "int(" + b + ".b * 255.0))"
        " ? " + c + " : " + "0" + ") + " + d;
    break;

    default:
        expr = d;
        break;
    }

    expr = ApplyBias(expr, bias);
    expr = ApplyScale(expr, scale);
    expr = ApplyClamp(expr, clamp);
expr = "TevOverflow(" + expr + ")";

    return expr;
}
    inline std::string bres::SwapModeToGLSL(EGXSwapMode m) {
        switch (m) {
        case EGXSwapMode::R: return "r";
        case EGXSwapMode::G: return "g";
        case EGXSwapMode::B: return "b";
        case EGXSwapMode::A: return "a";
        }
        return "r";
    }
    std::string bres::ApplySwapTable(const std::string& src, const SwapTable& table) {
        return "vec4(" +
            src + "." + SwapModeToGLSL(table[0]) + ", " +
            src + "." + SwapModeToGLSL(table[1]) + ", " +
            src + "." + SwapModeToGLSL(table[2]) + ", " +
            src + "." + SwapModeToGLSL(table[3]) + ")";
    }

        std::string bres::ApplySwapTable2(const std::string& src, const SwapTable& table) {
        return "vec3(" +
            src + "." + SwapModeToGLSL(table[0]) + ", " +
            src + "." + SwapModeToGLSL(table[1]) + ", " +
            src + "." + SwapModeToGLSL(table[2]) + ")";
    }
int bres::DecodeKonstAlphaSel(uint8_t raw)
{
    if (raw <= 0x07)
        return raw;
    if (raw >= 0x0C && raw <= 0x0F)
        return 8 + (raw - 0x0C);
    return 0;
}
std::string GetRasAlphaExpr(EGXRasColorChannelSlot chan,
                            const TevStage& s)
{
    switch (chan)
    {
    case EGXRasColorChannelSlot::COLOR0A0:
    case EGXRasColorChannelSlot::COLOR1A1:
        return "rasColor.a";

    case EGXRasColorChannelSlot::ALPHA_BUMP:
        return "bumpValue" + std::to_string((int)s.indTexStage);

    case EGXRasColorChannelSlot::ALPHA_BUMP_N:
        return "(1.0 - bumpValue" + std::to_string((int)s.indTexStage) + ")";

    case EGXRasColorChannelSlot::COLOR_ZERO:
        return "0.0";
    }

    return "0.0";
}
    std::string bres::GenerateTevStageGLSL(const TevStage& s,const GXMaterial& mat,int i)
    {
        
        std::ostringstream glsl;
if (s.texSwapTable)
    glsl << "stageTexColor" << i <<" = " << ApplySwapTable("stageTexColor" + std::to_string(i), *s.texSwapTable) << ";\n";

int colorIndex = MapChannelIdToColorIndex(s.channelId);
glsl << "rasColor = ComputeRasColor(" << colorIndex << ");\n";

glsl << "rasColor.a = " << GetRasAlphaExpr(s.channelId, s) << ";\n";

if (s.rasSwapTable)
    glsl << "rasColor = "
         << ApplySwapTable("rasColor", *s.rasSwapTable)
         << ";\n";

glsl << "konstColor = getKonstColor(" << DecodeKonstColorSel((uint8_t)s.konstColorSel) << ");\n";
glsl << "konstAlpha = getKonstAlpha(" << DecodeKonstAlphaSel((uint8_t)s.konstAlphaSel) << ");\n";

auto col = [&](EGXCombineColorInput in) {
std::string expr = GetTevColorInput(in, i);
    return expr;
};
auto alp = [&](EGXCombineAlphaInput in) {
    return GetTevAlphaInput(in, i);
};

glsl << "t_TevA = vec4(" 
     << col(s.colorInA) << ", " << alp(s.alphaInA) << ");\n";

glsl << "t_TevB = vec4(" 
     << col(s.colorInB) << ", " << alp(s.alphaInB) << ");\n";

glsl << "t_TevC = vec4(" 
     << col(s.colorInC) << ", " << alp(s.alphaInC) << ");\n";

glsl << "t_TevD = vec4(" 
     << col(s.colorInD) << ", " << alp(s.alphaInD) << ");\n";

    std::string regRGB, regA;
    switch (s.colorRegId) {
    case EGXTevRegister::Reg0: regRGB = "tevReg0"; break;
    case EGXTevRegister::Reg1: regRGB = "tevReg1"; break;
    case EGXTevRegister::Reg2: regRGB = "tevReg2"; break;
    case EGXTevRegister::Prev: regRGB = "prev";   break;
    }

    switch (s.alphaRegId) {
    case EGXTevRegister::Reg0: regA = "tevReg0"; break;
    case EGXTevRegister::Reg1: regA = "tevReg1"; break;
    case EGXTevRegister::Reg2: regA = "tevReg2"; break;
    case EGXTevRegister::Prev: regA = "prev";   break;
    }

    glsl << regRGB << ".rgb = "
         << ApplyTevOpColor("t_TevA.rgb", "t_TevB.rgb", "t_TevC.rgb", "t_TevD.rgb",
                            s.colorOp, s.colorBias, s.colorScale, s.colorClamp, s.colorInC)
         << ";\n";

    glsl << regA << ".a = "
         << ApplyTevOpAlpha("t_TevA.a", "t_TevB.a", "t_TevC.a", "t_TevD.a",
                            s.alphaOp, s.alphaBias, s.alphaScale, s.alphaClamp, s.alphaInC)
         << ";\n";

    return glsl.str();
         }
float bres::ConvertIndScale(EGXIndirectTexScale scale) {
    switch (scale) {
        case EGXIndirectTexScale::IndDivide_1:   return 1.0f;
        case EGXIndirectTexScale::IndDivide_2:   return 0.5f;
        case EGXIndirectTexScale::IndDivide_4:   return 0.25f;
        case EGXIndirectTexScale::IndDivide_8:   return 0.125f;
        case EGXIndirectTexScale::IndDivide_16:  return 0.0625f;
        case EGXIndirectTexScale::IndDivide_32:  return 0.03125f;
        case EGXIndirectTexScale::IndDivide_64:  return 0.015625f;
        case EGXIndirectTexScale::IndDivide_128: return 0.0078125f;
        case EGXIndirectTexScale::IndDivide_256: return 0.00390625f;
    }
    return 1.0f;
}
int bres::ConvertIndWrap(EGXIndirectWrapMode w)
{
    switch (w) {
    case EGXIndirectWrapMode::IndWrapMode_256: return 256;
    case EGXIndirectWrapMode::IndWrapMode_128: return 128;
    case EGXIndirectWrapMode::IndWrapMode_64:  return 64;
    case EGXIndirectWrapMode::IndWrapMode_32:  return 32;
    case EGXIndirectWrapMode::IndWrapMode_16:  return 16;
    case EGXIndirectWrapMode::IndWrapMode_8:   return 8;
    case EGXIndirectWrapMode::IndWrapMode_4:   return 4;
    case EGXIndirectWrapMode::IndWrapMode_2:   return 2;
    case EGXIndirectWrapMode::IndWrapMode_0:   return 0; 
    default: return 0;
    }
}

std::string bres::GenerateIndTexStageGLSL(
    const IndTexStage& s, int stageIndex, const TevStage& tev, int tevIndex)
{
    std::ostringstream ss;

    const int indUV = (int)s.texCoordId;
    const int texMap = (int)s.texture;
const int indMtxRaw = (int)tev.indTexMatrix;
const int indMtxIndex = (indMtxRaw & 0x03) - 1;

    const float scaleS = ConvertIndScale(s.scaleS);
    const float scaleT = ConvertIndScale(s.scaleT);

    const int wrapS = ConvertIndWrap(tev.indTexWrapS);
    const int wrapT = ConvertIndWrap(tev.indTexWrapT);

    ss << "// ---- Indirect Texture Stage " << stageIndex << " ----\n";

    ss << "vec2 indCoord" << tevIndex
       << " = ReadTexCoordInd" << indUV << "();\n";

    ss << "vec4 indSample" << tevIndex
       << " = texture(u_Tex" << texMap << ", indCoord" << tevIndex << ");\n";

    ss << "int indTexFormat" << tevIndex << " = " << (int)tev.indTexFormat << ";\n";
    ss << "int indTexBiasSel" << tevIndex << " = " << (int)tev.indTexBiasSel << ";\n";

    ss << "vec3 indVal" << tevIndex << ";\n";
    ss << "if (indTexFormat" << tevIndex << " == 0) indVal" << tevIndex << " = indSample" << tevIndex << ".rgb * 255.0 - 128.0;\n";
    ss << "else if (indTexFormat" << tevIndex << " == 1) indVal" << tevIndex << " = indSample" << tevIndex << ".rgb * 31.0 - 16.0;\n";
    ss << "else if (indTexFormat" << tevIndex << " == 2) indVal" << tevIndex << " = indSample" << tevIndex << ".rgb * 15.0 - 8.0;\n";
    ss << "else if (indTexFormat" << tevIndex << " == 3) indVal" << tevIndex << " = indSample" << tevIndex << ".rgb * 7.0 - 4.0;\n";
if (tev.indTexFormat == EGXIndirectTexFormat::IndFormat_8) {
ss << "vec3 bias" << tevIndex << " = vec3(0.0);\n";
ss << "float A" << tevIndex << " = indSample" << tevIndex << ".a * 255.0 - 128.0;\n";

ss << "if (indTexBiasSel" << tevIndex << " == 1) bias" << tevIndex << " = vec3(indVal" << tevIndex << ".x, 0.0, 0.0);\n";
ss << "if (indTexBiasSel" << tevIndex << " == 2) bias" << tevIndex << " = vec3(0.0, indVal" << tevIndex << ".y, 0.0);\n";
ss << "if (indTexBiasSel" << tevIndex << " == 3) bias" << tevIndex << " = vec3(indVal" << tevIndex << ".x, indVal" << tevIndex << ".y, 0.0);\n";

ss << "if (indTexBiasSel" << tevIndex << " == 4) bias" << tevIndex << " = vec3(A" << tevIndex << ", 0.0, 0.0);\n";
ss << "if (indTexBiasSel" << tevIndex << " == 5) bias" << tevIndex << " = vec3(indVal" << tevIndex << ".x + A" << tevIndex << ", 0.0, 0.0);\n";
ss << "if (indTexBiasSel" << tevIndex << " == 6) bias" << tevIndex << " = vec3(0.0, indVal" << tevIndex << ".y + A" << tevIndex << ", 0.0);\n";
ss << "if (indTexBiasSel" << tevIndex << " == 7) bias" << tevIndex << " = vec3(indVal" << tevIndex << ".x, indVal" << tevIndex << ".y, 0.0) + vec3(A" << tevIndex << ", 0.0, 0.0);\n";


ss << "vec3 indVec" << tevIndex << " = vec3(indVal" << tevIndex << ".xy + bias" << tevIndex << ".xy, 1.0);\n";
}
else
{
        ss << "vec3 indVec" << tevIndex << " = vec3(indVal" << tevIndex << ".xy, 1.0);\n";
}
if (tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_0 ||
    tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_1 ||
    tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_2)
{
    ss << "vec2 indMtx" << tevIndex
       << " = (u_IndMtx[" << indMtxIndex << "] * indVec" << tevIndex << ").xy;\n";
}
else if (tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_S0 ||
         tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_S1 ||
         tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_S2)
{
    ss << "vec2 indMtx" << tevIndex
       << " = u_IndMtx[" << indMtxIndex << "].w * ReadTexCoord"
       << (int)tev.texCoordId << "() * indVec" << tevIndex << ".xx;\n";
}
else if (tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_T0 ||
         tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_T1 ||
         tev.indTexMatrix == EGXIndirectTexMatrixId::IndTexMtx_T2)
{
    ss << "vec2 indMtx" << tevIndex
       << " = u_IndMtx[" << indMtxIndex << "].w * ReadTexCoord"
       << (int)tev.texCoordId << "() * indVec" << tevIndex << ".yy;\n";
}
else
{
    ss << "vec2 indMtx" << tevIndex
       << " = indVec" << tevIndex << ".xy;\n";
}

ss << "indMtx" << tevIndex << " *= vec2(" << scaleS << ", " << scaleT << ");\n";

ss << "indMtx" << tevIndex
   << " = ApplyIndTexWrap(indMtx" << tevIndex
   << ", " << wrapS << ", " << wrapT << ");\n";

if ((int)tev.texMap != 0xFF)
ss << "indOffset" << tevIndex << " = (indMtx" << tevIndex << " / 256.0) / u_TexSize[" << (int)tev.texMap << "];\n";
else
    ss << "indOffset" << tevIndex << " = vec2(0.0);\n";

    return ss.str();
}
    std::string bres::GLSLCompare(const std::string& a, const std::string& b, EGXCompareType c) {
        switch (c) {
        case EGXCompareType::Never:    return "false";
        case EGXCompareType::Less:     return "(" + a + " < " + b + ")";
        case EGXCompareType::Equal:    return "(" + a + " == " + b + ")";
        case EGXCompareType::LEqual:   return "(" + a + " <= " + b + ")";
        case EGXCompareType::Greater:  return "(" + a + " > " + b + ")";
        case EGXCompareType::NEqual:   return "(" + a + " != " + b + ")";
        case EGXCompareType::GEqual:   return "(" + a + " >= " + b + ")";
        case EGXCompareType::Always:   return "true";
        }
        return "true";
    }
std::string bres::GLSLAlphaOp(const std::string& a, const std::string& b, EGXAlphaOp op) {
    switch (op) {
    case EGXAlphaOp::And:
        return "(" + a + " && " + b + ")";
    case EGXAlphaOp::Or:
        return "(" + a + " || " + b + ")";
    case EGXAlphaOp::XOR:
        return "(" + a + " != " + b + ")";
    case EGXAlphaOp::XNOR:
        return "(" + a + " == " + b + ")";
    }
    return "(" + a + " || " + b + ")";
}
std::string bres::GenerateAlphaTestGLSL(const AlphaTest& at) {
    std::ostringstream ss;

    float refA = at.referenceA / 255.0f;
    float refB = at.referenceB / 255.0f;

    auto f2s = [](float v) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << v;
        return oss.str();
    };

    std::string condA = GLSLCompare("t_TevOutput.a", f2s(refA), at.compareA);
    std::string condB = GLSLCompare("t_TevOutput.a", f2s(refB), at.compareB);
    std::string finalCond = GLSLAlphaOp(condA, condB, at.op);

    ss << "// ---- AlphaTest ----\n";
    ss << "if (!(" << finalCond << ")) discard;\n";

    return ss.str();
}
    std::string bres::GenerateFogGLSL() {
        std::ostringstream ss;

        ss << R"(// ---- Fog ----
float z = v_FogZ;

float adjZ = z;
if (u_FogAdjEnabled)
    adjZ = (z - u_FogRangeAdjCenter) * u_FogRangeAdjScale + u_FogRangeAdjCenter;

float fogFactor = 0.0;

if (u_FogType == 1)
    fogFactor = ((adjZ - u_FogStartZ) / (u_FogEndZ - u_FogStartZ));
else if (u_FogType == 2)
    fogFactor = 1.0 - (exp(-adjZ));
else if (u_FogType == 3)
    fogFactor = 1.0 - (exp(-(adjZ * adjZ)));
else if (u_FogType == 4)
    fogFactor = 1.0 - (exp(-adjZ));
else if (u_FogType == 5)
    fogFactor = 1.0 - (exp(-(adjZ * adjZ)));
else if (u_FogType == 6)
    fogFactor = ((adjZ - u_FogStartZ) / (u_FogEndZ - u_FogStartZ));
else if (u_FogType == 7)
    fogFactor = 1.0 - (exp(-adjZ));
else if (u_FogType == 8)
    fogFactor = 1.0 - (exp(-(adjZ * adjZ)));
else if (u_FogType == 9)
    fogFactor = exp(-adjZ);
else if (u_FogType == 10)
    fogFactor = exp(-(adjZ * adjZ));

fogFactor = clamp(fogFactor, 0.0, 1.0);

FragColor.rgb = mix(FragColor.rgb, u_FogColor.rgb, fogFactor);
)";

        return ss.str();
    }
 std::string bres::GenerateTevPipelineGLSL(
    const GXMaterial& mat,
    const std::vector<Color>& colorConstants)
{
    std::ostringstream ss;
    ss << "vec4 rasColor;\n";
    ss << "vec4 texColor;\n";
    ss << "vec3 konstColor;\n";
    ss << "float konstAlpha;\n";
    ss << "vec4 t_TevA, t_TevB, t_TevC, t_TevD;\n";

    for (size_t i = 0; i < 16; i++)
        ss << "vec2 indOffset" << i << " = vec2(0.0);\n";

for (size_t i = 0; i < mat.tevStages.size(); i++) {
    const auto& tev = mat.tevStages[i];

    ss << "\n// ---- TEV Stage " << i << " ----\n";

    std::string baseCoord;
    std::string wrapped = GenerateTevTexCoordWrap(tev, mat);

    int tc = static_cast<int>(tev.texCoordId);

    if (tc < 0 || tc > 7) {
        ss << "vec2 tev" << i << " = vec2(0.0);\n";
    } else {
        if (wrapped.empty()) {
            baseCoord = "ReadTexCoord(" + std::to_string(tc) + ")";
        } else {
            baseCoord = wrapped;
        }
        ss << "vec2 tev" << i << " = " << baseCoord << ";\n";
    }

    std::string indCoord;
    if ((int)tev.indTexStage >= 0 && (int)tev.indTexStage < (int)mat.indTexStages.size()) {
        const auto& ind = mat.indTexStages[(int)tev.indTexStage];
        ss << GenerateIndTexStageGLSL(ind, (int)tev.indTexStage, tev, (int)i);
        indCoord = "indOffset" + std::to_string(i);
    }

    if (!baseCoord.empty() && !indCoord.empty()) {
        ss << "tev" << i << " += " << indCoord << ";\n";
    } else if (baseCoord.empty() && !indCoord.empty()) {
        ss << "tev" << i << " = " << indCoord << ";\n";
    } else if (baseCoord.empty() && indCoord.empty()) {
        ss << "tev" << i << " = vec2(0.0);\n";
    }

    bool usesTex =
        tev.colorInA == EGXCombineColorInput::TexColor ||
        tev.colorInB == EGXCombineColorInput::TexColor ||
        tev.colorInC == EGXCombineColorInput::TexColor ||
        tev.colorInD == EGXCombineColorInput::TexColor;

    if (usesTex) {
        ss << "vec3 srtCoord" << i
           << " = (u_TexSrtMtx[" << (int)tev.texMap << "] * vec4(tev" << i << ", 0.0, 1.0)).xyz;\n";
        ss << "vec2 uv" << i << " = srtCoord" << i << ".xy;\n";
        ss << "vec4 stageTexColor" << i
           << " = texture(u_Tex" << (int)tev.texMap << ", uv" << i << ");\n";
    } else {
        ss << "vec4 stageTexColor" << i << " = vec4(0.0);\n";
    }

    ss << GenerateTevStageGLSL(tev, mat, (int)i);
}

    const auto& last = mat.tevStages.back();

    auto getRegName = [](EGXTevRegister r) -> const char* {
        switch (r) {
        case EGXTevRegister::Reg0: return "tevReg0";
        case EGXTevRegister::Reg1: return "tevReg1";
        case EGXTevRegister::Reg2: return "tevReg2";
        case EGXTevRegister::Prev: return "prev";
        }
        return "prev";
    };

    std::string colorReg = getRegName(last.colorRegId);
    std::string alphaReg = getRegName(last.alphaRegId);

    if (colorReg == alphaReg)
        ss << "vec4 t_TevOutput = " << colorReg << ";\n";
    else
        ss << "vec4 t_TevOutput = vec4(" << colorReg << ".rgb, " << alphaReg << ".a);\n";

    ss << GenerateAlphaTestGLSL(mat.alphaTest);
    ss <<
R"(
if (u_ZBuffer != 0) {
    vec2 uv = gl_FragCoord.xy / u_ViewportSize;
    uv.y = 1.0 - uv.y;

    float depth = texture(u_Z, uv).r;
    float fragZ = gl_FragCoord.z; 

    bool pass = true;
    if (u_DepthFunc == 0) { 
        pass = false;
    } else if (u_DepthFunc == 1) {
        pass = (fragZ < depth);
    } else if (u_DepthFunc == 2) {
        pass = (abs(fragZ - depth) < 0.0005);
    } else if (u_DepthFunc == 3) {
        pass = (fragZ <= depth + 0.0005);
    } else if (u_DepthFunc == 4) {
        pass = (fragZ > depth);
    } else if (u_DepthFunc == 5) {
        pass = (abs(fragZ - depth) >= 0.0005);
    } else if (u_DepthFunc == 6) {
        pass = (fragZ >= depth - 0.0005);
    } else if (u_DepthFunc == 7) {
        pass = true;
    }

    if (!pass)
        discard;
}


if (u_DebugMode == 0)
    FragColor = t_TevOutput;
else if (u_DebugMode == 1)
    FragColor = v_Color1;
else if (u_DebugMode == 2)
    FragColor = vec4(tex0, 0.0, 1.0);
else if (u_DebugMode == 3)
    FragColor = vec4(tex0, 0.0, 1.0);
else if (u_DebugMode == 4)
    FragColor = vec4(vec3(v_FogZ * 0.1), 1.0);
else if (u_DebugMode == 5)
    FragColor = texColor;
else if (u_DebugMode == 6)
    FragColor = prev;
else if (u_DebugMode == 7)
    FragColor = tevReg0;
else if (u_DebugMode == 8)
    FragColor = tevReg1;
else if (u_DebugMode == 9)
    FragColor = tevReg2;
else if (u_DebugMode == 10)
FragColor = vec4(float(u_ChannelCtrl[0].ambSrc == 0),0.0, 0.0, 1.0);


)";

    return ss.str();
}
    std::string bres::GenerateUniformsGLSL(const GXMaterial& mat, int numTextures)
    {
        std::ostringstream ss;

        ss << "// ---- Texture Samplers ----\n";
        for (int i = 0; i < numTextures; ++i) {
            ss << "uniform sampler2D u_Tex" << i << ";\n";
        }
        ss << "uniform sampler2D u_Z;\n";
        ss << "uniform vec2 u_ViewportSize;\n";
        ss << "uniform vec4 u_MatColor[2];\n";
        ss << "uniform vec4 u_AmbColor[2];\n";
        ss << "\n// ---- TexGen Matrices ----\n";
        ss << "uniform mat4 u_TexSrtMtx[8];\n";
        ss << "uniform mat4 u_TexMtx[10];\n";
        ss << "uniform mat4 u_PostTexMtx[20];\n";
        ss << "uniform mat4 u_EffectMtx[8];\n";

        ss << "\n// ---- TEV Registers / Konst ----\n";
        ss << "uniform vec4 u_TevRegPrev;\n";
        ss << "uniform vec4 u_TevReg0;\n";
        ss << "uniform vec4 u_TevReg1;\n";
        ss << "uniform vec4 u_TevReg2;\n";
        ss << "uniform vec4 u_KColor[4];\n";

if (!mat.indTexStages.empty()) {
    ss << "\n// ---- Indirect Texturing ----\n";
    ss << "uniform mat3 u_IndMtx[4];\n";
}


            ss << "\n// ---- Fog ----\n";
            ss << "uniform int   u_FogType;\n";
            ss << "uniform bool  u_FogAdjEnabled;\n";
            ss << "uniform vec4  u_FogColor;\n";
            ss << "uniform float u_FogStartZ;\n";
            ss << "uniform float u_FogEndZ;\n";
            ss << "uniform float u_FogRangeAdjCenter;\n";
            ss << "uniform float u_FogRangeAdjScale;\n";

        if (mat.hasLightsBlock && !mat.lightChannels.empty()) {
            ss << "\n// ---- Lights ----\n";
            ss << "uniform vec3 u_LightPos[8];\n";
            ss << "uniform vec4 u_LightColor[8];\n";
         
        }

        return ss.str();
    }
std::string bres::GenerateFragmentShader(const GXMaterial& mat,
        int numTextures,
        const std::vector<Color>& kColors)
{
    std::ostringstream ss;

    ss << "#version 330 core\n"
        << "//"<< mat.name <<";\n"
       << "in vec4 v_Color0;\n"
       << "in vec4 v_Color1;\n"
       << "in vec4 v_Normal;\n"
       << "in vec4 v_Position;\n"
       << "in float v_FogZ;\n"
       << "out vec4 FragColor;\n\n"
       << "uniform int u_DebugMode;\n\n"
<< "uniform bool  u_LightIsDirectional[8];\n"
<< "uniform int  u_ZBuffer;\n"
<< "uniform int   u_DepthFunc;\n"
<< "uniform vec3  u_LightDir[8];\n"
<< "uniform vec3  u_LightPos[8];\n"
<< "uniform vec3  u_LightColor[8];\n"
<< "uniform vec3  u_LightDistAtten[8];\n"
<< "uniform vec3  u_LightCosAtten[8];\n";
       
ss << "struct ChannelCtrl {\n";
ss << "    bool lightingEnabled;\n";
ss << "    int diffuseFunc;\n";
ss << "    int attnFunc;\n";
ss << "    int lightMask;\n";
ss << "    int ambSrc;\n";
ss << "    int matSrc;\n";
ss << "};\n";
ss << "uniform ChannelCtrl u_ChannelCtrl[2];\n";
    for (int i = 0; i < 8; i++) {
        ss << "in vec3 v_Tex" << i << ";\n";
        ss << "in vec3 v_TexMtx" << i << ";\n";
    }
ss << "uniform int u_TexGenType[8];\n";

    ss << bres::GenerateUniformsGLSL(mat, numTextures) << "\n";

    ss << "vec3 getKonstColor(int sel) {\n"
       << "if (sel == 0) return u_KColor[0].rgb;\n"
       << "if (sel == 1) return u_KColor[1].rgb;\n"
       << "if (sel == 2) return u_KColor[2].rgb;\n"
       << "if (sel == 3) return u_KColor[3].rgb;\n"
       << "if (sel == 4)  return vec3(1.0/8.0);\n"
       << "if (sel == 5)  return vec3(2.0/8.0);\n"
       << "if (sel == 6)  return vec3(3.0/8.0);\n"
       << "if (sel == 7)  return vec3(4.0/8.0);\n"
       << "if (sel == 8)  return vec3(5.0/8.0);\n"
       << "if (sel == 9)  return vec3(6.0/8.0);\n"
       << "if (sel == 10) return vec3(7.0/8.0);\n"
       << "if (sel == 11) return vec3(1.0);\n"
       << "if (sel == 12) return vec3(u_KColor[0].r);\n"
       << "if (sel == 13) return vec3(u_KColor[0].g);\n"
       << "if (sel == 14) return vec3(u_KColor[0].b);\n"
       << "if (sel == 15) return vec3(u_KColor[0].a);\n"
       << "if (sel == 16) return vec3(u_KColor[1].r);\n"
       << "if (sel == 17) return vec3(u_KColor[1].g);\n"
       << "if (sel == 18) return vec3(u_KColor[1].b);\n"
       << "if (sel == 19) return vec3(u_KColor[1].a);\n"
       << "if (sel == 20) return vec3(u_KColor[2].r);\n"
       << "if (sel == 21) return vec3(u_KColor[2].g);\n"
       << "if (sel == 22) return vec3(u_KColor[2].b);\n"
       << "if (sel == 23) return vec3(u_KColor[2].a);\n"
       << "if (sel == 24) return vec3(u_KColor[3].r);\n"
       << "if (sel == 25) return vec3(u_KColor[3].g);\n"
       << "if (sel == 26) return vec3(u_KColor[3].b);\n"
       << "if (sel == 27) return vec3(u_KColor[3].a);\n"
       << "return vec3(0.0);\n"
       << "}\n"
       << "float TevOverflow(float a) {\n"
    << "return float(int(a * 255.0) & 255) / 255.0;\n"
<< "}\n"
<< "\n"
<< "vec3 TevOverflow(vec3 a) {\n"
<< "    return vec3(\n"
<< "        TevOverflow(a.r),\n"
<< "        TevOverflow(a.g),\n"
<< "        TevOverflow(a.b)\n"
<< "    );\n"
<< "}\n"
<< "\n"
<< "vec4 TevOverflow(vec4 a) {\n"
<< "    return vec4(\n"
<< "        TevOverflow(a.r),\n"
<< "        TevOverflow(a.g),\n"
<< "        TevOverflow(a.b),\n"
<< "        TevOverflow(a.a)\n"
<< "    );\n"
<< "}\n"
       << "float getKonstAlpha(int sel) {\n"
       << "if (sel == 7)  return 1.0/8.0;\n"
       << "if (sel == 6)  return 2.0/8.0;\n"
       << "if (sel == 5)  return 3.0/8.0;\n"
       << "if (sel == 4)  return 4.0/8.0;\n"
       << "if (sel == 3)  return 5.0/8.0;\n"
       << "if (sel == 2)  return 6.0/8.0;\n"
       << "if (sel == 1)  return 7.0/8.0;\n"
       << "if (sel == 0)  return 1.0;\n"
       << "if (sel == 8)  return u_KColor[0].a;\n"
       << "if (sel == 9)  return u_KColor[1].a;\n"
       << "if (sel == 10) return u_KColor[2].a;\n"
       << "if (sel == 11) return u_KColor[3].a;\n"
       << "return 0.0;\n}\n";

    for (int i = 0; i < 8; i++) {
    ss << "// ---- ReadTexCoord " << i << " ----\n";
    ss << "vec2 ReadTexCoord" << i << "() {\n";
    ss << "    if (u_TexGenType[" << i << "] == 0) {\n";
    ss << "        return v_Tex" << i << ".xy / v_Tex" << i << ".z;\n";
    ss << "    } else {\n";
    ss << "        return v_Tex" << i << ".xy;\n";
    ss << "    }\n";
    ss << "}\n";
    ss << "// ---- ReadTexCoordInd " << i << " ----\n";
    ss << "vec2 ReadTexCoordInd" << i << "() {\n";
    ss << "    if (u_TexGenType[" << i << "] == 0) {\n";
    ss << "        return v_TexMtx" << i << ".xy / v_TexMtx" << i << ".z;\n";
    ss << "    } else {\n";
    ss << "        return v_TexMtx" << i << ".xy;\n";
    ss << "    }\n";
    ss << "}\n";
    }
ss << R"(
uniform vec2 u_TexSize[8];
vec2 ReadTexCoord(int texCoordId) {
    if (texCoordId == 0) return ReadTexCoord0();
    if (texCoordId == 1) return ReadTexCoord1();
    if (texCoordId == 2) return ReadTexCoord2();
    if (texCoordId == 3) return ReadTexCoord3();
    if (texCoordId == 4) return ReadTexCoord4();
    if (texCoordId == 5) return ReadTexCoord5();
    if (texCoordId == 6) return ReadTexCoord6();
    if (texCoordId == 7) return ReadTexCoord7();
    return vec2(0.0);
}

vec4 ComputeRasColor(int chan)
{
    vec4 mat  = u_MatColor[chan];
    vec4 amb  = u_AmbColor[chan];
    vec4 vcol = (chan == 0 ? v_Color0 : v_Color1);

    if (!u_ChannelCtrl[chan].lightingEnabled) {
        return (u_ChannelCtrl[chan].matSrc == 0) ? mat : vcol;
    }

    vec3 N = normalize(v_Normal.xyz);

    vec3 ambSrc = (u_ChannelCtrl[chan].ambSrc == 0) ? amb.rgb : vcol.rgb;
    vec3 ras = ambSrc;

    int mask = u_ChannelCtrl[chan].lightMask;

    for (int i = 0; i < 8; i++) {
        if (((mask >> i) & 1) == 0)
            continue;

        vec3 L = u_LightIsDirectional[i]
            ? normalize(-u_LightDir[i])
            : normalize(u_LightPos[i] - v_Position.xyz);

        float diff = dot(N, L);

        if (u_ChannelCtrl[chan].diffuseFunc == 0) {
            diff = 1.0;
        } else if (u_ChannelCtrl[chan].diffuseFunc == 1) {
            diff = (diff + 1.0) * 0.5;
        } else if (u_ChannelCtrl[chan].diffuseFunc == 2) {
            diff = max(diff, 0.0);
        }

        float att = 1.0;

        if (u_ChannelCtrl[chan].attnFunc == 1) {
            float dist = length(u_LightPos[i] - v_Position.xyz);
            float distAtt = 1.0 / (u_LightDistAtten[i].x +
                                   u_LightDistAtten[i].y * dist +
                                   u_LightDistAtten[i].z * dist * dist);

            float cosAtt = max(dot(L, normalize(u_LightDir[i])), 0.0);
            float angAtt = u_LightCosAtten[i].x +
                           u_LightCosAtten[i].y * cosAtt +
                           u_LightCosAtten[i].z * cosAtt * cosAtt;

            att = distAtt * angAtt;
        }
        else if (u_ChannelCtrl[chan].attnFunc == 2) {
            float cosAtt = max(dot(L, normalize(u_LightDir[i])), 0.0);
            att = u_LightCosAtten[i].x +
                  u_LightCosAtten[i].y * cosAtt +
                  u_LightCosAtten[i].z * cosAtt * cosAtt;
        }

        ras += u_LightColor[i].rgb * diff * att;
    }

    if (u_ChannelCtrl[chan].matSrc == 0)
        ras *= mat.rgb;
    else
        ras *= vcol.rgb;

    return vec4(ras, mat.a);
}

vec2 ApplyIndTexWrap(vec2 coord, int wrapS, int wrapT)
{
    vec2 outCoord = coord;

    if (wrapS > 0)
        outCoord.x = mod(outCoord.x, float(wrapS));

    if (wrapT > 0)
        outCoord.y = mod(outCoord.y, float(wrapT));

    return outCoord;
}
)";


    ss << "void main() {\n";
    for (int i = 0; i < 8; i++) {
            ss << "vec2 tex" << i << " = ReadTexCoord" << i << "();\n";
        ss << "vec4 texColor" << i << " = texture(u_Tex" << i << ", tex" << i << ");\n";
    }
    ss << "    vec4 prev   = u_TevRegPrev;\n";
ss << "    vec4 tevReg0 = u_TevReg0;\n";
ss << "    vec4 tevReg1 = u_TevReg1;\n";
ss << "    vec4 tevReg2 = u_TevReg2;\n";
    ss << bres::GenerateTevPipelineGLSL(mat, kColors);
    ss << bres::GenerateFogGLSL();
    ss << "}\n";
    return ss.str();
}
    int bres::GetTexMtxIndex(EGXTexMatrix m) {
        int raw = static_cast<int>(m);
        if (raw >= 30 && raw <= 57)
            return (raw - 30) / 3;
        return -1;
    }
    int bres::GetPostTexMtxIndex(EGXPostTexGenMatrix m) {
        if (m >= EGXPostTexGenMatrix::PTTexMtx0 &&
            m <= EGXPostTexGenMatrix::PTTexMtx19)
            return (int(m) - int(EGXPostTexGenMatrix::PTTexMtx0)) / 3;

        return -1;
    }
    std::string bres::GenerateTevTexCoordWrapN(const std::string& coord, EGXIndirectWrapMode wrap)
{
    switch (wrap)
    {
        case EGXIndirectWrapMode::IndWrapMode_Off:   return coord;
        case EGXIndirectWrapMode::IndWrapMode_256:  return "mod(" + coord + ", 256.0)";
        case EGXIndirectWrapMode::IndWrapMode_128:  return "mod(" + coord + ", 128.0)";
        case EGXIndirectWrapMode::IndWrapMode_64:   return "mod(" + coord + ", 64.0)";
        case EGXIndirectWrapMode::IndWrapMode_32:   return "mod(" + coord + ", 32.0)";
        case EGXIndirectWrapMode::IndWrapMode_16:   return "mod(" + coord + ", 16.0)";
        case EGXIndirectWrapMode::IndWrapMode_8:    return "mod(" + coord + ", 8.0)";
        case EGXIndirectWrapMode::IndWrapMode_4:    return "mod(" + coord + ", 4.0)";
        case EGXIndirectWrapMode::IndWrapMode_2:    return "mod(" + coord + ", 2.0)";
        case EGXIndirectWrapMode::IndWrapMode_0:   return coord;    
    }
    return coord;
}
    std::string bres::GenerateTevTexCoordWrap(
        const TevStage& stage,
        const GXMaterial& material)
    {
        if ((int)stage.texCoordId == 0xFF || (int)stage.texMap == 0xFF)
            return "";

        int texGenId = (int)stage.texCoordId;
        int lastTexGenId = material.texGens.size();

if (texGenId < 0)
    return "vec2(0.0)";

if (texGenId >= (int)material.texGens.size())
    texGenId = (int)material.texGens.size() - 1;


        std::string baseCoord = "ReadTexCoord("+std::to_string(texGenId)+")";

        if ((int)stage.indTexWrapS == 0 && (int)stage.indTexWrapT == 0)
            return baseCoord;

        return "vec2(" +
            GenerateTevTexCoordWrapN(baseCoord + ".x", stage.indTexWrapS) + ", " +
            GenerateTevTexCoordWrapN(baseCoord + ".y", stage.indTexWrapT) + ")";
    }
std::string bres::GenerateTexGenGLSL(
    const TexGen& tg,
    int index,
    const ShapeRuntime& runtime)
{
    std::ostringstream ss;

    ss << "// ---- TexGen " << index << " ----\n";
    ss << "vec3 src" << index << " = vec3(0.0, 0.0, 1.0);\n";

    switch (tg.source) {
    case EGXTexGenSrc::Normal:
        ss << "src" << index << " = N_view;\n";
        break;
    case EGXTexGenSrc::Position:
        ss << "src" << index << " = P_view;\n";
        break;
    case EGXTexGenSrc::Color0:
        ss << "src" << index << " = a_Color0.rgb;\n";
        break;
    case EGXTexGenSrc::Color1:
        ss << "src" << index << " = a_Color1.rgb;\n";
        break;

    case EGXTexGenSrc::Tex0: ss << "src" << index << " = vec3(a_Tex0, 1.0);\n"; break;
    case EGXTexGenSrc::Tex1: ss << "src" << index << " = vec3(a_Tex1, 1.0);\n"; break;
    case EGXTexGenSrc::Tex2: ss << "src" << index << " = vec3(a_Tex2, 1.0);\n"; break;
    case EGXTexGenSrc::Tex3: ss << "src" << index << " = vec3(a_Tex3, 1.0);\n"; break;
    case EGXTexGenSrc::Tex4: ss << "src" << index << " = vec3(a_Tex4, 1.0);\n"; break;
    case EGXTexGenSrc::Tex5: ss << "src" << index << " = vec3(a_Tex5, 1.0);\n"; break;
    case EGXTexGenSrc::Tex6: ss << "src" << index << " = vec3(a_Tex6, 1.0);\n"; break;
    case EGXTexGenSrc::Tex7: ss << "src" << index << " = vec3(a_Tex7, 1.0);\n"; break;
    case EGXTexGenSrc::TexCoord0:
    case EGXTexGenSrc::TexCoord1:
    case EGXTexGenSrc::TexCoord2:
    case EGXTexGenSrc::TexCoord3:
    case EGXTexGenSrc::TexCoord4:
    case EGXTexGenSrc::TexCoord5:
    case EGXTexGenSrc::TexCoord6:
    {
        int coord = (int)tg.source - (int)EGXTexGenSrc::TexCoord0;
        ss << "src" << index << " = v_Tex" << coord << ";\n";
        break;
    }
    }
    int texMtxIdx  = GetTexMtxIndex(tg.matrix);
    int postIdx    = GetPostTexMtxIndex(tg.postMatrix);

    switch (tg.type) {
    case EGXTexGenType::Matrix3x4:
        if (texMtxIdx >= 0)
            ss << "vec3 mtx" << index
               << " = (u_TexMtx[" << texMtxIdx << "] * vec4(src" << index << ", 1.0)).xyz;\n";
        else
            ss << "vec3 mtx" << index << " = src" << index << ";\n";
        break;

    case EGXTexGenType::Matrix2x4:
        if (texMtxIdx >= 0)
            ss << "vec2 tmp" << index
               << " = (u_TexMtx[" << texMtxIdx << "] * vec4(src" << index << ", 1.0)).xy;\n"
               << "vec3 mtx" << index << " = vec3(tmp" << index << ", 1.0);\n";
        else
            ss << "vec3 mtx" << index << " = vec3(src" << index << ".xy, 1.0);\n";
        break;

    case EGXTexGenType::SRTG:
        if (tg.source == EGXTexGenSrc::Color0)
            ss << "vec3 mtx" << index << " = vec3(a_Color0.rg, 1.0);\n";
        else
            ss << "vec3 mtx" << index << " = vec3(a_Color1.rg, 1.0);\n";
        break;

    case EGXTexGenType::Bump0:
    case EGXTexGenType::Bump1:
    case EGXTexGenType::Bump2:
    case EGXTexGenType::Bump3:
    case EGXTexGenType::Bump4:
    case EGXTexGenType::Bump5:
    case EGXTexGenType::Bump6:
    case EGXTexGenType::Bump7:
        ss << "int bumpIndex = " << ((int)tg.type - (int)EGXTexGenType::Bump0) << ";\n";
        ss << "vec3 mtx" << index
           << " = vec3(dot(src" << index << ", u_BumpTangent[bumpIndex]), "
           << "dot(src" << index << ", u_BumpBinormal[bumpIndex]), 1.0);\n";
        break;
    }
    if (postIdx >= 0)
        ss << "vec3 tex" << index
           << " = (u_PostTexMtx[" << postIdx << "] * vec4(mtx" << index << ", 1.0)).xyz;\n";
    else
        ss << "vec3 tex" << index << " = mtx" << index << ";\n";
    return ss.str();
}

    std::string bres::GenerateVertexShader(
        const GXMaterial& gxMat,
        const std::vector<MDL0_TexSrtEntry>& texSrts,
        const ShapeRuntime& runtime)
    {
        std::ostringstream ss;

        ss <<
            R"(#version 330 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color0;
layout(location = 3) in vec4 a_Color1;
layout(location = 4) in vec2 a_Tex0;
layout(location = 5) in vec2 a_Tex1;
layout(location = 6) in vec2 a_Tex2;
layout(location = 7) in vec2 a_Tex3;
layout(location = 8) in vec2 a_Tex4;
layout(location = 9) in vec2 a_Tex5;
layout(location = 10) in vec2 a_Tex6;
layout(location = 11) in vec2 a_Tex7;
layout(location = 12) in uvec4 a_BoneIndex;
layout(location = 13) in vec4 a_BoneWeight;


uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Proj;

uniform vec3 u_CameraPos;
uniform vec3 u_LightPos[8];

uniform mat4 u_TexMtx[10];
uniform mat4 u_PostTexMtx[20];
uniform mat4 u_EffectMtx[8];
uniform mat4 u_BoneMatrix[32];
uniform mat4 u_FinalSRT[8];

out vec4 v_Color0;
out vec4 v_Color1;
out vec4 v_Normal;
out vec4 v_Position;
)";

        // ---- TexGen outputs ----
        for (int i = 0; i < 8; i++)
        {
            ss << "out vec3 v_Tex" << i << ";\n";
            ss << "out vec3 v_TexMtx" << i << ";\n";
        }

        ss <<
            R"(
out float v_FogZ;


void main()
{
bool rigid = (a_BoneWeight.x == 1.0 && a_BoneIndex.x == 0u);
vec4 skinnedPos;
vec3 skinnedNrm;
if (rigid) {
    skinnedPos = vec4(a_Position, 1.0);
    skinnedNrm = a_Normal;
} else {

skinnedPos = 
    u_BoneMatrix[a_BoneIndex.x] * vec4(a_Position, 1.0) * a_BoneWeight.x +
    u_BoneMatrix[a_BoneIndex.y] * vec4(a_Position, 1.0) * a_BoneWeight.y +
    u_BoneMatrix[a_BoneIndex.z] * vec4(a_Position, 1.0) * a_BoneWeight.z +
    u_BoneMatrix[a_BoneIndex.w] * vec4(a_Position, 1.0) * a_BoneWeight.w;

mat3 boneRot0 = mat3(u_BoneMatrix[a_BoneIndex.x]);
mat3 boneRot1 = mat3(u_BoneMatrix[a_BoneIndex.y]);
mat3 boneRot2 = mat3(u_BoneMatrix[a_BoneIndex.z]);
mat3 boneRot3 = mat3(u_BoneMatrix[a_BoneIndex.w]);

skinnedNrm =
      (boneRot0 * a_Normal) * a_BoneWeight.x +
      (boneRot1 * a_Normal) * a_BoneWeight.y +
      (boneRot2 * a_Normal) * a_BoneWeight.z +
      (boneRot3 * a_Normal) * a_BoneWeight.w;

skinnedNrm = normalize(skinnedNrm);
}
vec4 worldPos = u_Model * skinnedPos;
mat3 normalMtx = mat3(transpose(inverse(u_Model)));
skinnedNrm = normalize(normalMtx * skinnedNrm);


gl_Position = u_Proj * u_View * worldPos;

    v_Normal = vec4(skinnedNrm, 0.0);
    v_Color0 = a_Color0;
    v_Color1 = a_Color1;

   v_FogZ = abs((u_View * worldPos).z);


    vec3 N_view = (u_View * vec4(skinnedNrm, 0.0)).xyz;
    vec3 P_view = (u_View * worldPos).xyz;
    v_Position = worldPos;
)";

        for (int i = 0; i < 8; i++) {
            ss << "v_Tex" << i << " = vec3(0.0,0.0,0.0);\n";
            ss << "v_TexMtx" << i << " = vec3(0.0,0.0,0.0);\n";
        }
        for (int i = 0; i < gxMat.texGens.size(); i++) {
            ss << GenerateTexGenGLSL(gxMat.texGens[i], i, runtime);
            ss << "v_Tex" << i << " = tex" << i << ";\n";
            ss << "v_TexMtx" << i << " = mtx" << i << ";\n";
        }

        ss << "}\n";
        return ss.str();
    }
    static inline const float* ColorPtr(const Color& c) {
        return &c.r;
    }
 uint32_t bres::getAttributeFormatCompFlagsRaw(EGXAttribute3 attr, EGXComponentCount compCnt)
 {
     switch (attr) {
     case EGXAttribute3::Normal:
         if (compCnt == EGXComponentCount::Normal_NBT3) return 9;
         if (compCnt == EGXComponentCount::Normal_NBT)  return 6;
         return 3;

     case EGXAttribute3::Position:
         return (compCnt == EGXComponentCount::Position_XY) ? 2 : 3;

     case EGXAttribute3::TexCoord0:
     case EGXAttribute3::TexCoord1:
     case EGXAttribute3::TexCoord2:
     case EGXAttribute3::TexCoord3:
     case EGXAttribute3::TexCoord4:
     case EGXAttribute3::TexCoord5:
     case EGXAttribute3::TexCoord6:
     case EGXAttribute3::TexCoord7:
         return (compCnt == EGXComponentCount::TexCoord_U) ? 1 : 2;

     case EGXAttribute3::Color0:
     case EGXAttribute3::Color1:
         return (compCnt == EGXComponentCount::Color_RGB) ? 3 : 4;

     default:
         return 0;
     }
 }
    inline void bres::calcTexMtx_Basic(glm::mat4& dst,
        float scaleS, float scaleT,
        float rotation,
        float translationS, float translationT)
    {
        const float theta = rotation * (glm::pi<float>() / 180.0f);
        const float sinR = std::sin(theta);
        const float cosR = std::cos(theta);

        dst = glm::mat4(1.0f);

        dst[0][0] = scaleS * cosR;
        dst[1][0] = scaleS * sinR;
        dst[0][1] = scaleT * -sinR;
        dst[1][1] = scaleT * cosR;
        dst[3][0] = translationS;
        dst[3][1] = translationT;
    }

    inline void bres::calcTexMtx_Maya(glm::mat4& dst,
        float scaleS, float scaleT,
        float rotation,
        float translationS, float translationT)
    {
        const float theta = rotation * (glm::pi<float>() / 180.0f);
        const float sinR = std::sin(theta);
        const float cosR = std::cos(theta);

        dst = glm::mat4(1.0f);

        dst[0][0] = scaleS * cosR;
        dst[1][0] = scaleS * sinR;
        dst[0][1] = scaleT * -sinR;
        dst[1][1] = scaleT * cosR;

        dst[3][0] = scaleS * ((-0.5f * cosR) - (0.5f * sinR - 0.5f) - translationS);
        dst[3][1] = scaleT * ((-0.5f * cosR) + (0.5f * sinR - 0.5f) + translationT) + 1.0f;
    }

    inline void bres::calcTexMtx_XSI(glm::mat4& dst,
        float scaleS, float scaleT,
        float rotation,
        float translationS, float translationT)
    {
        const float theta = rotation * (glm::pi<float>() / 180.0f);
        const float sinR = std::sin(theta);
        const float cosR = std::cos(theta);

        dst = glm::mat4(1.0f);

        dst[0][0] = scaleS * cosR;
        dst[1][0] = scaleS * sinR;
        dst[0][1] = scaleT * -sinR;
        dst[1][1] = scaleT * cosR;

        dst[3][0] = (scaleS * sinR) - (scaleS * cosR * translationS) - (scaleS * sinR * translationT);
        dst[3][1] = (scaleT * -cosR) - (scaleT * sinR * translationS) + (scaleT * cosR * translationT) + 1.0f;
    }

    inline void bres::calcTexMtx_Max(glm::mat4& dst,
        float scaleS, float scaleT,
        float rotation,
        float translationS, float translationT)
    {
        const float theta = rotation * (glm::pi<float>() / 180.0f);
        const float sinR = std::sin(theta);
        const float cosR = std::cos(theta);

        dst = glm::mat4(1.0f);

        dst[0][0] = scaleS * cosR;
        dst[1][0] = scaleS * sinR;
        dst[0][1] = scaleT * -sinR;
        dst[1][1] = scaleT * cosR;

        dst[3][0] = scaleS * ((-cosR * (translationS + 0.5f)) + (sinR * (translationT - 0.5f))) + 0.5f;
        dst[3][1] = scaleT * ((sinR * (translationS + 0.5f)) + (cosR * (translationT - 0.5f))) + 0.5f;
    }

    inline void bres::calcTexMtx(glm::mat4& dst,
        TexMatrixMode mode,
        float scaleS, float scaleT,
        float rotation,
        float translationS, float translationT)
    {
        switch (mode) {
        case TexMatrixMode::Basic:
            return calcTexMtx_Basic(dst, scaleS, scaleT, rotation, translationS, translationT);
        case TexMatrixMode::Maya:
            return calcTexMtx_Maya(dst, scaleS, scaleT, rotation, translationS, translationT);
        case TexMatrixMode::XSI:
            return calcTexMtx_XSI(dst, scaleS, scaleT, rotation, translationS, translationT);
        case TexMatrixMode::Max:
            return calcTexMtx_Max(dst, scaleS, scaleT, rotation, translationS, translationT);
        default:
            return;
        }
    }
inline std::string bres::ReadString(bStream::CStream* base, size_t p)
{
    base->seek(p);
    std::string result;

    while (true) {
        uint8_t b = base->readUInt8();
        if (b == '\0')
            break;
        result.push_back((char)b);
    }

    return result;
}


std::vector<bres::ResDicEntry> bres::ParseResDic(bStream::CStream* stream, uint32_t tableOffs)
{
    std::vector<ResDicEntry> entries;

    if (tableOffs == 0)
        return entries;

    stream->seek(tableOffs);

    uint32_t tableSize = stream->readUInt32();
    uint32_t tableCount = stream->readUInt32();

    uint32_t tableIdx = tableOffs + 0x08;
    tableIdx += 0x10;

    for (uint32_t i = 0; i < tableCount; i++)
    {
        ResDicEntry e;
        e.name = ReadString(stream, tableOffs + stream->peekUInt32(tableIdx + 0x08));
        e.offs = tableOffs + stream->peekUInt32(tableIdx + 0x0C);

        entries.push_back(e);
        tableIdx += 0x10;
    }

    return entries;
}
    std::optional<bres::ResUserData> bres::ParseUserData(
        bStream::CStream* stream,
        size_t sectionSize,
        uint32_t offs)
    {
        if (offs == 0)
            return std::nullopt;

        uint32_t size = stream->readUInt32();

        auto resDic = ParseResDic(stream, offs + 0x04);

        ResUserData result;

        for (auto& entry : resDic) {
            uint32_t itemOffs = entry.offs;
            stream->seek(itemOffs);
            uint32_t itemSize = stream->readUInt32();
            uint32_t toData = stream->readUInt32();
            uint32_t arraySize = stream->readUInt32();
            auto userDataType = static_cast<ResUserDataItemValueType>(stream->readUInt32());

            uint32_t nameOffRel = stream->readUInt32();
            std::string name = ReadString(stream, itemOffs+nameOffRel);

            uint32_t id = stream->readUInt32();

            uint32_t dataBase = itemOffs + toData;

            switch (userDataType) {
            case ResUserDataItemValueType::S32: {
                ResUserDataItemNumber item;
                item.userDataType = userDataType;
                item.name = name;
                item.id = id;

                for (uint32_t j = 0; j < arraySize; ++j) {
                    stream->seek(dataBase + 4 * j);
                    int32_t v = stream->readInt32();
                    item.value.push_back(float(v));
                }

                result.entries.emplace_back(std::move(item));
                break;
            }

            case ResUserDataItemValueType::F32: {
                ResUserDataItemNumber item;
                item.userDataType = userDataType;
                item.name = name;
                item.id = id;

                for (uint32_t j = 0; j < arraySize; ++j) {
                    stream->seek(dataBase + 4 * j);
                    float v = stream->readFloat();
                    item.value.push_back(v);
                }

                result.entries.emplace_back(std::move(item));
                break;
            }

            case ResUserDataItemValueType::STRING: {
                ResUserDataItemString item;
                item.userDataType = userDataType;
                item.name = name;
                item.id = id;

                for (uint32_t j = 0; j < arraySize; ++j) {
                    uint32_t strOffRel = stream->seek(dataBase + 4 * j);
                    item.value.push_back(ReadString(stream, dataBase+strOffRel));
                }

                result.entries.emplace_back(std::move(item));
                break;
            }
            }
        }

        return result;
    }
    void displayListRegistersRun(bStream::CStream* stream, DisplayListRegisters& r, uint32_t data, size_t size)
    {
        size_t i = 0;
        stream->seek(data);
        while (i < size) {
            uint8_t cmd = stream->readUInt8();
            i++;
            if (cmd == 0x00)
                continue;
            if (cmd == 0x08) { 
                uint8_t regAddr = stream->readUInt8();
                i++;
                uint32_t regValue = stream->readUInt32();
                i += 4;
                r.cp[regAddr] = regValue;
                continue;
            }
            if (cmd == 0x61) {
                uint32_t regBag = stream->readUInt32();
                i += 4;
                r.bpSet(regBag);
                continue;
            }
            if (cmd == 0x10) {
                uint16_t len = stream->readUInt16() + 1;
                i += 2;
                assert(len <= 0x10);
                uint16_t regAddr = stream->readUInt16();
                i += 2;

                for (uint16_t j = 0; j < len; j++) {
                    stream->seek(data + i);
                    r.xfSet(regAddr, j, stream->readUInt32());
                    i += 4;
                }

                for (uint16_t j = len; j < 16; j++)
                    r.xfSet(regAddr, j, 0);
               

                continue;
            }
            break;
        }
    }
    inline bres::PLT0 bres::ParsePLT0(bStream::CStream* stream,size_t p)
    {
        stream->seek(p);
        std::string magic = stream->readString(4);
        const uint32_t version = stream->readUInt32();
        if (version != 0x01 && version != 0x03)
            throw std::runtime_error("Unsupported PLT0 version");

        const uint32_t dataOffs = stream->readUInt32();
        const uint32_t nameOffs = stream->readUInt32();
        const std::string name = ReadString(stream, p + nameOffs);
        stream->seek(p + 0x18);

        const uint32_t format = stream->readUInt32();
        const uint16_t numEntries = stream->readUInt16();

        PLT0 plt;
        plt.name = name;
        plt.format = format;
        plt.data.assign(dataOffs, dataOffs + numEntries * 2);

        return plt;
    }
    inline bres::TEX0 bres::ParseTEX0(bStream::CStream* stream, size_t p)
    {
        stream->seek(p+8);
        uint32_t version = stream->readUInt32();
        stream->skip(0x4);
        if (version == 2)
        {
            stream->skip(0x4);
            printf("TEX0 Version 2 detected. it's maybe draw_demo.szs\n");
        }
        if (version != 1 && version != 2 && version != 3)
        {
            assert(!"Unsupported TEX0 Version");
        }
        uint32_t dataOffs = stream->readUInt32();
        uint32_t nameOffs = stream->readUInt32();
        std::string name = ReadString(stream,p+nameOffs);
        if (version == 2)
        {
            stream->seek(p + 0x1C);
        }
        else
        {
            stream->seek(p + 0x18);
        }
        uint32_t flags = stream->readUInt32();
        uint16_t width = stream->readUInt16();
        uint16_t height = stream->readUInt16();
        uint32_t format = stream->readUInt32();
        uint32_t mipCountRaw = stream->readUInt32();
        float    minLOD = stream->readFloat();
        float    maxLOD = stream->readFloat();

        uint32_t mipCount = static_cast<uint32_t>(std::ceil(std::min<float>(mipCountRaw, maxLOD + 1.0f)));

        TEX0 tex;
        tex.name = name;
        tex.width = width;
        tex.height = height;
        tex.format = format;
        tex.mipCount = mipCount;
        tex.minLOD = minLOD;
        tex.maxLOD = maxLOD;
        tex.data = p + dataOffs;

        return tex;
    }

    void bres::ParseMDL0_TevEntry(bStream::CStream* stream, DisplayListRegisters& r, uint32_t numStagesCheck,size_t p)
    {
        stream->seek(p);
        const uint32_t size = stream->readUInt32();
        assert(size == 480 + 32);

        const uint32_t index = stream->readUInt32();
        const uint8_t numStages = stream->readUInt8();

        const uint32_t dlOffs = 0x20;

        const uint32_t dlSize = 480;

        displayListRegistersRun(stream,r, p+dlOffs, dlSize);
    }
    bres::MDL0_MaterialEntry bres::ParseMDL0_MaterialEntry(bStream::CStream* stream, uint32_t version, size_t p)
    {
        MDL0_MaterialEntry out{};
        stream->seek(p + 0x8);
        const uint32_t nameOffs = stream->readUInt32();
        out.name = ReadString(stream, p + nameOffs);

        stream->seek(p + 0x0c);
        out.index = stream->readUInt32();
        const uint32_t flags = stream->readUInt32();
        out.translucent = (flags & 0x80000000u) != 0;


        const uint8_t numTexGens = stream->readUInt8();
        const uint8_t numChans = stream->readUInt8();
        const uint8_t numTevs = stream->readUInt8();
        const uint8_t numInds = stream->readUInt8();

        EGXCullMode cullMode = static_cast<EGXCullMode>(stream->readUInt32());

        const bool zCompLoc = stream->readUInt8() != 0;
        out.zCompLoc = zCompLoc;
        out.lightSetIdx = static_cast<int8_t>(stream->readUInt8());
        out.fogIdx = static_cast<int8_t>(stream->readUInt8());

        stream->seek(p + 0x20);
        const uint8_t indMethod0 = stream->readUInt8();
        const uint8_t indMethod1 = stream->readUInt8();
        const uint8_t indMethod2 = stream->readUInt8();
        const uint8_t indMethod3 = stream->readUInt8();

        const uint8_t nrmRefLight0 = stream->readUInt8();
        const uint8_t nrmRefLight1 = stream->readUInt8();
        const uint8_t nrmRefLight2 = stream->readUInt8();
        const uint8_t nrmRefLight3 = stream->readUInt8();

        const uint32_t tevOffs = stream->readUInt32();
        assert(numTevs <= 16);
        const uint32_t numTexPltt = stream->readUInt32();
        const uint32_t texPlttOffs = stream->readUInt32();

        uint32_t endOfHeaderOffs = 0x34;
        if (version >= 0x0A)
            endOfHeaderOffs += 0x04; 
        endOfHeaderOffs += 0x04;    


        DisplayListRegisters r;
        stream->seek(p + endOfHeaderOffs);
        const uint32_t matDLOffs = stream->readUInt32();
        const uint32_t matDLSize = 32 + 128 + 64 + 160;
        displayListRegistersRun(stream, r, p + matDLOffs, matDLSize);

        ParseMDL0_TevEntry(stream, r, numTevs, p + tevOffs);

        GXMaterialBuilder mb;

        for (uint8_t i = 0; i < numTexGens; ++i)
            mb.setTexGenFromRegisters(r, i);
        for (uint8_t i = 0; i < numTevs; ++i)
            mb.setTevStageFromRegisters(r, i);
        for (uint8_t i = 0; i < numInds; ++i)
            mb.setIndTexStageFromRegisters(r, i);

        mb.setRopStateFromRegisters(r);
        mb.setCullMode(cullMode);
        mb.setColorUpdate(true);
        mb.setAlphaUpdate(false);

        out.indTexMatrices.clear();
        out.indTexMatrices.reserve(3);

        for (int i = 0; i < 3; ++i) {
            const int indTexScaleBase = 10;
            const int indTexScaleBias = 0x11;

            const int indOffs = i * 3;
            uint32_t mtxA = r.bp[0x06 + indOffs];
            uint32_t mtxB = r.bp[0x07 + indOffs];
            uint32_t mtxC = r.bp[0x08 + indOffs];

            const uint32_t scaleBitsA = (mtxA >> 22) & 0x03;
            const uint32_t scaleBitsB = (mtxB >> 22) & 0x03;
            const uint32_t scaleBitsC = (mtxC >> 22) & 0x03;
            const uint32_t scaleExp = (scaleBitsC << 4) | (scaleBitsB << 2) | scaleBitsA;
            const float scale = std::pow(2.0f, static_cast<float>(scaleExp - indTexScaleBias - indTexScaleBase));

            auto s11 = [](uint32_t v) -> int32_t {
                return static_cast<int32_t>((v << 21)) >> 21;
            };

            const int32_t p00 = s11((mtxA >> 0) & 0x07FF);
            const int32_t p10 = s11((mtxA >> 11) & 0x07FF);
            const int32_t p01 = s11((mtxB >> 0) & 0x07FF);
            const int32_t p11 = s11((mtxB >> 11) & 0x07FF);
            const int32_t p02 = s11((mtxC >> 0) & 0x07FF);
            const int32_t p12 = s11((mtxC >> 11) & 0x07FF);

            std::array<float, 8> m = {
                p00 * scale, p01 * scale, p02 * scale, scale,
                p10 * scale, p11 * scale, p12 * scale, 0.0f,
            };
            out.indTexMatrices.push_back(m);
        }

        out.colorRegisters.clear();
        out.colorConstants.clear();
        out.colorRegisters.resize(4);
        out.colorConstants.resize(4);

        for (int i = 0; i < 8; ++i) {
            uint32_t vl = r.tevKColor[i * 2 + 0];
            uint32_t vh = r.tevKColor[i * 2 + 1];

            float cr = ((vl >> 0) & 0x7FF) / 255.0f;
            float ca = ((vl >> 12) & 0x7FF) / 255.0f;
            float cb = ((vh >> 0) & 0x7FF) / 255.0f;
            float cg = ((vh >> 12) & 0x7FF) / 255.0f;

            Color c = colorNewFromRGBA(cr, cg, cb, ca);
            if (i < 4)
                out.colorRegisters[i] = c;
            else
                out.colorConstants[i - 4] = c;
        }

        out.colorMatRegs.clear();
        out.colorAmbRegs.clear();

        uint32_t lightChannelTableIdx = endOfHeaderOffs + 0x3B4;

for (int i = 0; i < 2; ++i) {
    uint32_t flagsChan = stream->peekUInt32(p + lightChannelTableIdx + 0x00);

    float matColorR = stream->peekUInt8(p + lightChannelTableIdx + 0x04) / 255.0f;
    float matColorG = stream->peekUInt8(p + lightChannelTableIdx + 0x05) / 255.0f;
    float matColorB = stream->peekUInt8(p + lightChannelTableIdx + 0x06) / 255.0f;
    float matColorA = stream->peekUInt8(p + lightChannelTableIdx + 0x07) / 255.0f;

    float ambColorR = stream->peekUInt8(p + lightChannelTableIdx + 0x08) / 255.0f;
    float ambColorG = stream->peekUInt8(p + lightChannelTableIdx + 0x09) / 255.0f;
    float ambColorB = stream->peekUInt8(p + lightChannelTableIdx + 0x0A) / 255.0f;
    float ambColorA = stream->peekUInt8(p + lightChannelTableIdx + 0x0B) / 255.0f;

    uint32_t chanCtrlC = stream->peekUInt32(p + lightChannelTableIdx + 0x0C);
    uint32_t chanCtrlA = stream->peekUInt32(p + lightChannelTableIdx + 0x10);

    if (i < numChans) {
        r.xfSet(0x100E + i, 0, chanCtrlC);
        r.xfSet(0x1010 + i, 0, chanCtrlA);
        mb.setColorChannelFromRegisters(r, i);
    }


            out.colorMatRegs.push_back(colorNewFromRGBA(matColorR, matColorG, matColorB, matColorA));
            out.colorAmbRegs.push_back(colorNewFromRGBA(ambColorR, ambColorG, ambColorB, ambColorA));

            lightChannelTableIdx += 0x14;
        }

        out.gxMaterial = mb.finish(out.name);

        out.gxMaterial.texMatrices.clear();
out.gxMaterial.texMatrices.assign(10, glm::mat4(1.0f));

for (size_t i = 0; i < out.texSrts.size(); ++i) {
    const auto& tg  = out.gxMaterial.texGens[i];
    const auto& srt = out.texSrts[i];

    int slot = GetTexMtxIndex(tg.matrix);
    if (slot < 0)
        continue;

    switch (srt.mapMode) {

    case MapMode::TEXCOORD:
        out.gxMaterial.texMatrices[slot] = srt.srtMtx;
        break;

    case MapMode::ENV_CAMERA:
    case MapMode::ENV_LIGHT:
        out.gxMaterial.texMatrices[slot] = srt.effectMtx;
        break;

    case MapMode::PROJECTION:
        break;
    }
}
out.gxMaterial.postTexMatrices.clear();
out.gxMaterial.postTexMatrices.assign(20, glm::mat4(1.0f));

for (size_t i = 0; i < out.texSrts.size(); ++i) {
    const auto& tg  = out.gxMaterial.texGens[i];
    const auto& srt = out.texSrts[i];

    int postSlot = GetPostTexMtxIndex(tg.postMatrix);
    if (postSlot < 0)
        continue;

    switch (srt.mapMode) {
    case MapMode::PROJECTION: {
        out.gxMaterial.postTexMatrices[postSlot] = glm::mat4(1.0f);
        break;
    }
    case MapMode::ENV_CAMERA:
    case MapMode::ENV_LIGHT: {
        out.gxMaterial.postTexMatrices[postSlot] = glm::mat4(1.0f);
        break;
    }
    default:
        out.gxMaterial.postTexMatrices[postSlot] = glm::mat4(1.0f);
        break;
    }
}


        out.samplers.clear();

        for (uint32_t i = 0; i < numTexPltt; ++i) {
            uint32_t texPlttInfoOffs = texPlttOffs + i * 0x34;
            stream->seek(p + texPlttInfoOffs);


            uint32_t nameTexOffs = stream->readUInt32();
            uint32_t namePltOffs = stream->readUInt32();
            uint32_t unk1 = stream->readUInt32();
            uint32_t unk2 = stream->readUInt32();

            EGXTexMapSlot texMapId = static_cast<EGXTexMapSlot>(stream->readUInt32());
            uint32_t tlutId = stream->readUInt32();
            EGXWrapMode wrapS = static_cast<EGXWrapMode>(stream->readUInt32());
            EGXWrapMode wrapT = static_cast<EGXWrapMode>(stream->readUInt32());
            EGXFilterMode minFilter = static_cast<EGXFilterMode>(stream->readUInt32());
            EGXFilterMode magFilter = static_cast<EGXFilterMode>(stream->readUInt32());
            float lodBias = stream->readFloat();
            uint32_t maxAniso = stream->readUInt32();
            uint8_t biasClamp = stream->readUInt8();
            uint8_t edgeLod = stream->readUInt8();

            std::string nameTex = ReadString(stream, p + texPlttInfoOffs + nameTexOffs);

            std::string namePlt;
            if (namePltOffs != 0)
                namePlt = ReadString(stream, p + texPlttInfoOffs + namePltOffs);

            MDL0_MaterialSamplerEntry sampler{};
            sampler.name = nameTex;
            sampler.namePalette = namePlt;
            sampler.wrapS = wrapS;
            sampler.wrapT = wrapT;
            sampler.minFilter = minFilter;
            sampler.magFilter = magFilter;
            sampler.lodBias = lodBias;
            sampler.texMapSlot = (int)texMapId;
            if (static_cast<size_t>(texMapId) >= out.samplers.size())
                out.samplers.resize(static_cast<size_t>(texMapId) + 1);

            out.samplers[static_cast<size_t>(texMapId)] = sampler;
        }

        stream->seek(p + 0x16c);
        uint32_t srtFlags = static_cast<uint32_t>(stream->readUInt32());

        TexMatrixMode texMtxMode = static_cast<TexMatrixMode>(stream->readUInt32());


        uint32_t texSrtTableIdx = endOfHeaderOffs + 0x174;
        uint32_t texMtxTableIdx = endOfHeaderOffs + 0x214;

        out.texSrts.clear();
        out.texSrts.reserve(8);
        for (int i = 0; i < 8; ++i) {
            enum Flags : uint32_t {
                SCALE_ONE = 0x02,
                ROT_ZERO = 0x04,
                TRANS_ZERO = 0x08,
            };
            stream->seek(p + texSrtTableIdx);
            uint32_t srtFlag = (srtFlags >> (i * 4)) & 0x0F;


            float scaleS = (srtFlag & SCALE_ONE) ? 1.0f : stream->readFloat();
            float scaleT = (srtFlag & SCALE_ONE) ? 1.0f : stream->readFloat();
            float rotation = (srtFlag & ROT_ZERO) ? 0.0f : stream->readFloat();
            float translationS = (srtFlag & TRANS_ZERO) ? 0.0f : stream->readFloat();
            float translationT = (srtFlag & TRANS_ZERO) ? 0.0f : stream->readFloat();

            stream->seek(p + texMtxTableIdx);
            int refCamera = stream->readUInt8();
            int refLight = stream->readUInt8();
            MapMode mapMode = static_cast<MapMode>(stream->readUInt8());
            int miscFlags = stream->readUInt8();

            float m00 = stream->readFloat();
            float m01 = stream->readFloat();
            float m02 = stream->readFloat();
            float m03 = stream->readFloat();
            float m10 = stream->readFloat();
            float m11 = stream->readFloat();
            float m12 = stream->readFloat();
            float m13 = stream->readFloat();
            float m20 = stream->readFloat();
            float m21 = stream->readFloat();
            float m22 = stream->readFloat();
            float m23 = stream->readFloat();

            glm::mat4 effectMtx(1.0f);
            effectMtx[0][0] = m00; effectMtx[1][0] = m10; effectMtx[2][0] = m20; effectMtx[3][0] = 0.0f;
            effectMtx[0][1] = m01; effectMtx[1][1] = m11; effectMtx[2][1] = m21; effectMtx[3][1] = 0.0f;
            effectMtx[0][2] = m02; effectMtx[1][2] = m12; effectMtx[2][2] = m22; effectMtx[3][2] = 0.0f;
            effectMtx[0][3] = m03; effectMtx[1][3] = m13; effectMtx[2][3] = m23; effectMtx[3][3] = 1.0f;

            switch (mapMode) {
            case MapMode::TEXCOORD:
                break;
            case MapMode::PROJECTION:
                out.gxMaterial.texGens[i].matrix = static_cast<EGXTexMatrix>(static_cast<int>(EGXTexMatrixType::Matrix2x4));
                break;
            case MapMode::ENV_CAMERA:
            case MapMode::ENV_LIGHT:
                out.gxMaterial.texGens[i].matrix =
                    static_cast<EGXTexMatrix>(static_cast<int>(EGXTexMatrixType::Matrix3x4) + i * 3);
                break;
            default:
                break;
            }

            glm::mat4 srtMtx(1.0f);
            calcTexMtx(srtMtx, texMtxMode, scaleS, scaleT, rotation, translationS, translationT);

            MDL0_TexSrtEntry texSrt{};
            texSrt.refCamera = refCamera;
            texSrt.refLight = refLight;
            texSrt.mapMode = mapMode;
            texSrt.scaleS = scaleS;
            texSrt.scaleT = scaleT;
            texSrt.rotation = rotation;
            texSrt.translationS = translationS;
            texSrt.translationT = translationT;
            texSrt.srtMtx = srtMtx;
            texSrt.effectMtx = effectMtx;

            out.texSrts.push_back(texSrt);

            texSrtTableIdx += 0x14;
            texMtxTableIdx += 0x34;
        }

        return out;
    }

    bres::VtxBufferData bres::parseMDL0_VtxData(
        bStream::CStream* stream, 
        size_t mdl0Size,
        uint32_t entryOffs,
        EGXAttribute3 vtxAttrib,size_t p)
    {
        stream->seek(entryOffs + 0x08);

        const uint32_t dataOffs = stream->readUInt32();
        const uint32_t nameOffs = stream->readUInt32();

        const std::string name = ReadString(stream, entryOffs + nameOffs);
        stream->seek(entryOffs + 0x10);
        uint32_t id = stream->readUInt32();
        uint32_t compCnt = stream->readUInt32();
        uint32_t compType = stream->readUInt32();
        uint8_t compShift = stream->readUInt8();
        uint8_t stride = stream->readUInt8();
    if (vtxAttrib == EGXAttribute3::Color0) {
        stride    = compShift;
        compShift = 0;
    }

        if (vtxAttrib >= EGXAttribute3::TexCoord0 &&
            vtxAttrib <= EGXAttribute3::TexCoord7)
        {
            if (stride == 4)
                compType = (uint32_t)EGXComponentType::Signed16;
            else if (stride == 8)
                compType = (uint32_t)EGXComponentType::Float;
        }
        uint16_t count = stream->readUInt16();

        const int numComponents =
            getFormatCompFlagsComponentCount(
                getAttributeFormatCompFlagsRaw(vtxAttrib, EGXComponentCount(compCnt)));
        const int compSize = getAttributeComponentByteSizeRaw(EGXComponentType(compType));
        const int compByteSize = numComponents * compSize;

        const size_t dataByteSize = size_t(compByteSize) * size_t(count) + 4;

        std::vector<uint8_t> data(dataByteSize);
stream->seek(entryOffs + dataOffs);

for (size_t i = 0; i < dataByteSize; i++)
    data[i] = stream->readUInt8();
        return VtxBufferData{
            name,
            id,
            EGXComponentCount(compCnt),
            EGXComponentType(compType),
            compShift,
            stride,
            count,
            std::move(data),
            0
        };
    }

    std::vector<bres::VtxBufferData> bres::parseInputBufferSet(
        bStream::CStream* stream,
        size_t mdl0Size,
        EGXAttribute3 vtxAttrib,
        const std::vector<ResDicEntry>& resDic,size_t p)
    {
        std::vector<VtxBufferData> out;
        out.reserve(resDic.size());

        for (size_t i = 0; i < resDic.size(); i++) {
            const auto& entry = resDic[i];
            VtxBufferData vtx = parseMDL0_VtxData(
                stream,     
                mdl0Size,      
                entry.offs,  
                vtxAttrib,p
            );

            assert(vtx.name == entry.name);
            assert(vtx.id == i);

            out.push_back(std::move(vtx));
        }
        return out;
    }
    bres::InputVertexBuffers bres::parseInputVertexBuffers(
        bStream::CStream* stream,
        size_t mdl0Size,
        const std::vector<ResDicEntry>& posDic,
        const std::vector<ResDicEntry>& nrmDic,
        const std::vector<ResDicEntry>& clrDic,
        const std::vector<ResDicEntry>& txcDic,size_t p)
    {
        InputVertexBuffers out;

        out.pos = parseInputBufferSet(stream, mdl0Size, EGXAttribute3::Position, posDic,p);
        out.nrm = parseInputBufferSet(stream, mdl0Size, EGXAttribute3::Normal, nrmDic,p);

        out.clr0 = parseInputBufferSet(stream, mdl0Size, EGXAttribute3::Color0, clrDic,p);
        out.clr1 = parseInputBufferSet(stream, mdl0Size, EGXAttribute3::Color1, clrDic,p);

        out.txc = parseInputBufferSet(stream, mdl0Size, EGXAttribute3::TexCoord0, txcDic,p);

        return out;
    }
    inline const bres::ResDicEntry* bres::FindResDicEntry(
        const std::vector<ResDicEntry>& dic,
        const std::string& name)
    {
        for (const auto& e : dic) {
            if (e.name == name)
                return &e;
        }
        return nullptr;
    }
    inline bool operator&(bres::NodeFlags a, bres::NodeFlags b) {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }


    bres::MDL0_ShapeEntry bres::parseMDL0_ShapeEntry(
        bStream::CStream* stream,
        const InputVertexBuffers& InputBuffer,
        size_t p,size_t ps
    ) {
        MDL0_ShapeEntry out{};
        stream->seek(p + 0x08);
        out.mtxIdx = stream->readInt32();

        stream->seek(p + 0x18);
        out.prePrimDLSize = stream->readUInt32();
        out.prePrimDLCmdSize = stream->readUInt32();
        out.prePrimDLOffs = 0x18 + stream->readUInt32();

        out.primDLSize = stream->readUInt32();
        out.primDLCmdSize = stream->readUInt32();
        out.primDLOffs = 0x24 + stream->readUInt32();

        uint32_t vcdFlags = stream->readUInt32();
        uint32_t flags = stream->readUInt32();
        uint32_t nameOffs = stream->readUInt32();

        size_t abs = p + nameOffs;
        size_t size = stream->getSize();
        if (abs >= size)
            printf("[ERROR] name string offset out of range\n");

        out.name = ReadString(stream, p + nameOffs);

        stream->seek(p + 0x3C);
        out.id = stream->readUInt32();
        out.numVertices = stream->readUInt32();
        out.numPolygons = stream->readUInt32();

        out.idVtxPos = stream->readInt16();
        out.idVtxNrm = stream->readInt16();
        out.idVtxClr0 = stream->readInt16();
        out.idVtxClr1 = stream->readInt16();
        out.idVtxTxc0 = stream->readInt16();
        out.idVtxTxc1 = stream->readInt16();
        out.idVtxTxc2 = stream->readInt16();
        out.idVtxTxc3 = stream->readInt16();
        out.idVtxTxc4 = stream->readInt16();
        out.idVtxTxc5 = stream->readInt16();
        out.idVtxTxc6 = stream->readInt16();
        out.idVtxTxc7 = stream->readInt16();
        out.idVtxFurVec = stream->readInt16();
        out.idVtxFurPos = stream->readInt16();
        out.mtxSetOffs = stream->readUInt32();

        DisplayListRegisters r{};
        displayListRegistersRun(stream, r, p + out.prePrimDLOffs, out.prePrimDLSize);

        uint32_t vcdL = r.cp[0x50];
        uint32_t vcdH = r.cp[0x60];

        std::array<GX_VtxDesc, 21> vcd{};
        auto setDesc = [&](int attr, uint8_t type) {
            vcd[attr].type = type;
        };

        setDesc((int)EGXAttribute::PositionMatrixIdx, (vcdL >> 0) & 0x01);
        setDesc((int)EGXAttribute::Tex0MatrixIdx, (vcdL >> 1) & 0x01);
        setDesc((int)EGXAttribute::Tex1MatrixIdx, (vcdL >> 2) & 0x01);
        setDesc((int)EGXAttribute::Tex2MatrixIdx, (vcdL >> 3) & 0x01);
        setDesc((int)EGXAttribute::Tex3MatrixIdx, (vcdL >> 4) & 0x01);
        setDesc((int)EGXAttribute::Tex4MatrixIdx, (vcdL >> 5) & 0x01);
        setDesc((int)EGXAttribute::Tex5MatrixIdx, (vcdL >> 6) & 0x01);
        setDesc((int)EGXAttribute::Tex6MatrixIdx, (vcdL >> 7) & 0x01);
        setDesc((int)EGXAttribute::Tex7MatrixIdx, (vcdL >> 8) & 0x01);
        setDesc((int)EGXAttribute::Position, (vcdL >> 9) & 0x03);
        setDesc((int)EGXAttribute::Normal, (vcdL >> 11) & 0x03);
        setDesc((int)EGXAttribute::Color0, (vcdL >> 13) & 0x03);
        setDesc((int)EGXAttribute::Color1, (vcdL >> 15) & 0x03);
        setDesc((int)EGXAttribute::TexCoord0, (vcdH >> 0) & 0x03);
        setDesc((int)EGXAttribute::TexCoord1, (vcdH >> 2) & 0x03);
        setDesc((int)EGXAttribute::TexCoord2, (vcdH >> 4) & 0x03);
        setDesc((int)EGXAttribute::TexCoord3, (vcdH >> 6) & 0x03);
        setDesc((int)EGXAttribute::TexCoord4, (vcdH >> 8) & 0x03);
        setDesc((int)EGXAttribute::TexCoord5, (vcdH >> 10) & 0x03);
        setDesc((int)EGXAttribute::TexCoord6, (vcdH >> 12) & 0x03);
        setDesc((int)EGXAttribute::TexCoord7, (vcdH >> 14) & 0x03);

        uint32_t vatA = r.cp[0x70 + 0];
        uint32_t vatB = r.cp[0x80 + 0];
        uint32_t vatC = r.cp[0x90 + 0];

        auto vatFmt = [&](uint8_t compCnt, uint8_t compType, uint8_t compShift) -> VATAttr {
            return VATAttr{ compCnt, compType, compShift };
        };

        std::array<VATAttr, 21> vat{};
        vat[(int)EGXAttribute::Position] = vatFmt((vatA >> 0) & 0x01, (vatA >> 1) & 0x07, (vatA >> 4) & 0x1F);

        bool nrm3 = !!(vatA >> 31);
        uint8_t nrmCnt = nrm3 ? 2 : ((vatA >> 9) & 0x01);
        vat[(int)EGXAttribute::Normal] = vatFmt(nrmCnt, (vatA >> 10) & 0x07, 0);
        vat[(int)EGXAttribute::Color0] = vatFmt((vatA >> 13) & 0x01, (vatA >> 14) & 0x07, 0);
        vat[(int)EGXAttribute::Color1] = vatFmt((vatA >> 17) & 0x01, (vatA >> 18) & 0x07, 0);
        vat[(int)EGXAttribute::TexCoord0] = vatFmt((vatA >> 21) & 0x01, (vatA >> 22) & 0x07, (vatA >> 25) & 0x1F);
        vat[(int)EGXAttribute::TexCoord1] = vatFmt((vatB >> 0) & 0x01, (vatB >> 1) & 0x07, (vatB >> 4) & 0x1F);
        vat[(int)EGXAttribute::TexCoord2] = vatFmt((vatB >> 9) & 0x01, (vatB >> 10) & 0x07, (vatB >> 13) & 0x1F);
        vat[(int)EGXAttribute::TexCoord3] = vatFmt((vatB >> 18) & 0x01, (vatB >> 19) & 0x07, (vatB >> 22) & 0x1F);
        vat[(int)EGXAttribute::TexCoord4] = vatFmt((vatB >> 27) & 0x01, (vatB >> 28) & 0x07, (vatC >> 0) & 0x1F);
        vat[(int)EGXAttribute::TexCoord5] = vatFmt((vatC >> 5) & 0x01, (vatC >> 6) & 0x07, (vatC >> 9) & 0x1F);
        vat[(int)EGXAttribute::TexCoord6] = vatFmt((vatC >> 14) & 0x01, (vatC >> 15) & 0x07, (vatC >> 18) & 0x1F);
        vat[(int)EGXAttribute::TexCoord7] = vatFmt((vatC >> 23) & 0x01, (vatC >> 24) & 0x07, (vatC >> 27) & 0x1F);

        std::array<GX_Array, 21> vtxArrays{};

        auto setArray = [&](int attr, int16_t id, const auto& srcVec) {
            if (id >= 0) {
                const VtxBufferData& buf = srcVec[id];
                vtxArrays[attr] = GX_Array{
                    buf.buffer.data(),
                    buf.stride,
                    buf.count,
                    0
                };
            }
        };

        setArray((int)EGXAttribute::Position, out.idVtxPos, InputBuffer.pos);
        setArray((int)EGXAttribute::Normal, out.idVtxNrm, InputBuffer.nrm);
        setArray((int)EGXAttribute::Color0, out.idVtxClr0, InputBuffer.clr0);
        setArray((int)EGXAttribute::Color1, out.idVtxClr1, InputBuffer.clr1);
        setArray((int)EGXAttribute::TexCoord0, out.idVtxTxc0, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord1, out.idVtxTxc1, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord2, out.idVtxTxc2, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord3, out.idVtxTxc3, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord4, out.idVtxTxc4, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord5, out.idVtxTxc5, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord6, out.idVtxTxc6, InputBuffer.txc);
        setArray((int)EGXAttribute::TexCoord7, out.idVtxTxc7, InputBuffer.txc);
const GX_Array& a0 = vtxArrays[(int)EGXAttribute::TexCoord0];
const GX_Array& a1 = vtxArrays[(int)EGXAttribute::TexCoord1];

        VCDInfo vcdInfo{};
        auto mapVcdBitsToAttr = [](uint32_t bits) -> VCDInfo::AttrType {
            return static_cast<VCDInfo::AttrType>(bits & 0x03);
        };

        vcdInfo.pos = mapVcdBitsToAttr((vcdL >> 9) & 0x03);
        vcdInfo.nrm = mapVcdBitsToAttr((vcdL >> 11) & 0x03);
        vcdInfo.clr0 = mapVcdBitsToAttr((vcdL >> 13) & 0x03);
        vcdInfo.clr1 = mapVcdBitsToAttr((vcdL >> 15) & 0x03);

        vcdInfo.tex[0] = mapVcdBitsToAttr((vcdH >> 0) & 0x03);
        vcdInfo.tex[1] = mapVcdBitsToAttr((vcdH >> 2) & 0x03);
        vcdInfo.tex[2] = mapVcdBitsToAttr((vcdH >> 4) & 0x03);
        vcdInfo.tex[3] = mapVcdBitsToAttr((vcdH >> 6) & 0x03);
        vcdInfo.tex[4] = mapVcdBitsToAttr((vcdH >> 8) & 0x03);
        vcdInfo.tex[5] = mapVcdBitsToAttr((vcdH >> 10) & 0x03);
        vcdInfo.tex[6] = mapVcdBitsToAttr((vcdH >> 12) & 0x03);
        vcdInfo.tex[7] = mapVcdBitsToAttr((vcdH >> 14) & 0x03);


        vcdInfo.pnmtx = (vcdL & 0x01) ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[0] = (vcdL >> 1) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[1] = (vcdL >> 2) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[2] = (vcdL >> 3) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[3] = (vcdL >> 4) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[4] = (vcdL >> 5) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[5] = (vcdL >> 6) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[6] = (vcdL >> 7) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;
        vcdInfo.texmtx[7] = (vcdL >> 8) & 0x01 ? VCDInfo::AttrType::DIRECT : VCDInfo::AttrType::NONE;

        VATInfo vatInfo{};
        vatInfo.pos = vat[(int)EGXAttribute::Position];
        vatInfo.nrm = vat[(int)EGXAttribute::Normal];
        vatInfo.clr0 = vat[(int)EGXAttribute::Color0];
        vatInfo.clr1 = vat[(int)EGXAttribute::Color1];
        for (int i = 0; i < 8; i++)
            vatInfo.tex[i] = vat[(int)EGXAttribute::TexCoord0 + i];

        VertexLayout layout = buildVertexLayout(vatInfo, vcdInfo);

        std::array<VCDInfo, 8> vcdInfos{};
        for (int i = 0; i < 8; i++)
            vcdInfos[i] = vcdInfo;

        ParsedDL dl = parseDisplayList(
            p + out.primDLOffs,
            stream,
            out.primDLSize,
            vcdInfos,
            vatInfo
        );

        LoadedVertexData vtx = runVertices(
            vatInfo,
            vcdInfos,
            vtxArrays,
            dl,
            layout,
            stream,
            p+out.primDLOffs,out.idVtxTxc0
        );

        out.indexBuffer = dl.indexBuffer;
        out.InputBuffers = InputBuffer;
        out.vtxdata = std::move(vtx);

        if (dl.totalVertexCount != out.numVertices)
            std::cout << "Vertex count mismatch" << dl.totalVertexCount << ":" << out.numVertices << "\n";
        return out;
    }




    bres::MDL0_NodeEntry bres::parseMDL0_NodeEntry(
        size_t entryPtr,
        size_t nodeBasePtr,
        size_t mdl0Base,
        bStream::CStream* stream
    )
    {
        stream->seek(entryPtr + 0x8);

        uint32_t nameOffs = stream->readUInt32();
        std::string name = ReadString(stream,entryPtr + nameOffs);
        stream->seek(entryPtr + 0xC);
        uint32_t id = stream->readUInt32();
        uint32_t mtxId = stream->readUInt32();
        NodeFlags flags = static_cast<NodeFlags>(stream->readUInt32());
        BillboardMode billboardMode = static_cast<BillboardMode>(stream->readUInt32());
        uint32_t billboardRefNodeId = stream->readUInt32();

        float scaleX = stream->readFloat();
        float scaleY = stream->readFloat();
        float scaleZ = stream->readFloat();
constexpr float DEG_TO_RAD = glm::pi<float>() / 180.0f;
float rotX = stream->readFloat() * DEG_TO_RAD;
float rotY = stream->readFloat() * DEG_TO_RAD;
float rotZ = stream->readFloat() * DEG_TO_RAD;
        float transX = stream->readFloat();
        float transY = stream->readFloat();
        float transZ = stream->readFloat();

        glm::mat4 modelMatrix = computeModelMatrixSRT(
            scaleX, scaleY, scaleZ,
            rotX, rotY, rotZ,
            transX, transY, transZ
        );

        float bboxMinX = stream->readFloat();
        float bboxMinY = stream->readFloat();
        float bboxMinZ = stream->readFloat();
        float bboxMaxX = stream->readFloat();
        float bboxMaxY = stream->readFloat();
        float bboxMaxZ = stream->readFloat();

        std::optional<AABB> bbox;
        if ((bboxMaxX - bboxMinX) > 0.0f)
            bbox = AABB(bboxMinX, bboxMinY, bboxMinZ, bboxMaxX, bboxMaxY, bboxMaxZ);

        int32_t toParentNode = stream->readInt32();
        int32_t toChildNode = stream->readInt32();
        int32_t toNextSibling = stream->readInt32();
        int32_t toPrevSibling = stream->readInt32();
        int32_t toResUserData = stream->readInt32();
        stream->seek(mdl0Base + 0x04);
        uint32_t userDataTableOffs = stream->readUInt32();
        std::optional<ResUserData> userData = ParseUserData(stream, userDataTableOffs, toResUserData);


        int parentNodeId = -1;
        if (toParentNode != 0) {
            size_t parentPtr = entryPtr + toParentNode;
            parentNodeId = int((parentPtr - nodeBasePtr) / 0xD0);
        }

        stream->seek(entryPtr + 0x70);
        float fb00 = stream->readFloat(), fb01 = stream->readFloat(), fb02 = stream->readFloat(), fb03 = stream->readFloat();
        float fb10 = stream->readFloat(), fb11 = stream->readFloat(), fb12 = stream->readFloat(), fb13 = stream->readFloat();
        float fb20 = stream->readFloat(), fb21 = stream->readFloat(), fb22 = stream->readFloat(), fb23 = stream->readFloat();

        glm::mat4 forwardBindPose = glm::mat4(
            fb00, fb10, fb20, 0.0f,
            fb01, fb11, fb21, 0.0f,
            fb02, fb12, fb22, 0.0f,
            fb03, fb13, fb23, 1.0f
        );

        float ib00 = stream->readFloat(), ib01 = stream->readFloat(), ib02 = stream->readFloat(), ib03 = stream->readFloat();
        float ib10 = stream->readFloat(), ib11 = stream->readFloat(), ib12 = stream->readFloat(), ib13 = stream->readFloat();
        float ib20 = stream->readFloat(), ib21 = stream->readFloat(), ib22 = stream->readFloat(), ib23 = stream->readFloat();

        glm::mat4 inverseBindPose = glm::mat4(
            ib00, ib10, ib20, 0.0f,
            ib01, ib11, ib21, 0.0f,
            ib02, ib12, ib22, 0.0f,
            ib03, ib13, ib23, 1.0f
        );

        bool visible =
            (static_cast<uint32_t>(flags) & static_cast<uint32_t>(NodeFlags::VISIBLE)) != 0;

        MDL0_NodeEntry out{
            name,
            id,
            mtxId,
            flags,
            billboardMode,
            billboardRefNodeId,
            modelMatrix,
            bbox,
            visible,
            parentNodeId,
            forwardBindPose,
            inverseBindPose,
            userData
        };
        out.scale     = glm::vec3(scaleX, scaleY, scaleZ);
        out.rotate    = glm::vec3(rotX, rotY, rotZ);
        out.translate = glm::vec3(transX, transY, transZ);

        return out;
    }
    std::vector<bres::NodeTreeOp> bres::parseMDL0_NodeTreeBytecode(bStream::CStream* stream, size_t size,size_t p)
    {
        std::vector<NodeTreeOp> ops;
        size_t i = 0;

        while (i < size) {
            stream->seek(p + i);
            ByteCodeOp op = static_cast<ByteCodeOp>(stream->readUInt8());

            if (op == ByteCodeOp::RET)
                break;

            else if (op == ByteCodeOp::NODEDESC) {
                uint16_t nodeId = stream->readUInt16();
                uint16_t parentMtxId = stream->readUInt16();
                i += 0x05;
                ops.emplace_back(NodeDescOp{ op, nodeId, parentMtxId });
            }

            else if (op == ByteCodeOp::MTXDUP) {
                uint16_t toMtxId = stream->readUInt16();
                uint16_t fromMtxId = stream->readUInt16();
                i += 0x05;
                ops.emplace_back(MtxDupOp{ op, toMtxId, fromMtxId });
            }

            else {
                throw std::runtime_error("Invalid NodeTree bytecode");
            }
        }

        return ops;
    }

    const bres::ResDicEntry* bres::findResDic(const std::vector<ResDicEntry>& dic, const std::string& name)
    {
        for (const auto& entry : dic) {
            if (entry.name == name)
                return &entry;
        }
        return nullptr;
    }
    std::vector<bres::NodeMixOp> bres::parseMDL0_NodeMixBytecode(bStream::CStream* stream, size_t size, size_t p)
    {
        std::vector<NodeMixOp> ops;
        size_t i = 0;

        while (i < size) {
            stream->seek(p + i);
            ByteCodeOp op = static_cast<ByteCodeOp>(stream->readUInt8());

            if (op == ByteCodeOp::RET)
                break;

            else if (op == ByteCodeOp::NODEMIX) {

                uint16_t dstMtxId = stream->readUInt16();
                uint8_t numBlend = stream->readUInt8();
                i += 0x04;

                std::vector<uint16_t> blendIds;
                std::vector<float> weights;
                stream->seek(p + i);
                for (int j = 0; j < numBlend; j++) {
                    uint16_t blendId = stream->readUInt16();
                    float weight = stream->readFloat();
                    i += 0x06;

                    blendIds.push_back(blendId);
                    weights.push_back(weight);
                }

                ops.emplace_back(NodeMixOp_{ op, dstMtxId, blendIds, weights });
            }

            else if (op == ByteCodeOp::EVPMTX) {
                uint16_t mtxId = stream->readUInt16();
                uint16_t nodeId = stream->readUInt16();
                i += 0x05;
                ops.emplace_back(EvpMtxOp{ op, mtxId, nodeId });
            }

            else {
                throw std::runtime_error("Invalid NodeMix bytecode");
            }
        }

        return ops;
    }
    std::vector<bres::DrawOp> bres::parseMDL0_DrawBytecode(bStream::CStream* stream, size_t size, size_t p)
    {
        std::vector<DrawOp> ops;
        size_t i = 0;

        while (i < size) {
            stream->seek(p + i);
            ByteCodeOp op = static_cast<ByteCodeOp>(stream->readUInt8());

            if (op == ByteCodeOp::RET)
                break;

            else if (op == ByteCodeOp::DRAW) {
                uint16_t matId = stream->readUInt16();
                uint16_t shpId = stream->readUInt16();
                uint16_t nodeId = stream->readUInt16();

                i += 0x08;
                ops.push_back({ matId, shpId, nodeId });
            }

            else {
                throw std::runtime_error("Invalid Draw bytecode");
            }
        }

        return ops;
    }
    bres::MDL0_SceneGraph bres::parseMDL0_SceneGraph(
        bStream::CStream* stream,
        size_t fileSize,
        const std::vector<ResDicEntry>& byteCodeResDic,size_t p)
    {
        MDL0_SceneGraph sg;

        auto* nodeTreeEntry = findResDic(byteCodeResDic, "NodeTree");
        if (!nodeTreeEntry) throw std::runtime_error("Missing NodeTree");
        sg.nodeTreeOps = parseMDL0_NodeTreeBytecode(stream,
            fileSize - nodeTreeEntry->offs,nodeTreeEntry->offs);

        if (auto* e = findResDic(byteCodeResDic, "NodeMix")) {
            sg.nodeMixOps = parseMDL0_NodeMixBytecode(stream,
                fileSize - e->offs,e->offs);
        }

        if (auto* e = findResDic(byteCodeResDic, "DrawOpa")) {
            sg.drawOpaOps = parseMDL0_DrawBytecode(stream,
                fileSize - e->offs, e->offs);
        }

        if (auto* e = findResDic(byteCodeResDic, "DrawXlu")) {
            sg.drawXluOps = parseMDL0_DrawBytecode(stream,
                fileSize - e->offs, e->offs);
        }
        return sg;
    }
uint32_t bres::directAttrSize(EGXAttribute3 attr, const VATAttr& fmt) {
    uint32_t compSize = 0;

    switch (fmt.compType) {
    case 0:
    case 1:
        compSize = 1;
        break;
    case 2:
    case 3: 
        compSize = 2;
        break;
    case 4: 
        compSize = 4;
        break;
    default:
        assert(false);
    }

    uint32_t compCount = 0;

    switch (attr) {
    case EGXAttribute3::Position:
        compCount = (fmt.compCnt == 0 ? 3 : 4);
        break;

    case EGXAttribute3::Normal:
        compCount = (fmt.compCnt + 1) * 3;
        break;

    case EGXAttribute3::Color0:
    case EGXAttribute3::Color1:
        compCount = 4;
        break;

    case EGXAttribute3::TexCoord0:
    case EGXAttribute3::TexCoord1:
    case EGXAttribute3::TexCoord2:
    case EGXAttribute3::TexCoord3:
    case EGXAttribute3::TexCoord4:
    case EGXAttribute3::TexCoord5:
    case EGXAttribute3::TexCoord6:
    case EGXAttribute3::TexCoord7:
        compCount = (fmt.compCnt == 0 ? 2 : 3);
        break;

    default:
        compCount = 1;
    }

    return compSize * compCount;
}

int bres::getIndexNumComponents(EGXAttribute3 attr, const VATAttr& fmt) {
    switch (attr) {
    case EGXAttribute3::Position:
    case EGXAttribute3::Normal:
        return 3;
    case EGXAttribute3::Color0:
    case EGXAttribute3::Color1:
        return 4;
    case EGXAttribute3::TexCoord0:
    case EGXAttribute3::TexCoord1:
    case EGXAttribute3::TexCoord2:
    case EGXAttribute3::TexCoord3:
    case EGXAttribute3::TexCoord4:
    case EGXAttribute3::TexCoord5:
    case EGXAttribute3::TexCoord6:
    case EGXAttribute3::TexCoord7:
        return 2;
    default:
        return 1;
    }
}

static bool isColorAttr(int attrIndex) {
    return attrIndex == 3 || attrIndex == 4;
}

uint32_t bres::getIndexNumComponents(int attrIndex, const VATAttr& fmt) {
    if (attrIndex == 2 && fmt.compCnt == 2)
        return 3;
    return 1;
}

uint32_t bres::getAttributeByteSizeRaw(int attrIndex, const VATAttr& fmt) {
    if (isColorAttr(attrIndex)) {
        switch (fmt.compType) {
        case 0: return 2;
        case 1: return 3;
        case 2: return 4;
        case 3: return 2;
        case 4: return 3;
        case 5: return 4;
        }
    }

    const uint32_t compCount = getIndexNumComponents(attrIndex, fmt);

    switch (fmt.compType) {
    case 0:  return 1 * compCount;
    case 1:  return 1 * compCount;
    case 2:  return 2 * compCount;
    case 3:  return 2 * compCount;
    case 4:  return 4 * compCount;
    default: return 0;
    }
}

uint32_t bres::computeSrcVertexSize(const VATInfo& vat, const VCDInfo& vcd) {
    uint32_t size = 0;
    struct AttrEntry {
        const char* name;
        VCDInfo::AttrType type;
        const VATAttr* fmt;
        int gxAttr;
    };

    AttrEntry attrs[21] = {
        { "PNMTX", (VCDInfo::AttrType)vcd.pnmtx, nullptr, (int)EGXAttribute3::PositionMatrixIdx },
        { "TEX0MTX", (VCDInfo::AttrType)vcd.texmtx[0], nullptr, (int)EGXAttribute3::Tex0MatrixIdx },
        { "TEX1MTX", (VCDInfo::AttrType)vcd.texmtx[1], nullptr, (int)EGXAttribute3::Tex1MatrixIdx },
        { "TEX2MTX", (VCDInfo::AttrType)vcd.texmtx[2], nullptr, (int)EGXAttribute3::Tex2MatrixIdx },
        { "TEX3MTX", (VCDInfo::AttrType)vcd.texmtx[3], nullptr, (int)EGXAttribute3::Tex3MatrixIdx },
        { "TEX4MTX", (VCDInfo::AttrType)vcd.texmtx[4], nullptr, (int)EGXAttribute3::Tex4MatrixIdx },
        { "TEX5MTX", (VCDInfo::AttrType)vcd.texmtx[5], nullptr, (int)EGXAttribute3::Tex5MatrixIdx },
        { "TEX6MTX", (VCDInfo::AttrType)vcd.texmtx[6], nullptr, (int)EGXAttribute3::Tex6MatrixIdx },
        { "TEX7MTX", (VCDInfo::AttrType)vcd.texmtx[7], nullptr, (int)EGXAttribute3::Tex7MatrixIdx },
        { "POS",   vcd.pos,   &vat.pos,  (int)EGXAttribute3::Position },
        { "NRM",   vcd.nrm,   &vat.nrm,  (int)EGXAttribute3::Normal },
        { "CLR0",  vcd.clr0,  &vat.clr0, (int)EGXAttribute3::Color0 },
        { "CLR1",  vcd.clr1,  &vat.clr1, (int)EGXAttribute3::Color1 },
        { "TEX0",  vcd.tex[0], &vat.tex[0], (int)EGXAttribute3::TexCoord0 },
        { "TEX1",  vcd.tex[1], &vat.tex[1], (int)EGXAttribute3::TexCoord1 },
        { "TEX2",  vcd.tex[2], &vat.tex[2], (int)EGXAttribute3::TexCoord2 },
        { "TEX3",  vcd.tex[3], &vat.tex[3], (int)EGXAttribute3::TexCoord3 },
        { "TEX4",  vcd.tex[4], &vat.tex[4], (int)EGXAttribute3::TexCoord4 },
        { "TEX5",  vcd.tex[5], &vat.tex[5], (int)EGXAttribute3::TexCoord5 },
        { "TEX6",  vcd.tex[6], &vat.tex[6], (int)EGXAttribute3::TexCoord6 },
        { "TEX7",  vcd.tex[7], &vat.tex[7], (int)EGXAttribute3::TexCoord7 },
    };

    for (int i = 0; i < 21; i++) {
        const auto& a = attrs[i];
        if (a.type == VCDInfo::AttrType::NONE)
            continue;
        if (a.fmt == nullptr && a.type == VCDInfo::AttrType::DIRECT) {
    size += 1;
    continue;
}


        const VATAttr& fmt = *a.fmt;
        uint32_t before = size;
        uint32_t bytes = 0;

        switch (a.type) {
        case VCDInfo::AttrType::DIRECT:
            bytes = getAttributeByteSizeRaw(a.gxAttr, fmt);
            break;
        case VCDInfo::AttrType::INDEX8:
            bytes = 1 * getIndexNumComponents(a.gxAttr, fmt);
            break;
        case VCDInfo::AttrType::INDEX16:
            bytes = 2 * getIndexNumComponents(a.gxAttr, fmt); 
            break;
        default:
            break;
        }

        size += bytes;
    }

    return size;
}



 bres::ParsedDL bres::parseDisplayList(
    size_t p,
    bStream::CStream* stream,
    uint32_t primDLSize,
    const std::array<VCDInfo, 8>& vcds,
    VATInfo& vat)
{
    ParsedDL out{};

    std::vector<DrawCall> drawCalls;
    std::vector<LoadedVertexDraw> draws;

    uint32_t totalVertexCount = 0;
    uint32_t totalIndexCount  = 0;
    uint32_t drawCallIdx      = 0;

    LoadedVertexDraw* currentDraw  = nullptr;
    LoadedVertexDraw* currentXfmem = nullptr;

    auto newDraw = [&](uint32_t indexOffset) {
        LoadedVertexDraw d{};
        d.indexOffset = indexOffset;
        d.indexCount  = 0;
        std::fill(std::begin(d.posMatrixTable), std::end(d.posMatrixTable), 0xFFFF);
        std::fill(std::begin(d.texMatrixTable), std::end(d.texMatrixTable), 0xFFFF);
        return d;
    };

    while (true) {
        if (drawCallIdx >= primDLSize)
            break;

        stream->seek(p + drawCallIdx);
        uint8_t cmd = stream->readUInt8();
        if (cmd == 0)
            break;

        switch (cmd) {
        case 0x20: { // LOAD_INDX_A
            currentDraw = nullptr;
            if (!currentXfmem) {
                draws.push_back(newDraw(totalIndexCount));
                currentXfmem = &draws.back();
            }

            uint16_t arrayIndex = stream->readUInt16();
            uint16_t addrLen    = stream->readUInt16();

            const uint32_t len  = (addrLen >> 12) + 1;
            const uint32_t addr = addrLen & 0x0FFF;

            const uint32_t memoryElemSize = 3 * 4;
            const uint32_t memoryBaseAddr = 0x0000;
            const uint32_t tableIndex     = (addr - memoryBaseAddr) / memoryElemSize;

            if (len != memoryElemSize)
                throw std::runtime_error("LOAD_INDX_A len mismatch");

            currentXfmem->posMatrixTable[tableIndex] = arrayIndex;
            drawCallIdx += 0x05;
            continue;
        }
        case 0x30: { // LOAD_INDX_C
            currentDraw = nullptr;
            if (!currentXfmem) {
                draws.push_back(newDraw(totalIndexCount));
                currentXfmem = &draws.back();
            }

            uint16_t arrayIndex = stream->readUInt16();
            uint16_t addrLen    = stream->readUInt16();

            const uint32_t len  = (addrLen >> 12) + 1;
            const uint32_t addr = addrLen & 0x0FFF;

            const uint32_t memoryElemSize = 3 * 4;
            const uint32_t memoryBaseAddr = 0x0078;
            const uint32_t tableIndex     = (addr - memoryBaseAddr) / memoryElemSize;

            if (len != memoryElemSize)
                throw std::runtime_error("LOAD_INDX_C len mismatch");

            currentXfmem->texMatrixTable[tableIndex] = arrayIndex;
            drawCallIdx += 0x05;
            continue;
        }
        case 0x28:
        case 0x38:
            drawCallIdx += 0x05;
            continue;
        }

        const uint8_t primType     = cmd & 0xF8;
        const uint8_t vertexFormat = cmd & 0x07;

        stream->seek(p + drawCallIdx + 0x1);
        uint16_t vertexCount = stream->readUInt16();

        drawCallIdx += 0x03;
        const uint32_t srcOffs = drawCallIdx;
        totalVertexCount += vertexCount;

        if (!currentDraw) {
            if (currentXfmem) {
                currentDraw = currentXfmem;
                currentXfmem = nullptr;
            } else {
                draws.push_back(newDraw(totalIndexCount));
                currentDraw = &draws.back();
            }
        }

        uint32_t indexCount = 0;
        switch (primType) {
        case 0x90: // TRIANGLES
            indexCount = vertexCount;
            break;
        case 0x80: // QUADS
        case 0x88: // QUAD_STRIP
            indexCount = (vertexCount * 6) / 4;
            break;
        case 0xA0: // TRIANGLE_FAN
        case 0x98: // TRIANGLE_STRIP
            indexCount = (vertexCount - 2) * 3;
            break;
        default:
            printf("Invalid DL cmd at offs 0x%04X: 0x%02X\n",
                   drawCallIdx - 0x03, cmd);
            goto end_parse;
        }

uint16_t di = static_cast<uint16_t>(currentDraw - draws.data());

DrawCall dc{};
dc.primType = primType;
dc.vertexFormat = vertexFormat;
dc.srcOffs = srcOffs;
dc.vertexCount = vertexCount;
dc.drawIndex = di;

drawCalls.push_back(dc);

currentDraw->indexCount += indexCount;
totalIndexCount += indexCount;

        const VCDInfo& vcdInfoForFmt = vcds[vertexFormat];
        uint32_t srcVertexSize       = computeSrcVertexSize(vat, vcdInfoForFmt);
        drawCallIdx += srcVertexSize * vertexCount;
    }

end_parse:
    out.drawCalls        = std::move(drawCalls);
    out.draws            = std::move(draws);
    out.totalVertexCount = totalVertexCount;
    out.totalIndexCount  = totalIndexCount;

    out.indexBuffer.resize(totalIndexCount);
    uint32_t indexDataIdx = 0;
    uint32_t baseVertex   = 0;

    for (const DrawCall& dc : out.drawCalls) {
        switch (dc.primType) {
        case 0x90:
            for (uint32_t i = 0; i < dc.vertexCount; i++)
                out.indexBuffer[indexDataIdx++] = baseVertex + i;
            break;
        case 0x80:
        case 0x88:
            for (uint32_t i = 0; i < dc.vertexCount; i += 4) {
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 0;
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 1;
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 2;
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 0;
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 2;
                out.indexBuffer[indexDataIdx++] = baseVertex + i + 3;
            }
            break;
        case 0x98:
            for (uint32_t i = 2; i < dc.vertexCount; i++) {
                out.indexBuffer[indexDataIdx++] = baseVertex + i - 2;
                out.indexBuffer[indexDataIdx++] = baseVertex + i - (~i & 1);
                out.indexBuffer[indexDataIdx++] = baseVertex + i - (i & 1);
            }
            break;
        case 0xA0:
            for (uint32_t i = 2; i < dc.vertexCount; i++) {
                out.indexBuffer[indexDataIdx++] = baseVertex + 0;
                out.indexBuffer[indexDataIdx++] = baseVertex + i - 1;
                out.indexBuffer[indexDataIdx++] = baseVertex + i;
            }
            break;
        }
        baseVertex += dc.vertexCount;
    }

    return out;
}
    bres::MDL0 bres::parseMDL0(bStream::CStream* stream, size_t fileSize, size_t p)
    {
        stream->seek(p);
        uint32_t MDL0pos = stream->tell();
        std::string magic = stream->readString(4);
        if (magic != "MDL0")
           printf("MDL0 Magic was not detected,it occurs crash on CTGP-Revolution!\n");
        stream->seek(p + 0x08);
        uint32_t version = stream->readUInt32();
        if (!(version == 0x08 || version == 0x09 || version == 0x0A || version == 0x0B))
            assert("Unsupported MDL0 version");

uint32_t offs = 0x10;

auto nextResDic = [&]() -> std::vector<ResDicEntry> {
    if ((p + stream->peekUInt32(p + offs)) == MDL0pos)
    {
        offs += 0x04;
        return {};
    }
    else
    {
        auto resDic = ParseResDic(stream, p + stream->peekUInt32(p + offs));
        offs += 0x04;
        return resDic;
    }
};
        auto byteCodeResDic = nextResDic();
        auto nodeResDic = nextResDic();
        auto vtxPosResDic = nextResDic();
        auto vtxNrmResDic = nextResDic();
        auto vtxClrResDic = nextResDic();
        auto vtxTxcResDic = nextResDic();

        if (version >= 0x0A) {
            auto furVecResDic = nextResDic();
            auto furPosResDic = nextResDic();
        }

        auto materialResDic = nextResDic();
        auto tevResDic = nextResDic();
        auto shpResDic = nextResDic();

        offs += 0x04;
        offs += 0x04;

        if (version >= 0x0B)
            offs += 0x04; 

        stream->seek(p + offs);
        uint32_t nameOffs = stream->readUInt32();
        std::string name = ReadString(stream,p + nameOffs);

        size_t infoOffs = offs + 0x04;
        stream->seek(p + infoOffs + 0x08);
        uint32_t scalingRule = stream->readUInt32();
        uint32_t texMtxMode = stream->readUInt32();
        uint32_t numVerts = stream->readUInt32();
        uint32_t numPolygons = stream->readUInt32();

        stream->seek(p + infoOffs + 0x1C);
        uint32_t numViewMtx = stream->readUInt32();
        bool needNrmMtxArray = stream->readUInt8() != 0;
        bool needTexMtxArray = stream->readUInt8() != 0;
        bool isValidBBox = stream->readUInt8() != 0;

        stream->seek(p + infoOffs + 0x24);
        uint32_t mtxIdToNodeIdRel = stream->readUInt32();
        size_t mtxIdToNodeIdOffs = infoOffs + mtxIdToNodeIdRel;

        stream->seek(p + mtxIdToNodeIdOffs);
        uint32_t numWorldMtx = stream->readUInt32();

        std::vector<int32_t> mtxIdToNodeId(numWorldMtx);
        for (uint32_t i = 0; i < numWorldMtx; i++) {
            stream->seek(p + mtxIdToNodeIdOffs + 0x04 + i * 4);
            mtxIdToNodeId[i] = stream->readUInt32();
        }

        std::optional<AABB> bbox;
        if (isValidBBox) {
            stream->seek(p + infoOffs + 0x28);
            float minX = stream->readFloat();
            float minY = stream->readFloat();
            float minZ = stream->readFloat();
            float maxX = stream->readFloat();
            float maxY = stream->readFloat();
            float maxZ = stream->readFloat();
            bbox = AABB(minX, minY, minZ, maxX, maxY, maxZ);
        }

        std::vector<MDL0_MaterialEntry> materials;

        for (auto& e : materialResDic)
        {
                auto mat = ParseMDL0_MaterialEntry(stream, version, e.offs);
                assert(mat.name == e.name);
                materials.push_back(mat);
        }


        MDL0 mdl;
        mdl.name = name;
        mdl.materials = std::move(materials);
        std::vector<MDL0_ShapeEntry> shapes;
        mdl.vtxPosResDic = vtxPosResDic;
        mdl.vtxTxcResDic = vtxTxcResDic;
        mdl.vtxNrmResDic = vtxNrmResDic;
        mdl.vtxClrResDic = vtxClrResDic;
InputVertexBuffers inputBuffers =
            parseInputVertexBuffers(
                stream,
                mdl.fileSize,
                mdl.vtxPosResDic,
                mdl.vtxNrmResDic,
                mdl.vtxClrResDic,
                mdl.vtxTxcResDic,p);

        for (int i = 0; i < shpResDic.size(); i++) {
            auto& e = shpResDic[i];
                auto shape = parseMDL0_ShapeEntry(stream, inputBuffers, e.offs, MDL0pos);
                assert(shape.name == e.name);
                shapes.push_back(std::move(shape));
        }


        std::vector<MDL0_NodeEntry> nodes;
        for (auto& e : nodeResDic) {
            if (MDL0pos != e.offs)
            {
                auto node = parseMDL0_NodeEntry(e.offs, nodeResDic[0].offs, p, stream);
                nodes.push_back(node);
            }
        }
        auto sceneGraph = parseMDL0_SceneGraph(stream, fileSize, byteCodeResDic,p);

for (auto& op : sceneGraph.drawOpaOps) {
    auto& mat = mdl.materials[op.matId];

    if (mat.primaryShapeIdx < 0) {
        mat.primaryShapeIdx = op.shpId;
    }
}

for (auto& op : sceneGraph.drawXluOps) {
    auto& mat = mdl.materials[op.matId];

    if (mat.primaryShapeIdx < 0) {
        mat.primaryShapeIdx = op.shpId;
    }
}
        mdl.bbox = std::move(bbox);
        mdl.shapes = std::move(shapes);
        mdl.nodes = std::move(nodes);
        mdl.sceneGraph = std::move(sceneGraph);
        mdl.numWorldMtx = std::move(numWorldMtx);
        mdl.numViewMtx = std::move(numViewMtx);
        mdl.needNrmMtxArray = std::move(needNrmMtxArray);
        mdl.needTexMtxArray = std::move(needTexMtxArray);
        mdl.mtxIdToNodeId = std::move(mtxIdToNodeId);
        mdl.inputBuffers = std::move(inputBuffers);
        return mdl;
    }
    float bres::SampleFloatAnimationTrackLinear(const FloatTrackLinear& track, float frame) {
        const auto& frames = track.frames;
        int n = static_cast<int>(frames.size());

        if (n == 1)
            return frames[0];

        if (frame == 0.0f)
            return frames[0];
        else if (frame > n - 1)
            return frames[n - 1];

        int idx0 = static_cast<int>(frame);
        float k0 = frames[idx0];
        int idx1 = idx0 + 1;
        float k1 = frames[idx1];

        float t = frame - idx0;
        return LerpPeriodic(k0, k1, t);
    }
    inline float bres::GetPointHermite(float p0, float p1, float s0, float s1, float t) {
        float t2 = t * t;
        float t3 = t2 * t;

        return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 +
            (t3 - 2.0f * t2 + t) * s0 +
            (-2.0f * t3 + 3.0f * t2) * p1 +
            (t3 - t2) * s1;
    }

    float bres::HermiteInterpolate(const FloatKeyHermite& k0, const FloatKeyHermite& k1, float frame) {
        float length = k1.frame - k0.frame;
        float t = (frame - k0.frame) / length;
        float p0 = k0.value;
        float p1 = k1.value;
        float s0 = k0.tangent * length;
        float s1 = k1.tangent * length;
        return GetPointHermite(p0, p1, s0, s1, t);
    }
    float bres::SampleFloatAnimationTrackHermite(const FloatTrackHermite& track, float frame) {
        const auto& frames = track.frames;
        int n = static_cast<int>(frames.size());

        if (n == 1)
            return frames[0].value;

        int idx1 = 0;
        for (; idx1 < n; idx1++) {
            if (frame < frames[idx1].frame)
                break;
        }

        if (idx1 == 0)
            return frames[0].value;
        else if (idx1 == n)
            return frames[n - 1].value;

        int idx0 = idx1 - 1;
        const auto& k0 = frames[idx0];
        const auto& k1 = frames[idx1];

        return HermiteInterpolate(k0, k1, frame);
    }
    float bres::SampleFloatTrack(const FloatAnimationTrack& track, float frame)
    {
        return std::visit([&](auto&& t) -> float {
            using T = std::decay_t<decltype(t)>;

            if constexpr (std::is_same_v<T, FloatTrackLinear>) {
                return SampleFloatAnimationTrackLinear(t, frame);
            }
            else if constexpr (std::is_same_v<T, FloatTrackHermite>) {
                return SampleFloatAnimationTrackHermite(t, frame);
            }
        }, track);
    }

    float bres::GetAnimFrame(const AnimationBase& anim, float frame) {
        float last = anim.duration;

        if (anim.loopMode == LoopMode::ONCE) {
            return std::min(frame, last);
        }
        else {
            while (last > 0 && frame > last)
                frame -= last;
            return frame;
        }
    }
    float bres::Hermite(float p0, float p1, float s0, float s1, float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        return (2 * t3 - 3 * t2 + 1) * p0 +
            (t3 - 2 * t2 + t) * s0 +
            (-2 * t3 + 3 * t2) * p1 +
            (t3 - t2) * s1;
    }
    bool bres::SampleAnimationTrackBoolean(const BitMap& frames, float animFrame) {
        if (frames.numBits == 1)
            return frames.getBit(0);

        return frames.getBit(int(animFrame));
    }
    bres::FloatAnimationTrack bres::ParseAnimationTrackC8(bStream::CStream* stream, int numKeyframes, size_t p) {
        stream->seek(p);
        float scale = stream->readFloat();
        float bias = stream->readFloat();

        std::vector<float> frames(numKeyframes + 1);
        int idx = 0x08;

        for (int i = 0; i < numKeyframes + 1; i++) {
            stream->seek(p + idx);
            frames[i] = stream->readUInt8() * scale + bias;
            idx += 1;
        }

        return FloatTrackLinear{ AnimationTrackType::LINEAR, frames };
    }
    bres::FloatAnimationTrack bres::ParseAnimationTrackC16(bStream::CStream* stream,int numKeyframes, size_t p) {
        stream->seek(p);
        float scale = stream->readFloat();
        float bias = stream->readFloat();

        std::vector<float> frames(numKeyframes + 1);
        int idx = 0x08;

        for (int i = 0; i < numKeyframes + 1; i++) {
            stream->seek(p+ idx);
            frames[i] = stream->readUInt16() * scale + bias;
            idx += 2;
        }

        return FloatTrackLinear{ AnimationTrackType::LINEAR, frames };
    }
    bres::FloatAnimationTrack bres::ParseAnimationTrackC32(bStream::CStream* stream, int numKeyframes, size_t p) {
        std::vector<float> frames(numKeyframes + 1);
        for (int i = 0; i < numKeyframes + 1; i++)
        {
            stream->seek(p + i * 4);
            frames[i] = stream->readFloat();
        }

        return FloatTrackLinear{ AnimationTrackType::LINEAR, frames };
    }
    bres::FloatAnimationTrack bres::ParseAnimationTrackF32(bStream::CStream* stream, size_t p)
    {
        stream->seek(p);

        uint16_t numKeyframes = stream->readUInt16();
        float    invKeyframeRange = stream->readFloat();
        float    scale = stream->readFloat();
        float    offset = stream->readFloat();

        size_t idx = 0x10;

        FloatTrackHermite hermite;
        hermite.type = AnimationTrackType::HERMITE;
        hermite.frames.reserve(numKeyframes);

        for (uint16_t i = 0; i < numKeyframes; i++) {
            stream->seek(p + idx);

            uint8_t  frameU8 = stream->readUInt8();
            uint16_t valueU16 = stream->readUInt16();
            int16_t  tanS16 = stream->readInt16();

            float frame = static_cast<float>(frameU8);
            float value = (static_cast<uint16_t>(valueU16 >> 4)) * scale + offset;
            float tangent = static_cast<float>((tanS16 << 20) >> 20) / 0x20f;

            hermite.frames.push_back({ frame, value, tangent });

            idx += 0x04;
        }

        return FloatAnimationTrack{ std::move(hermite) };
    }

    bres::FloatAnimationTrack bres::ParseAnimationTrackF48(bStream::CStream* stream, size_t p)
    {
        stream->seek(p);

        uint16_t numKeyframes = stream->readUInt16();
        float    invKeyframeRange = stream->readFloat();
        float    scale = stream->readFloat();
        float    offset = stream->readFloat();

        size_t idx = 0x10;

        FloatTrackHermite hermite;
        hermite.type = AnimationTrackType::HERMITE;
        hermite.frames.reserve(numKeyframes);

        for (uint16_t i = 0; i < numKeyframes; i++) {
            stream->seek(p + idx);

            int16_t frameS10_5 = stream->readInt16();
            uint16_t valueU16 = stream->readUInt16();
            int16_t tanS7_8 = stream->readInt16();

            float frame = static_cast<float>(frameS10_5) / 0x20f;
            float value = static_cast<float>(valueU16) * scale + offset;
            float tangent = static_cast<float>(tanS7_8) / 0x100f;

            hermite.frames.push_back({ frame, value, tangent });

            idx += 0x06;
        }

        return FloatAnimationTrack{ std::move(hermite) };
    }


    bres::FloatAnimationTrack bres::ParseAnimationTrackF96(bStream::CStream* stream, size_t p)
    {
        stream->seek(p);

        uint16_t numKeyframes = stream->readUInt16();
        float invKeyframeRange = stream->readFloat();

        size_t idx = 0x08;
        std::vector<FloatKeyHermite> frames;
        frames.reserve(numKeyframes);

        for (uint16_t i = 0; i < numKeyframes; i++) {
            stream->seek(p + idx);

            float frame = stream->readFloat();
            float value = stream->readFloat();
            float tangent = stream->readFloat();

            idx += 0x0C;

            frames.push_back({ frame, value, tangent });
        }

        return FloatTrackHermite{ AnimationTrackType::HERMITE, frames };
    }

    bres::FloatAnimationTrack bres::MakeConstantAnimationTrack(float value) {
        FloatTrackLinear t;
        t.type = AnimationTrackType::LINEAR;
        t.frames = { value };
        return t;
    }
    bres::FloatAnimationTrack bres::ParseAnimationTrackF96OrConst(
        bStream::CStream* stream,
        bool isConstant,
        size_t offsPos)
    {
        stream->seek(offsPos);

        if (isConstant) {
            float value = stream->readFloat();
            return MakeConstantAnimationTrack(value);
        }
        else {
            uint32_t offs = stream->readUInt32();
            size_t trackBase = offsPos + offs;
            return ParseAnimationTrackF96(stream, trackBase);
        }
    }


    float bres::SampleFloatAnimationTrack(const FloatAnimationTrack& track, float frame) {
        if (std::holds_alternative<FloatTrackLinear>(track)) {
            return SampleFloatAnimationTrackLinear(std::get<FloatTrackLinear>(track), frame);
        }
        else {
            return SampleFloatAnimationTrackHermite(std::get<FloatTrackHermite>(track), frame);
        }
    }
    bres::SRT0_TexData bres::ParseSRT0_TexData(bStream::CStream* stream, size_t p) {
        stream->seek(p);

        enum Flags : uint32_t {
            SCALE_ONE = 0x002,
            ROT_ZERO = 0x004,
            TRANS_ZERO = 0x008,
            SCALE_UNIFORM = 0x010,
            SCALE_S_CONSTANT = 0x020,
            SCALE_T_CONSTANT = 0x040,
            ROT_CONSTANT = 0x080,
            TRANS_S_CONSTANT = 0x100,
            TRANS_T_CONSTANT = 0x200,
        };

        uint32_t flags = stream->readUInt32();

        SRT0_TexData out{};
        size_t animationTableIdx = 0x04;

        auto nextTrack = [&](bool isConst) -> FloatAnimationTrack {
            size_t offsPos = p + animationTableIdx;
            animationTableIdx += 0x04;
            return ParseAnimationTrackF96OrConst(stream, isConst, offsPos);
        };


        if (!(flags & SCALE_ONE))
            out.scaleS = nextTrack(flags & SCALE_S_CONSTANT);

        if (!(flags & SCALE_UNIFORM))
            out.scaleT = nextTrack(flags & SCALE_T_CONSTANT);
        else
            out.scaleT = out.scaleS;

        if (!(flags & ROT_ZERO))
            out.rotation = nextTrack(flags & ROT_CONSTANT);

        if (!(flags & TRANS_ZERO)) {
            out.translationS = nextTrack(flags & TRANS_S_CONSTANT);
            out.translationT = nextTrack(flags & TRANS_T_CONSTANT);
        }

        return out;
    }
    bres::SRT0_MatData bres::ParseSRT0_MatData(bStream::CStream* stream, size_t p) {
        stream->seek(p);
        uint32_t materialNameOffs = stream->readUInt32();
        std::string materialName = ReadString(stream,p+materialNameOffs);
        stream->seek(p + 0x04);
        uint32_t texFlags = stream->readUInt32();
        uint32_t indFlags = stream->readUInt32();
        uint32_t flags = (indFlags << 8) | texFlags;

        size_t texAnimationTableIdx = 0x0C;
        std::vector<SRT0_TexData> texAnimations(static_cast<size_t>(TexMtxIndex::COUNT));

        for (uint32_t i = 0; i < static_cast<uint32_t>(TexMtxIndex::COUNT); i++) {
            if (!(flags & (1u << i)))
                continue;
            stream->seek(p + texAnimationTableIdx);
            uint32_t texAnimationOffs = stream->readUInt32();
            texAnimationTableIdx += 0x04;

            SRT0_TexData texData = ParseSRT0_TexData(stream, p+ texAnimationOffs);
            texAnimations[i] = std::move(texData);
        }

        SRT0_MatData out{};
        out.materialName = std::move(materialName);
        out.texAnimations = std::move(texAnimations);
        return out;
    }
    bres::SRT0 bres::ParseSRT0(bStream::CStream* stream, size_t size, size_t p) {
        stream->seek(p + 0x08);
        uint32_t version = stream->readUInt32();
        assert(version == 0x04 || version == 0x05);

        stream->seek(p + 0x10);
        uint32_t texSrtMatDataResDicOffs = stream->readUInt32();
        auto texSrtMatDataResDic = ParseResDic(stream, p+texSrtMatDataResDicOffs);

        size_t offs = 0x14;
        if (version >= 0x05)
            offs += 0x04;

        stream->seek(p + offs);
        uint32_t nameOffs = stream->readUInt32();
        std::string name = ReadString(stream,p + nameOffs);

        stream->seek(p + offs+0x08);
        uint16_t duration = stream->readUInt16();
        uint16_t numMaterials = stream->readUInt16();
        TexMatrixMode texMtxMode = static_cast<TexMatrixMode>(stream->readUInt32());
        LoopMode loopMode = static_cast<LoopMode>(stream->readUInt32());

        std::vector<SRT0_MatData> matAnimations;
        matAnimations.reserve(numMaterials);

        for (const auto& e : texSrtMatDataResDic) {
            SRT0_MatData matData = ParseSRT0_MatData(stream,e.offs);
            matAnimations.push_back(std::move(matData));
        }

        assert(matAnimations.size() == numMaterials);

        SRT0 out{};
        out.name = std::move(name);
        out.duration = static_cast<float>(duration);
        out.loopMode = loopMode;
        out.texMtxMode = texMtxMode;
        out.matAnimations = std::move(matAnimations);
        return out;
    }
    std::vector<bres::PAT0_TexFrameData> bres::ParsePAT0_TexFrameTrack(bStream::CStream* stream, size_t size, size_t p) {
        stream->seek(p);
        uint16_t numKeyframes = stream->readUInt16();
        float invKeyframeRange = stream->readFloat();

        size_t idx = 0x08;
        std::vector<PAT0_TexFrameData> frames;
        frames.reserve(numKeyframes);

        for (uint16_t i = 0; i < numKeyframes; i++) {
            stream->seek(p + idx);
            float frame = stream->readFloat();
            uint16_t texIndex = stream->readUInt16();
            uint16_t palIndex = stream->readUInt16();
            idx += 0x08;

            frames.push_back({ frame, texIndex, palIndex });
        }

        return frames;
    }
    bres::PAT0_MatData bres::ParsePAT0_MatData(bStream::CStream* stream, size_t p) {
        stream->seek(p);
        uint32_t materialNameOffs = stream->readUInt32();
        std::string materialName = ReadString(stream,p + materialNameOffs);

        stream->seek(p+0x04);
        uint32_t flags = stream->readUInt32();

        enum Flags : uint32_t {
            EXISTS = 1 << 0,
            CONSTANT = 1 << 1,
            TEX_EXISTS = 1 << 2,
            PAL_EXISTS = 1 << 3,
        };

        size_t animationTableIdx = 0x08;
        auto nextTrack = [&](bool isConst) -> std::vector<PAT0_TexFrameData> {
            stream->seek(p + animationTableIdx);
            if (isConst) {
                uint16_t texIndex = stream->readUInt16();
                uint16_t palIndex = stream->readUInt16();
                animationTableIdx += 0x04;
                return { { 0.0f, texIndex, palIndex } };
            }
            else {
                uint32_t offs = stream->readUInt32();
                animationTableIdx += 0x04;
                return ParsePAT0_TexFrameTrack(stream,0,p+offs);
            }
        };

        std::vector<PAT0_TexData> texAnimations(8);
        for (int i = 0; i < 8; i++) {
            uint32_t texFlags = (flags >> (i * 4)) & 0x0F;
            if (!(texFlags & EXISTS))
                continue;

            bool texIndexValid = texFlags & TEX_EXISTS;
            bool palIndexValid = texFlags & PAL_EXISTS;
            bool isConstant = texFlags & CONSTANT;

            assert(texIndexValid && !palIndexValid);

            texAnimations[i] = {
                nextTrack(isConstant),
                texIndexValid,
                palIndexValid
            };
        }

        return { materialName, texAnimations };
    }
    bres::PAT0 bres::ParsePAT0(bStream::CStream* stream,  size_t p) {
        stream->seek(p + 0x08);
        uint32_t version = stream->readUInt32();
        assert(version == 0x03 || version == 0x04);

        stream->seek(p + 0x10);
        uint32_t matResDicOffs = stream->readUInt32();
        auto matResDic = ParseResDic(stream,p+matResDicOffs);

        stream->seek(p + 0x14);
        uint32_t texNameTableOffs = stream->readUInt32();
        uint32_t palNameTableOffs = stream->readUInt32();

        size_t offs = 0x24;
        if (version >= 0x04)
            offs += 0x04;

        stream->seek(p + offs);
        std::string name = ReadString(stream,p + stream->readUInt32());

        stream->seek(p + offs + 0x08);
        uint16_t duration = stream->readUInt16();
        uint16_t numMaterials = stream->readUInt16();
        uint16_t numTexNames = stream->readUInt16();
        uint16_t numPalNames = stream->readUInt16();
        LoopMode loopMode = static_cast<LoopMode>(stream->readUInt32());

        assert(numPalNames == 0);

        std::vector<PAT0_MatData> matAnimations;
        for (const auto& entry : matResDic) {
            matAnimations.push_back(ParsePAT0_MatData(stream,entry.offs));
        }
        assert(matAnimations.size() == numMaterials);

        std::vector<std::string> texNames;
        size_t texNameIdx = texNameTableOffs;
        for (int i = 0; i < numTexNames; i++) {
            stream->seek(p + texNameIdx);
            uint32_t nameOffs = stream->readUInt32();
            texNames.push_back(ReadString(stream,p + texNameTableOffs + nameOffs));
            texNameIdx += 0x04;
        }

        PAT0 out{};
        out.name = std::move(name);
        out.duration = static_cast<float>(duration);
        out.loopMode = loopMode;
        out.matAnimations = std::move(matAnimations);
        out.texNames = std::move(texNames);
        return out;
    }
    std::vector<uint32_t> bres::ParseAnimationTrackColor(
    bStream::CStream* stream,
    uint16_t numKeyframes,
    bool isConst,
    size_t p)
{
    size_t oldPos = stream->tell();
    stream->seek(p);

    std::vector<uint32_t> frames;

    if (isConst) {
        frames.push_back(stream->readUInt32());
    } else {
        uint32_t offs = stream->readUInt32();
        size_t base = p + offs;

        frames.resize(numKeyframes + 1);
        for (int i = 0; i < numKeyframes + 1; i++) {
            stream->seek(base + i * 4);
            frames[i] = stream->readUInt32();
        }
    }

    stream->seek(oldPos);
    return frames;
}


uint32_t bres::SampleAnimationTrackColor(const std::vector<uint32_t>& frames, float frame)
{
    int n = (int)frames.size();
    if (n == 0)
        return 0;

    if (n == 1)
        return frames[0];

    int idx = (int)std::round(frame);
    if (idx < 0) idx = 0;
    if (idx >= n) idx = n - 1;
    return frames[idx];
}

    inline uint32_t bres::ColorToRGBA8(const Color& c) {
        auto clamp01 = [](float v) {
            return std::max(0.0f, std::min(1.0f, v));
        };
        uint32_t r = static_cast<uint32_t>(std::round(clamp01(c.r) * 255.0f));
        uint32_t g = static_cast<uint32_t>(std::round(clamp01(c.g) * 255.0f));
        uint32_t b = static_cast<uint32_t>(std::round(clamp01(c.b) * 255.0f));
        uint32_t a = static_cast<uint32_t>(std::round(clamp01(c.a) * 255.0f));
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    inline void ColorFromRGBA8(Color& dst, uint32_t rgba) {
        auto ch = [&](int shift) -> float {
            return float((rgba >> shift) & 0xFF) / 255.0f;
        };
        dst.r = ch(24);
        dst.g = ch(16);
        dst.b = ch(8);
        dst.a = ch(0);
    }
    bres::CLR0_MatData bres::ParseCLR0_MatData(bStream::CStream* stream, uint16_t numKeyframes,size_t p) {
        stream->seek(p);
        uint32_t materialNameOffs = stream->readUInt32();
        std::string materialName = ReadString(stream,p + materialNameOffs);

        stream->seek(p+0x04);
        uint32_t flags = stream->readUInt32();

        enum Flags : uint32_t {
            EXISTS = 1 << 0,
            CONSTANT = 1 << 1,
        };

        size_t animationTableIdx = 0x08;

        auto nextColorData = [&](bool isConst) -> CLR0_ColorData {
            stream->seek(p + animationTableIdx);
            uint32_t mask = stream->readUInt32();
            #ifdef _WIN32
mask = _byteswap_ulong(mask); 
#else
mask = __builtin_bswap32(mask);
#endif
            std::vector<uint32_t> frames = ParseAnimationTrackColor(stream, numKeyframes, isConst, p + animationTableIdx + 0x04);
            animationTableIdx += 0x08;

            return { mask, std::move(frames) };
        };

        std::vector<CLR0_ColorData> clrAnimations(static_cast<size_t>(AnimatableColor::COUNT));

        for (size_t i = 0; i < static_cast<size_t>(AnimatableColor::COUNT); i++) {
            uint32_t clrFlags = (flags >> (i * 2)) & 0x03;
            if (!(clrFlags & EXISTS))
                continue;

            bool isConst = (clrFlags & CONSTANT) != 0;
            clrAnimations[i] = nextColorData(isConst);
        }

        return { std::move(materialName), std::move(clrAnimations) };
    }
const bres::CLR0_ColorData* bres::FindAnimationData_CLR0(
    const CLR0& clr0,
    const std::string& materialName,
    AnimatableColor target)
{
    for (auto& mat : clr0.matAnimations) {
        if (mat.materialName == materialName) {

            const auto& arr = mat.clrAnimations;
            size_t idx = (size_t)target;

            if (idx >= arr.size())
                return nullptr;

            const auto& data = arr[idx];

            if (data.frames.empty())
                return nullptr;


            return &data;
        }
    }
    return nullptr;
}

    VCDInfo bres::buildVCD(uint32_t vcdL, uint32_t vcdH) {
        auto decode = [&](int shift) -> VCDInfo::AttrType {
            uint32_t t = (shift < 32) ? ((vcdL >> shift) & 0x03)
                : ((vcdH >> (shift - 32)) & 0x03);
            switch (t) {
            case 0: return VCDInfo::NONE;
            case 1: return VCDInfo::NONE;
            case 2: return VCDInfo::INDEX8;
            case 3: return VCDInfo::INDEX16;
            }
            return VCDInfo::NONE;
        };

        VCDInfo out{};
        out.pos = decode(9);
        out.nrm = decode(11);
        out.clr0 = decode(13);
        out.clr1 = decode(15);

        out.tex[0] = decode(32 + 0);
        out.tex[1] = decode(32 + 2);
        out.tex[2] = decode(32 + 4);
        out.tex[3] = decode(32 + 6);
        out.tex[4] = decode(32 + 8);
        out.tex[5] = decode(32 + 10);
        out.tex[6] = decode(32 + 12);
        out.tex[7] = decode(32 + 14);

        return out;
    }
    bres::CLR0 bres::ParseCLR0(bStream::CStream* stream,  size_t p) {
        stream->seek(p + 0x08);
        uint32_t version = stream->readUInt32();
        assert(version == 0x03 || version == 0x04);

        stream->seek(p + 0x10);
        uint32_t matResDicOffs = stream->readUInt32();
        auto matResDic = ParseResDic(stream,p+ matResDicOffs);

        size_t offs = 0x14;
        if (version >= 0x04)
            offs += 0x04;
        stream->seek(p + offs);
        std::string name = ReadString(stream,p + stream->readUInt32());

        stream->seek(p + offs+0x08);
        uint16_t duration = stream->readUInt16();
        uint16_t numMaterials = stream->readUInt16();
        LoopMode loopMode = static_cast<LoopMode>(stream->readUInt32());

        std::vector<CLR0_MatData> matAnimations;
        matAnimations.reserve(numMaterials);

        for (const auto& entry : matResDic) {
            matAnimations.push_back(
                ParseCLR0_MatData(stream, duration,entry.offs)
            );
        }

        assert(matAnimations.size() == numMaterials);

        CLR0 out{};
        out.name = std::move(name);
        out.duration = static_cast<float>(duration);
        out.loopMode = loopMode;
        out.matAnimations = std::move(matAnimations);
        return out;
    }
    bres::CHR0_NodeData bres::ParseCHR0_NodeData(bStream::CStream* stream, uint16_t numKeyframes, size_t p)
    {
        enum Flags : uint32_t {
            IDENTITY = (1 << 1),
            RT_ZERO = (1 << 2),
            SCALE_ONE = (1 << 3),
            SCALE_UNIFORM = (1 << 4),
            ROTATE_ZERO = (1 << 5),
            TRANS_ZERO = (1 << 6),
            SCALE_USE_MODEL = (1 << 7),
            ROTATE_USE_MODEL = (1 << 8),
            TRANS_USE_MODEL = (1 << 9),
            SCALE_COMPENSATE_APPLY = (1 << 10),
            SCALE_COMPENSATE_PARENT = (1 << 11),
            CLASSIC_SCALE_OFF = (1 << 12),
            SCALE_X_CONSTANT = (1 << 13),
            SCALE_Y_CONSTANT = (1 << 14),
            SCALE_Z_CONSTANT = (1 << 15),
            ROTATE_X_CONSTANT = (1 << 16),
            ROTATE_Y_CONSTANT = (1 << 17),
            ROTATE_Z_CONSTANT = (1 << 18),
            TRANS_X_CONSTANT = (1 << 19),
            TRANS_Y_CONSTANT = (1 << 20),
            TRANS_Z_CONSTANT = (1 << 21),
            REQUIRE_SCALE = (1 << 22),
            REQUIRE_ROTATE = (1 << 23),
            REQUIRE_TRANS = (1 << 24),

            SCALE_NOT_EXIST = (IDENTITY | SCALE_ONE | SCALE_USE_MODEL),
            ROTATE_NOT_EXIST = (IDENTITY | RT_ZERO | ROTATE_ZERO | ROTATE_USE_MODEL),
            TRANS_NOT_EXIST = (IDENTITY | RT_ZERO | TRANS_ZERO | TRANS_USE_MODEL),
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

        stream->seek(p);
        uint32_t nameOffs = stream->readUInt32();
        std::string nodeName = ReadString(stream,p+nameOffs);

        stream->seek(p+0x04);
        uint32_t flags = stream->readUInt32();

        size_t animationTableIdx = 0x08;

        auto nextAnimationTrack =
            [&](TrackFormat fmt, bool isConst) -> FloatAnimationTrack
        {
            FloatAnimationTrack track;
            stream->seek(p + animationTableIdx);
            if (isConst || fmt == TrackFormat::CONSTANT) {
                float value = stream->readFloat();
                track = MakeConstantAnimationTrack(value);
            }
            else {
                uint32_t offs = stream->readUInt32();

                switch (fmt) {
                case TrackFormat::_32:
                    track = ParseAnimationTrackF32(stream,p+offs);
                    break;
                case TrackFormat::_48:
                    track = ParseAnimationTrackF48(stream, p + offs);
                    break;
                case TrackFormat::_96:
                    track = ParseAnimationTrackF96(stream, p + offs);
                    break;
                case TrackFormat::FRM_8:
                    track = ParseAnimationTrackC8(stream, numKeyframes, p + offs);
                    break;
                case TrackFormat::FRM_16:
                    track = ParseAnimationTrackC16(stream, numKeyframes, p + offs);
                    break;
                case TrackFormat::FRM_32:
                    track = ParseAnimationTrackC32(stream, numKeyframes, p + offs);
                    break;
                default:
                    throw std::runtime_error("Unsupported CHR0 track format");
                }
            }

            animationTableIdx += 0x04;
            return track;
        };

        TrackFormat scaleFmt = static_cast<TrackFormat>((flags >> 25) & 0x03);
        TrackFormat rotationFmt = static_cast<TrackFormat>((flags >> 27) & 0x07);
        TrackFormat translationFmt = static_cast<TrackFormat>((flags >> 30) & 0x03);

        CHR0_NodeData out{};
        out.nodeName = nodeName;

        if (!(flags & Flags::SCALE_NOT_EXIST)) {
            out.scaleX = nextAnimationTrack(scaleFmt, flags & Flags::SCALE_X_CONSTANT);
        }

        if (!(flags & Flags::SCALE_UNIFORM)) {
            out.scaleY = nextAnimationTrack(scaleFmt, flags & Flags::SCALE_Y_CONSTANT);
            out.scaleZ = nextAnimationTrack(scaleFmt, flags & Flags::SCALE_Z_CONSTANT);
        }
        else {
            out.scaleY = out.scaleX;
            out.scaleZ = out.scaleX;
        }

        if (!(flags & Flags::ROTATE_NOT_EXIST)) {
            out.rotationX = nextAnimationTrack(rotationFmt, flags & Flags::ROTATE_X_CONSTANT);
            out.rotationY = nextAnimationTrack(rotationFmt, flags & Flags::ROTATE_Y_CONSTANT);
            out.rotationZ = nextAnimationTrack(rotationFmt, flags & Flags::ROTATE_Z_CONSTANT);
        }

        if (!(flags & Flags::TRANS_NOT_EXIST)) {
            out.translationX = nextAnimationTrack(translationFmt, flags & Flags::TRANS_X_CONSTANT);
            out.translationY = nextAnimationTrack(translationFmt, flags & Flags::TRANS_Y_CONSTANT);
            out.translationZ = nextAnimationTrack(translationFmt, flags & Flags::TRANS_Z_CONSTANT);
        }

        return out;
    }
    bres::CHR0 bres::ParseCHR0(bStream::CStream* stream, size_t p) {
        stream->seek(p + 0x08);
        uint32_t version = stream->readUInt32();
        if (version != 0x03 && version != 0x04 && version != 0x05)
            throw std::runtime_error("Unsupported CHR0 version");

        stream->seek(p + 0x10);
        uint32_t chrNodeDataResDicOffs = stream->readUInt32();
        auto nodeResDic = ParseResDic(stream,p+chrNodeDataResDicOffs);

        size_t offs = 0x14;
        if (version >= 0x05)
            offs += 0x04;

        stream->seek(p + offs);
        uint32_t nameOffs = stream->readUInt32();
        std::string name = ReadString(stream,p+ nameOffs);

        stream->seek(p + 0x08 + offs);
        uint16_t duration = stream->readUInt16();;
        uint16_t numNodes = stream->readUInt16();
        LoopMode loopMode = static_cast<LoopMode>(stream->readUInt32());

        uint32_t scalingRule = stream->readUInt32();
        (void)scalingRule;

        std::vector<CHR0_NodeData> nodeAnimations;
        nodeAnimations.reserve(numNodes);

        for (const auto& entry : nodeResDic) {
            stream->seek(entry.offs);

            CHR0_NodeData node = ParseCHR0_NodeData(stream, duration,entry.offs);
            nodeAnimations.push_back(std::move(node));
        }

        if (nodeAnimations.size() != numNodes)
            throw std::runtime_error("CHR0 node count mismatch");

        CHR0 out{};
        out.name = std::move(name);
        out.duration = static_cast<float>(duration);
        out.loopMode = loopMode;
        out.nodeAnimations = std::move(nodeAnimations);
        return out;
    }
    int bres::ToTexMtxIndex(int m)
    {
        if (m >= 30)
            return (m - 30) / 3;
        else
            return m;         
    }

    int bres::ToPostMtxIndex(int m)
    {
        if (m >= 64)
            return (m - 64) / 3;
        else
            return m;
    }
    GLenum bres::toGLType(EGXComponentType t) {
        switch (t) {
        case EGXComponentType::Unsigned8:   return GL_UNSIGNED_BYTE;  
        case EGXComponentType::Signed8:     return GL_BYTE;            
        case EGXComponentType::Unsigned16:  return GL_UNSIGNED_SHORT;  
        case EGXComponentType::Signed16:    return GL_SHORT;         
        case EGXComponentType::Float:       return GL_FLOAT;         
        default: return GL_FLOAT;
        }
    }

    bool bres::toNormalized(EGXComponentType t) {
        switch (t) {
        case EGXComponentType::Unsigned8:
        case EGXComponentType::Signed8: 
        case EGXComponentType::Unsigned16:
        case EGXComponentType::Signed16:
            return true;

        case EGXComponentType::Float:
        case EGXComponentType::RGBA8:
            return false;
        }
        return false;
    }
GLuint bres::UploadTEX0Texture(bStream::CStream* stream, const TEX0& tex)
{
    stream->seek(tex.data);

    std::vector<uint8_t> rgba(tex.width * tex.height * 4);

    switch (tex.format) {
    case 0x00: DecodeI4(stream, tex.width, tex.height, rgba.data()); break;
    case 0x01: DecodeI8(stream, tex.width, tex.height, rgba.data()); break;
    case 0x02: DecodeIA4(stream, tex.width, tex.height, rgba.data()); break;
    case 0x03: DecodeIA8(stream, tex.width, tex.height, rgba.data()); break;
    case 0x04: DecodeRGB565(stream, tex.width, tex.height, rgba.data()); break;
    case 0x05: DecodeRGB5A3(stream, tex.width, tex.height, rgba.data()); break;
    case 0x06: DecodeRGBA32(stream, tex.width, tex.height, rgba.data()); break;
    case 0x0E: DecodeCMPR(stream, tex.width, tex.height, rgba.data()); break;
    }

    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        tex.width,
        tex.height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgba.data()
    );
    std::vector<uint8_t>().swap(rgba);

    return id;
}
bres::ShapeRuntime bres::buildShapeData(const LoadedVertexData& vtx)
{
    ShapeRuntime rt;
    rt.layout = vtx.layout;
    rt.indexCount = (GLsizei)vtx.indexBuffer.size();

    glm::vec3 min(+FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    const uint8_t* base = vtx.vertexBuffer.data();
    uint32_t stride = vtx.layout.stride;
    uint32_t posOffset = vtx.layout.posOffset;
    for (uint16_t idx : vtx.indexBuffer) {
        const uint8_t* ptr = base + idx * stride + posOffset;

        float x = *reinterpret_cast<const float*>(ptr + 0);
        float y = *reinterpret_cast<const float*>(ptr + 4);
        float z = *reinterpret_cast<const float*>(ptr + 8);

        glm::vec3 pos(x, y, z);
        min = glm::min(min, pos);
        max = glm::max(max, pos);
    }

    rt.boundingCenter = (min + max) * 0.5f;
    rt.boundingRadius = glm::length(max - rt.boundingCenter);
    rt.vertexBuffer = vtx.vertexBuffer;


    glGenVertexArrays(1, &rt.vao);
    glBindVertexArray(rt.vao);

    glGenBuffers(1, &rt.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, rt.vbo);
    glBufferData(GL_ARRAY_BUFFER,
        vtx.vertexBuffer.size(),
        vtx.vertexBuffer.data(),
        GL_DYNAMIC_DRAW);

    glGenBuffers(1, &rt.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rt.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
        vtx.indexBuffer.size() * sizeof(uint16_t),
        vtx.indexBuffer.data(),
        GL_STATIC_DRAW);


        const auto& layout = rt.layout;

        if (layout.posOffset >= 0) {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)(size_t)layout.posOffset);
        }
        if (layout.nrmOffset >= 0) {
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                stride, (void*)(size_t)layout.nrmOffset);
        }
        if (layout.clr0Offset >= 0) {
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                stride, (void*)(size_t)layout.clr0Offset);
        }
        if (layout.clr1Offset >= 0) {
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE,
                stride, (void*)(size_t)layout.clr1Offset);
        }
if (layout.texOffset[0] >= 0) {
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(
        4, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[0]
    );
}
if (layout.texOffset[1] >= 0) {
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(
        5, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[1]
    );
}
if (layout.texOffset[2] >= 0) {
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(
        6, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[2]
    );
}
if (layout.texOffset[3] >= 0) {
    glEnableVertexAttribArray(7);
    glVertexAttribPointer(
        7, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[3]
    );
}
if (layout.texOffset[4] >= 0) {
    glEnableVertexAttribArray(8);
    glVertexAttribPointer(
        8, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[4]
    );
}
if (layout.texOffset[5] >= 0) {
    glEnableVertexAttribArray(9);
    glVertexAttribPointer(
        9, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[5]
    );
}
if (layout.texOffset[6] >= 0) {
    glEnableVertexAttribArray(10);
    glVertexAttribPointer(
        10, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[6]
    );
}
if (layout.texOffset[7] >= 0) {
    glEnableVertexAttribArray(11);
    glVertexAttribPointer(
        11, 2, GL_FLOAT, GL_FALSE,
        stride,
        (void*)(size_t)layout.texOffset[7]
    );
}
for (int i = 0; i < 8; ++i) {
    if (layout.texOffset[i] >= 0) {
        rt.texCoordMap[i] = i;
    } else {
        rt.texCoordMap[i] = -1;
    }
}
glEnableVertexAttribArray(12);
glVertexAttribIPointer(12, 4, GL_UNSIGNED_INT, stride, (void*)layout.boneIndexOffset);

glEnableVertexAttribArray(13);
glVertexAttribPointer(13, 4, GL_FLOAT, GL_FALSE, stride, (void*)layout.boneWeightOffset);

  glBindVertexArray(0);
  rt.min = min;
rt.max = max;

        return rt;
    }
    uint32_t makeSortKeyOpaque(uint32_t layer, uint32_t programKey, uint32_t depthKey) {
        return
            (0u << 31) |
            ((layer & 0x7F) << 24) |
            ((depthKey & 0xFFFFFF) << 0);
    }

uint32_t makeSortKeyTranslucent(uint32_t layer, uint32_t depthKey, uint32_t bias) {
    return
        (1u << 31) |
        ((layer & 0x7F) << 24) |
        ((depthKey & 0xFFFF) << 8) |
        (bias & 0xFF);
}

    void bres::ModelInstance::GatherRenderPackets(
        std::vector<Packet>& packets,
        const glm::mat4& view,
        const glm::vec3& cameraPos)
    {
        auto& sg = model->sceneGraph;

        auto process = [&](const DrawOp& op) {
            auto& mat = model->materials[op.matId];
            auto& shp = model->shapes[op.shpId];
            int nodeId = shp.mtxIdx;
            auto& node = model->nodes[nodeId];

            bool isTranslucent = (mat.translucent);

            glm::vec3 localCenter = shp.runtime->center();

            glm::mat4 modelRoot = GetModelMatrix();
            glm::mat4 nodeWorld = modelRoot * model->worldMatrices[nodeId];

            glm::vec3 worldCenter = glm::vec3(nodeWorld * glm::vec4(localCenter, 1.0));
            float dist = glm::distance(cameraPos, worldCenter);
glm::vec4 viewPos = view * glm::vec4(worldCenter, 1.0f);
float depth = -viewPos.z;

float depthClamped = glm::clamp(depth, 0.0f, 65535.0f);
uint32_t depthKey = uint32_t(depthClamped);
uint32_t sortKey;
if (!isTranslucent) {
    uint32_t bias = op.shpId & 0xFF;
    sortKey = makeSortKeyOpaque(mat.index, bias, depthKey);
} else {
    uint32_t bias = op.shpId & 0xFF;
    sortKey = makeSortKeyTranslucent(mat.index, depthKey, bias);
}

            packets.push_back({
                sortKey,
                &shp,
                &node,
                this,
                op.matId,
                nodeId,
                dist,
                isTranslucent
                });
        };

        for (auto& op : sg.drawOpaOps) process(op);
        for (auto& op : sg.drawXluOps) process(op);
    }

    void bres::SetLights(const LightSet& ls) {
        for (auto& p : packets)
        {
            p.localLights = ls;
        }
    }
    void bres::MDL0::updateBoneMatrices()
    {
        boneMatrices.clear();
        boneMatrices.resize(mtxIdToNodeId.size());

        for (size_t mtxId = 0; mtxId < mtxIdToNodeId.size(); mtxId++)
        {
            int nodeId = mtxIdToNodeId[mtxId];
            boneMatrices[mtxId] = worldMatrices[nodeId] * nodes[nodeId].inverseBindPose;
        }
    }


void bres::MDL0::updateWorldMatrices(const glm::mat4& camWorld)
{
    worldMatrices.resize(nodes.size());

    for (size_t i = 0; i < nodes.size(); i++) {
        const auto& node = nodes[i];

        glm::mat4 local =
            node.billboardMode == BillboardMode::NONE
            ? node.computeLocalMatrix()
            : node.computeLocalMatrixBillboard(camWorld);

        if (node.parentNodeId < 0)
            worldMatrices[i] = local;
        else
            worldMatrices[i] = worldMatrices[node.parentNodeId] * local;
    }
}
    GLenum ConvertGXBlendFactor(EGXBlendModeControl f)
    {
        switch (f)
        {
        case EGXBlendModeControl::Zero:
            return GL_ZERO;

        case EGXBlendModeControl::One:
            return GL_ONE;

        case EGXBlendModeControl::SrcColor:
            return GL_SRC_COLOR;

        case EGXBlendModeControl::InverseSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;

        case EGXBlendModeControl::SrcAlpha:
            return GL_SRC_ALPHA;

        case EGXBlendModeControl::InverseSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;

        case EGXBlendModeControl::DstAlpha:
            return GL_DST_ALPHA;

        case EGXBlendModeControl::InverseDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        }

        return GL_ONE;
    }
    GLenum ConvertGXLogicOp(EGXLogicOp op)
    {
        switch (op)
        {
        case EGXLogicOp::Clear:          return GL_CLEAR;
        case EGXLogicOp::And:            return GL_AND;
        case EGXLogicOp::RevAnd:     return GL_AND_REVERSE;
        case EGXLogicOp::Copy:           return GL_COPY;
        case EGXLogicOp::InvAnd:    return GL_AND_INVERTED;
        case EGXLogicOp::NoOp:           return GL_NOOP;
        case EGXLogicOp::XOr:            return GL_XOR;
        case EGXLogicOp::Or:             return GL_OR;
        case EGXLogicOp::NOr:            return GL_NOR;
        case EGXLogicOp::Equiv:          return GL_EQUIV;
        case EGXLogicOp::Inv:         return GL_INVERT;
        case EGXLogicOp::RevOr:      return GL_OR_REVERSE;
        case EGXLogicOp::InvCopy:   return GL_COPY_INVERTED;
        case EGXLogicOp::InvOr:     return GL_OR_INVERTED;
        case EGXLogicOp::NAnd:           return GL_NAND;
        case EGXLogicOp::Set:            return GL_SET;
        }

        return GL_COPY;
    }
    void bres::Packet::Render(const glm::mat4& view, const glm::mat4& proj,const glm::vec3 cameraPos)
    {
        MaterialInstance* matInst = instance->GetMaterialInstance(materialIndex);
        auto& mat = matInst->material;
        auto& shp = *shape;
        auto& nodes = *node;
        auto& inst = *instance;
        glUseProgram(mat->shaderProgram);
                switch (mat->gxMaterial.cullMode) {
        case EGXCullMode::None:  glDisable(GL_CULL_FACE); break;
        case EGXCullMode::Back:  glEnable(GL_CULL_FACE); glCullFace(GL_BACK); break;
        case EGXCullMode::Front: glEnable(GL_CULL_FACE); glCullFace(GL_FRONT); break;
        case EGXCullMode::All:   glEnable(GL_CULL_FACE); glCullFace(GL_FRONT_AND_BACK); break;
        }
            if (mat->gxMaterial.ropInfo.depthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

    glDepthMask(mat->gxMaterial.ropInfo.depthWrite ? GL_TRUE : GL_FALSE);

    switch (mat->gxMaterial.ropInfo.depthFunc) {
        case EGXCompareType::Less:    glDepthFunc(GL_LESS); break;
        case EGXCompareType::LEqual:  glDepthFunc(GL_LEQUAL); break;
        case EGXCompareType::Equal:   glDepthFunc(GL_EQUAL); break;
        case EGXCompareType::GEqual:  glDepthFunc(GL_GEQUAL); break;
        case EGXCompareType::Greater: glDepthFunc(GL_GREATER); break;
        case EGXCompareType::Always:  glDepthFunc(GL_ALWAYS); break;
        case EGXCompareType::Never:   glDepthFunc(GL_NEVER); break;
        }
        switch (mat->gxMaterial.ropInfo.blendMode)
        {
        case EGXBlendMode::None:
            glDisable(GL_BLEND);
            glDisable(GL_COLOR_LOGIC_OP);
glColorMask(
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.alphaUpdate ? GL_TRUE : GL_FALSE
);

            glBlendEquation(GL_FUNC_ADD);
            break;

        case EGXBlendMode::Blend:
            glEnable(GL_BLEND);
            glDisable(GL_COLOR_LOGIC_OP);

            glBlendFunc(
                ConvertGXBlendFactor(mat->gxMaterial.ropInfo.blendSrcFactor),
                ConvertGXBlendFactor(mat->gxMaterial.ropInfo.blendDstFactor)
            );
            glBlendEquation(GL_FUNC_ADD);
glColorMask(
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.alphaUpdate ? GL_TRUE : GL_FALSE
);

            break;

        case EGXBlendMode::Logic:
            glDisable(GL_BLEND);
            glEnable(GL_COLOR_LOGIC_OP);
            glLogicOp(ConvertGXLogicOp(mat->gxMaterial.ropInfo.blendLogicOp));
            glBlendEquation(GL_FUNC_ADD);
glColorMask(
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.alphaUpdate ? GL_TRUE : GL_FALSE
);

            break;

        case EGXBlendMode::Subtract:
            glEnable(GL_BLEND);
            glDisable(GL_COLOR_LOGIC_OP);

            glBlendFunc(
                ConvertGXBlendFactor(mat->gxMaterial.ropInfo.blendSrcFactor),
                ConvertGXBlendFactor(mat->gxMaterial.ropInfo.blendDstFactor)
            );
            glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
glColorMask(
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.colorUpdate ? GL_TRUE : GL_FALSE,
    mat->gxMaterial.ropInfo.alphaUpdate ? GL_TRUE : GL_FALSE
);

            break;
        }

if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_CameraPos"); loc >= 0) {
    glUniform3fv(loc, 1, glm::value_ptr(cameraPos));
}


for (int i = 0; i < static_cast<int>(mat->gxMaterial.texMatrices.size()); ++i) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram,
        ("u_TexMtx[" + std::to_string(i) + "]").c_str());
        loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE,
            glm::value_ptr(mat->gxMaterial.texMatrices[i]));
    }
}

for (int i = 0; i < static_cast<int>(mat->gxMaterial.postTexMatrices.size()); ++i) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram,
        ("u_PostTexMtx[" + std::to_string(i) + "]").c_str());
        loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE,
            glm::value_ptr(mat->gxMaterial.postTexMatrices[i]));
    }
}
        for (int i = 0; i < static_cast<int>(mat->texSrts.size()); ++i) {
            const auto& srt = mat->texSrts[i];
            if (GLint loc = glGetUniformLocation(mat->shaderProgram, ("u_TexSrtMtx[" + std::to_string(i) + "]").c_str()); loc >= 0)
                glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(srt.srtMtx));
            if (GLint loc = glGetUniformLocation(mat->shaderProgram, ("u_EffectMtx[" + std::to_string(i) + "]").c_str());
                loc >= 0) {
                glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(srt.effectMtx));
            }
        }

        
for (const auto& s : mat->samplers) {
    int texMap = s.texMapSlot;

    if (s.glTexID == 0)
        continue;

    glActiveTexture(GL_TEXTURE0 + texMap);
    glBindTexture(GL_TEXTURE_2D, s.glTexID);

    std::string name = "u_Tex" + std::to_string(texMap);
    if (GLint loc = glGetUniformLocation(mat->shaderProgram, name.c_str());
        loc >= 0) {
        glUniform1i(loc, texMap);
    }
}
for (int i = 0; i < 3; i++) {
    GLint loc = glGetUniformLocation(mat->shaderProgram,
        ("u_IndMtx[" + std::to_string(i) + "]").c_str());
    if (loc >= 0)
        glUniformMatrix3fv(loc, 1, GL_FALSE, glm::value_ptr(matInst->indTexSrtMtx[i]));
}

auto& regs = mat->colorRegisters;

if (regs.size() >= 1) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_TevRegPrev"); loc >= 0)
        glUniform4fv(loc, 1, ColorPtr(regs[0]));
}
if (regs.size() >= 2) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_TevReg0"); loc >= 0)
        glUniform4fv(loc, 1, ColorPtr(regs[1]));
}
if (regs.size() >= 3) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_TevReg1"); loc >= 0)
        glUniform4fv(loc, 1, ColorPtr(regs[2])); 
}
if (regs.size() >= 4) {
    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_TevReg2"); loc >= 0)
        glUniform4fv(loc, 1, ColorPtr(regs[3]));
}

for (int i = 0; i < mat->samplers.size(); i++) {
    GLint loc = glGetUniformLocation(mat->shaderProgram,
        ("u_TexSize[" + std::to_string(i) + "]").c_str());
    if (loc >= 0)
        glUniform2f(loc, (float)mat->samplers[i].width, (float)mat->samplers[i].height);
}


        for (int i = 0; i < 4 && i < static_cast<int>(mat->colorConstants.size()); ++i) {
            if (GLint loc = glGetUniformLocation(mat->shaderProgram, ("u_KColor[" + std::to_string(i) + "]").c_str());
                loc >= 0) {
                glUniform4fv(loc, 1, ColorPtr(mat->colorConstants[i]));
            }
        }

        for (int chan = 0; chan < static_cast<int>(mat->colorMatRegs.size()); ++chan) {
            if (chan >= static_cast<int>(mat->colorAmbRegs.size()))
                break;

            if (GLint loc = glGetUniformLocation(mat->shaderProgram, ("u_MatColor[" + std::to_string(chan) + "]").c_str());
                loc >= 0) {
                glUniform4fv(loc, 1, ColorPtr(mat->colorMatRegs[chan]));
            }
            if (GLint loc = glGetUniformLocation(mat->shaderProgram, ("u_AmbColor[" + std::to_string(chan) + "]").c_str());
                loc >= 0) {
                glUniform4fv(loc, 1, ColorPtr(mat->colorAmbRegs[chan]));
            }
        }
        if (mat->lightSetIdx > 0)
        {
            const auto& lightSet = localLights;

for (int i = 0; i < lightSet.lights.size(); i++) {
    const auto& L = lightSet.lights[i];

    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightPos[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(L.position));

    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightDir[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(L.direction));

    glUniform4fv(glGetUniformLocation(mat->shaderProgram, ("u_LightColor[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(L.color));

    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightDistAtten[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(L.distAtten));

    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightCosAtten[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(L.cosAtten));

    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_LightIsDirectional[" + std::to_string(i) + "]").c_str()),
                L.isDirectional ? 1 : 0);
}
        }
        else
        {

for (int i = 0; i < 8; ++i) {
    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightPos[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(glm::vec3(0.0f)));
    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightDir[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(glm::vec3(0.0f)));
    glUniform4fv(glGetUniformLocation(mat->shaderProgram, ("u_LightColor[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(glm::vec4(0.0f)));
    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightDistAtten[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(glm::vec3(0.0f, 0.0f, 1.0f)));
    glUniform3fv(glGetUniformLocation(mat->shaderProgram, ("u_LightCosAtten[" + std::to_string(i) + "]").c_str()),
                 1, glm::value_ptr(glm::vec3(1.0f, 0.0f, 0.0f)));
    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_LightIsDirectional[" + std::to_string(i) + "]").c_str()),
                0);
}
        }
        for (int chan = 0; chan < mat->gxMaterial.lightChannels.size(); chan++) {
    const auto& cc = mat->gxMaterial.lightChannels[chan].colorChannel;

    int diffuse = 0;
    switch (cc.diffuseFunction) {
        case EGXDiffuseFunction::None:   diffuse = 0; break;
        case EGXDiffuseFunction::Clamp:  diffuse = 1; break;
        case EGXDiffuseFunction::Signed: diffuse = 2; break;
    }

    int attn = 0;
    switch (cc.attenuationFunction) {
        case EGXAttenuationFunction::None: attn = 0; break;
        case EGXAttenuationFunction::Spec: attn = 1; break;
        case EGXAttenuationFunction::Spot: attn = 2; break;
    }

    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].lightingEnabled").c_str()),
                cc.lightingEnabled ? 1 : 0);

    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].diffuseFunc").c_str()),
                diffuse);

    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].attnFunc").c_str()),
                attn);
    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].lightMask").c_str()),
        cc.litMask);
    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].ambSrc").c_str()),
        (int)cc.ambColorSource);
    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_ChannelCtrl[" + std::to_string(chan) + "].matSrc").c_str()),
        (int)cc.matColorSource);
}
        glUniform1i(glGetUniformLocation(mat->shaderProgram, "u_DebugMode"),0);
        for (int i = 0; i < mat->gxMaterial.texGens.size(); ++i) {
    glUniform1i(glGetUniformLocation(mat->shaderProgram, ("u_TexGenType[" + std::to_string(i) + "]").c_str()),
                 (int)mat->gxMaterial.texGens[i].type);
}
        if (!shp.runtime || shp.runtime->indexCount == 0) {
            return;
        }
        glBindVertexArray(shp.runtime->vao);
        glDrawElements(GL_TRIANGLES,
            shp.runtime->indexCount,
            GL_UNSIGNED_SHORT,
            nullptr);
        glDisable(GL_COLOR_LOGIC_OP);
        glDisable(GL_BLEND);

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        glDisable(GL_CULL_FACE);
    }
    bres::MaterialInstance* bres::ModelInstance::GetMaterialInstance(int materialIndex) {
        if (materialIndex < 0 || materialIndex >= (int)materialInstances.size())
            return nullptr;
        return materialInstances[materialIndex].get();
    }

std::vector<std::string> bres::ParseSHP0Section1_VertexNames(bStream::CStream* stream, size_t base, uint32_t stringListOffset)
{
    std::vector<std::string> names;

    if (stringListOffset == 0)
        return names;

    size_t sec1Base = base + stringListOffset;
    stream->seek(sec1Base);

    uint32_t count = stream->peekUInt16(base + 0x26);
    names.resize(count);

    std::vector<uint32_t> offsets(count);
    for (uint32_t i = 0; i < count; ++i)
        offsets[i] = stream->readUInt32();

    for (uint32_t i = 0; i < count; ++i)
    {
        if (offsets[i] == 0) {
            names[i].clear();
            continue;
        }

        size_t strPos = sec1Base + offsets[i];
        names[i] = ReadString(stream, strPos);
    }

    return names;
}
bres::SHP0 bres::ParseSHP0(bStream::CStream* stream, size_t p)
{
    SHP0 out{};

    stream->seek(p + 0x08);
    uint32_t version = stream->readUInt32();
    assert(version == 0x04);

    stream->seek(p + 0x10);
    uint32_t dataOffset       = stream->readUInt32();
    uint32_t stringListOffset = stream->readUInt32();
    uint32_t userDataOffset   = stream->readUInt32();
    uint32_t stringOffset     = stream->readUInt32();
    uint32_t origPathOffset   = stream->readUInt32();

    out.duration        = stream->readUInt16();
    uint16_t numEntries = stream->readUInt16();
    out.loopMode        = (LoopMode)stream->readUInt32();
    out.name            = ReadString(stream, p + stringOffset);

    auto vertexNames = ParseSHP0Section1_VertexNames(stream, p, stringListOffset);

    auto group = ParseResDic(stream, p + dataOffset);
    for (int e = 0; e < numEntries - 1; ++e) {
        const auto& dic = group[e];

        size_t entPos = dic.offs;
        stream->seek(entPos);

        int32_t flags       = stream->readInt32();
        int32_t strOffset   = stream->readInt32();
        int16_t nameIndex   = stream->readInt16();
        int16_t numIndices  = stream->readInt16();
        int32_t fixedFlags  = stream->readInt32();
        int32_t indicesOffs = stream->readInt32();

        SHP0_EntryData entry{};

        entry.vertexNodeName = ReadString(stream, entPos + strOffset);

        size_t indicesPos = entPos + indicesOffs;
        stream->seek(indicesPos);
        std::vector<int16_t> indices(numIndices);
        for (int i = 0; i < numIndices; ++i)
            indices[i] = stream->readInt16();

        size_t entryOffsetsPos = indicesPos - 4 * numIndices;
        stream->seek(entryOffsetsPos);
        std::vector<int32_t> entryOffsets(numIndices);
        for (int i = 0; i < numIndices; ++i)
            entryOffsets[i] = stream->readInt32();

        for (int i = 0; i < numIndices; ++i) {
            size_t kfBase = entryOffsetsPos + entryOffsets[i];
            stream->seek(kfBase);
            int16_t numKf = stream->readInt16();
            int16_t pad   = stream->readInt16();

            SHP0_MorphTrack track;

            track.shapeIndex = indices[i];

            if (track.shapeIndex >= 0 && track.shapeIndex < (int)vertexNames.size())
                track.targetName = vertexNames[track.shapeIndex];

            FloatTrackHermite herm;
            herm.frames.reserve(numKf);

            float invDuration = stream->readFloat();

            for (int k = 0; k < numKf; ++k) {
                FloatKeyHermite key;
                key.frame   = stream->readFloat();
                key.value   = stream->readFloat();
                key.tangent = stream->readFloat();
                herm.frames.push_back(key);
            }

            track.keys = herm;
            entry.tracks.push_back(track);
        }

        out.entries.push_back(std::move(entry));
    }
    
    return out;
}

    bres::RRES bres::ParseRRES(bStream::CStream* stream, size_t size) {
        stream->seek(0);
        uint32_t header = stream->readUInt32();
        uint16_t endian = stream->readUInt16();
        if (endian != 0xFEFF)
            throw std::runtime_error("Unsupported endian");

        stream->seek(8);
        uint32_t fileLength = stream->readUInt32();
        uint16_t rootSectionOffs = stream->readUInt16();

        size_t rootBase = rootSectionOffs;
        stream->seek(rootBase);
        size_t rootSize = stream->readUInt32();

        stream->seek(rootBase);
        assert(stream->readString(4) == "root");
        auto rootResDic = ParseResDic(stream, rootSectionOffs + 8);


        RRES out;
        if (auto* palettesEntry = FindResDicEntry(rootResDic, "Palettes(NW4R)")) {
            auto palettesResDic = ParseResDic(stream, palettesEntry->offs);
            for (auto& plt0Entry : palettesResDic) {
                out.plt0.push_back(ParsePLT0(stream,plt0Entry.offs));
                assert(out.plt0.back().name == plt0Entry.name);
            }
        }
        if (auto* texturesEntry = FindResDicEntry(rootResDic, "Textures(NW4R)")) {
            auto texturesResDic = ParseResDic(stream, texturesEntry->offs);
            for (auto& tex0Entry : texturesResDic) {
                out.tex0.push_back(ParseTEX0(stream, tex0Entry.offs));
                assert(out.tex0.back().name == tex0Entry.name);
            }
        }
        if (auto* modelsEntry = FindResDicEntry(rootResDic, "3DModels(NW4R)")) {
            auto modelsResDic = ParseResDic(stream, modelsEntry->offs);
            for (auto& mdl0Entry : modelsResDic) {
                out.mdl0.push_back(parseMDL0(stream, size, mdl0Entry.offs));
                assert(out.mdl0.back().name == mdl0Entry.name);
            }
        }
        if (auto* anmTexSrtEntry = FindResDicEntry(rootResDic, "AnmTexSrt(NW4R)")) {
            auto anmTexSrtResDic = ParseResDic(stream, anmTexSrtEntry->offs);
            for (auto& srt0Entry : anmTexSrtResDic) {
                out.srt0.push_back(ParseSRT0(stream, size,srt0Entry.offs));
                assert(out.srt0.back().name == srt0Entry.name);
            }
        }
        if (auto* anmTexPatEntry = FindResDicEntry(rootResDic, "AnmTexPat(NW4R)")) {
            auto anmTexPatResDic = ParseResDic(stream, anmTexPatEntry->offs);
            for (auto& pat0Entry : anmTexPatResDic) {
                try {
                    out.pat0.push_back(ParsePAT0(stream, pat0Entry.offs));
                    assert(out.pat0.back().name == pat0Entry.name);
                }
                catch (...) {

                }
            }
        }
        if (auto* anmClrEntry = FindResDicEntry(rootResDic, "AnmClr(NW4R)")) {
            auto anmClrResDic = ParseResDic(stream, anmClrEntry->offs);
            for (auto& clr0Entry : anmClrResDic) {
                out.clr0.push_back(ParseCLR0(stream, clr0Entry.offs));
                assert(out.clr0.back().name == clr0Entry.name);
            }
        }
        if (auto* anmChrEntry = FindResDicEntry(rootResDic, "AnmChr(NW4R)")) {
           
            auto anmChrResDic = ParseResDic(stream, anmChrEntry->offs);
            for (auto& chr0Entry : anmChrResDic) {
                out.chr0.push_back(ParseCHR0(stream, chr0Entry.offs));
                assert(out.chr0.back().name == chr0Entry.name);
            }
        }

        if (auto* anmShpEntry = FindResDicEntry(rootResDic, "AnmShp(NW4R)")) {
           
            auto anmShpResDic = ParseResDic(stream, anmShpEntry->offs);
            for (auto& shp0Entry : anmShpResDic) {
                out.shp0.push_back(ParseSHP0(stream, shp0Entry.offs));
                assert(out.shp0.back().name == shp0Entry.name);
            }
        }

        return out;
    }
    GLuint bres::BuildMaterialShader(const MDL0_MaterialEntry& mat,const ShapeRuntime& runtime)
    {
        std::string vs = GenerateVertexShader(mat.gxMaterial, mat.texSrts,runtime);
        std::string fs = GenerateFragmentShader(mat.gxMaterial,8, mat.colorConstants);

        return CreateShaderProgram(vs.c_str(), fs.c_str());
    }
    void bres::clearinstance()
    {
        for (auto& mdl : allLoadedModels)
        {
            for (auto& shp : mdl->shapes)
            {
                if (!shp.runtime) continue;

                if (shp.runtime->ibo) glDeleteBuffers(1, &shp.runtime->ibo);
                if (shp.runtime->vbo) glDeleteBuffers(1, &shp.runtime->vbo);
                if (shp.runtime->vao) glDeleteVertexArrays(1, &shp.runtime->vao);

                shp.runtime.reset();
            }

            for (auto& mat : mdl->materials)
            {
                mat.shaderProgram = 0;
            }
        }

        if (rres)
        {
            for (auto& tex : rres->tex0)
            {
                if (tex.texID)
                {
                    glDeleteTextures(1, &tex.texID);
                    tex.texID = 0;
                }
            }
        }
        glDeleteTextures(1, &id);
        id = 0;
        allLoadedModels.clear();
        texByName.clear();
        packets.clear();
        instance.clear();
        rres = nullptr;
    }
    void bres::LoadAllAnimations()
    {
        for (auto& inst : instance)
        {
            for (int i = 0; i < inst->loadedSRT0s.size(); i++)
                inst->BindSRT0(animationController.get(), inst->loadedSRT0s[i].get());

            for (int i = 0; i < inst->loadedPAT0s.size(); i++)
                inst->BindPAT0(animationController.get(), inst->loadedPAT0s[i].get(), texByName);

            for (int i = 0; i < inst->loadedCLR0s.size(); i++)
                inst->BindCLR0(animationController.get(), inst->loadedCLR0s[i].get());

            for (int i = 0; i < inst->loadedSHP0s.size(); i++)
                inst->BindSHP0(animationController.get(), inst->loadedSHP0s[i].get());
        }
    }
    
    void bres::Loader(bStream::CStream* stream, int modelId,glm::vec3 pos, glm::vec3 rot, glm::vec3 scale)
    {
        animationController->SetTimeSeconds(0);
        rres = std::make_shared<RRES>(ParseRRES(stream, stream->getSize()));
        if (!rres) {
            return;
        }

        if (modelId < 0 || modelId >= (int)rres->mdl0.size()) {
            return;
        }

        MDL0& mdl0 = rres->mdl0[modelId];
        for (auto& tex : rres->tex0) {
            stream->seek(tex.data);
            tex.texID = UploadTEX0Texture(stream, tex);
            texByName[tex.name] = &tex;
        }

        auto processMaterial = [&](const DrawOp& op) {
    auto& shp = mdl0.shapes[op.shpId];
    auto& mat = mdl0.materials[op.matId];

    if (!shp.runtime) {
        shp.runtime = std::make_shared<ShapeRuntime>();
        *shp.runtime = buildShapeData(shp.vtxdata);
    }

    for (auto& sampler : mat.samplers) {
        auto it = texByName.find(sampler.name);
        sampler.glTexID = (it != texByName.end()) ? it->second->texID : 0;
        sampler.width = it->second->width;
        sampler.height = it->second->height;

        glBindTexture(GL_TEXTURE_2D, sampler.glTexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ConvertGXWrap((uint8_t)sampler.wrapS));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ConvertGXWrap((uint8_t)sampler.wrapT));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ConvertGXMinFilter((uint8_t)sampler.minFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, ConvertGXMagFilter((uint8_t)sampler.magFilter));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_LOD_BIAS, sampler.lodBias / 16.0f);

        if ((uint8_t)sampler.minFilter >= 2)
            glGenerateMipmap(GL_TEXTURE_2D);
    }

    mat.shaderProgram = BuildMaterialShader(mat, *shp.runtime);

    if (!mat.samplers.empty())
        shp.runtime->texID = mat.samplers[0].glTexID;
};

for (const DrawOp& op : mdl0.sceneGraph.drawOpaOps) processMaterial(op);
for (const DrawOp& op : mdl0.sceneGraph.drawXluOps) processMaterial(op);

auto mdlPtr = std::shared_ptr<MDL0>(rres, &rres->mdl0[modelId]);

auto inst = std::make_shared<ModelInstance>(mdlPtr);

for (auto& srt0 : rres->srt0)
inst->loadedSRT0s.push_back(std::make_unique<SRT0>(srt0));

for (auto& pat0 : rres->pat0)
inst->loadedPAT0s.push_back(std::make_unique<PAT0>(pat0));

for (auto& clr0 : rres->clr0)
inst->loadedCLR0s.push_back(std::make_unique<CLR0>(clr0));

for (auto& chr0 : rres->chr0)
inst->loadedCHR0s.push_back(std::make_unique<CHR0>(chr0));

//for (auto& shp0 : rres->shp0)
//inst->loadedSHP0s.push_back(std::make_unique<SHP0>(shp0));

inst->transform.translation = pos;
inst->transform.rotation = rot;
inst->transform.scale = scale;
instance.push_back(inst);

allLoadedModels.push_back(mdlPtr);
    }
    void bres::Update(float deltaTime, glm::mat4 cameraProj, glm::mat4 view)
    {
        animationController->AddTimeSeconds(deltaTime);
        for (auto& inst : instance)
        {
            inst->Update(deltaTime, cameraProj,view, inst->GetModelMatrix());
        }
    }
    void bres::Renderer(float windowWidth,float windowHeight,glm::mat4 view, glm::mat4 proj, fog::BFG a,bool b)
    {

glm::vec3 cameraPos = glm::vec3(glm::inverse(view)[3]);

        if (instance.empty())
            return;

        for (auto& inst : instance)
        {
            inst->model->updateWorldMatrices(glm::mat4(1.0f));
            inst->model->updateBoneMatrices();

            inst->GatherRenderPackets(packets, view, cameraPos);
        }

        std::sort(
            packets.begin(),
            packets.end(),
            [](const Packet& a, const Packet& b) -> bool {
                return a.sortKey < b.sortKey;
         }
        );

for (auto& p : packets)
{

        MaterialInstance* matInst = p.instance->GetMaterialInstance(p.materialIndex);
        auto& mat = matInst->material;
                glUseProgram(mat->shaderProgram);

                glm::mat4 modelRoot = p.instance->GetModelMatrix();
                glm::mat4 model = modelRoot * p.instance->model->worldMatrices[p.nodeId];

                glUniformMatrix4fv(glGetUniformLocation(mat->shaderProgram, "u_View"),
                    1, GL_FALSE, glm::value_ptr(view));
                glUniformMatrix4fv(glGetUniformLocation(mat->shaderProgram, "u_Proj"),
                    1, GL_FALSE, glm::value_ptr(proj));
                glUniformMatrix4fv(glGetUniformLocation(mat->shaderProgram, "u_Model"),
                    1, GL_FALSE, glm::value_ptr(model));

                    glUseProgram(mat->shaderProgram);

                    glActiveTexture(GL_TEXTURE0 + 8);
                    glBindTexture(GL_TEXTURE_2D, zbufferTex);
                    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_Z"); loc >= 0)
                        glUniform1i(loc, 8);

                    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_ZBuffer"); loc >= 0)
                        glUniform1i(loc, 0);

                    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_DepthFunc"); loc >= 0)
                        glUniform1i(loc, (int)mat->gxMaterial.ropInfo.depthFunc);

                    if (GLint loc = glGetUniformLocation(mat->shaderProgram, "u_ViewportSize"); loc >= 0)
                        glUniform2f(loc, windowWidth, windowHeight);
               
                    if (!a.entries.empty())
                        if (b)
                            UploadFogUniforms(mat->shaderProgram, a.entries[0]);
                        else
                            UploadFogUniforms(mat->shaderProgram, fog::BFGEntry{});
                p.Render(view, proj, cameraPos);
            }   
    }

    void bres::SRT0TexMtxAnimator::CalcTexMtx(glm::mat4& dst)
    {
        _CalcTexMtx(dst, mSrt0->texMtxMode);
    }
    void bres::SRT0TexMtxAnimator::CalcIndTexMtx(glm::mat4& dst)
    {
            float frame = mController->getTimeInFrames();

            float animFrame = GetAnimFrame(*mSrt0, frame);
            float scaleS = mTexData->scaleS
                ? SampleFloatAnimationTrack(*mTexData->scaleS, animFrame)
                : 1.0f;
            float scaleT = mTexData->scaleT
                ? SampleFloatAnimationTrack(*mTexData->scaleT, animFrame)
                : 1.0f;
            float rotation = mTexData->rotation
                ? SampleFloatAnimationTrack(*mTexData->rotation, animFrame)
                : 0.0f;
            float transS = mTexData->translationS
                ? SampleFloatAnimationTrack(*mTexData->translationS, animFrame)
                : 0.0f;
            float transT = mTexData->translationT
                ? SampleFloatAnimationTrack(*mTexData->translationT, animFrame)
                : 0.0f;

            calcTexMtx(dst, TexMatrixMode::Basic, scaleS, scaleT, rotation, transS, transT);
    }
    void bres::SRT0TexMtxAnimator::_CalcTexMtx(glm::mat4& dst, TexMatrixMode mode)
    {
            float frame = mController->getTimeInFrames();

            float animFrame = GetAnimFrame(*mSrt0, frame);
            const auto& base = mMaterial->material->texSrts[mTexMtxIdx];
            float scaleS = mTexData->scaleS
                ? SampleFloatAnimationTrack(*mTexData->scaleS, animFrame) / base.scaleS
                : 1.0f;
            float scaleT = mTexData->scaleT
                ? SampleFloatAnimationTrack(*mTexData->scaleT, animFrame) / base.scaleT
                : 1.0f;
            float rotation = mTexData->rotation
                ? SampleFloatAnimationTrack(*mTexData->rotation, animFrame) - base.rotation
                : 0.0f;
            float transS = mTexData->translationS
                ? SampleFloatAnimationTrack(*mTexData->translationS, animFrame) - base.translationS
                : 0.0f;
            float transT = mTexData->translationT
                ? SampleFloatAnimationTrack(*mTexData->translationT, animFrame) - base.translationT
                : 0.0f;

            calcTexMtx(dst, mode, scaleS, scaleT, rotation, transS, transT);
        
    }
    
void bres::CLR0ColorAnimator::CalcColor(Color& dst, const Color& orig)
{
    float frame = mController->getTimeInFrames();
    if (mCLR0->loopMode == LoopMode::REPEAT)
        frame = fmodf(frame, mCLR0->duration);
    else
        frame = std::clamp(frame, 0.0f, mCLR0->duration);

    float animFrame = GetAnimFrame(*mCLR0, frame);

    uint32_t animRGBA = SampleAnimationTrackColor(mClrData->frames, animFrame);
    uint32_t origRGBA = ColorToRGBA8(orig);

    uint32_t c = (origRGBA & ~mClrData->mask) | (animRGBA & mClrData->mask);
    ColorFromRGBA8(dst, c);
}


bool bres::CHR0NodesAnimator::CalcModelMtx(glm::mat4& dst, size_t nodeId)
{
    if (nodeId >= mNodeData.size() || disabled[nodeId])
        return false;

    const auto& node = mNodeData[nodeId];
    float frame = mController->getTimeInFrames();
    float animFrame = GetAnimFrame(*mCHR0, frame);

    auto sample = [&](const std::optional<FloatAnimationTrack>& track, float def = 0.0f) {
        return track ? SampleFloatAnimationTrack(*track, animFrame) : def;
    };

    float scaleX = sample(node.scaleX, 1.0f);
    float scaleY = sample(node.scaleY, 1.0f);
    float scaleZ = sample(node.scaleZ, 1.0f);

    float rotationX = glm::radians(sample(node.rotationX));
    float rotationY = glm::radians(sample(node.rotationY));
    float rotationZ = glm::radians(sample(node.rotationZ));

    float transX = sample(node.translationX);
    float transY = sample(node.translationY);
    float transZ = sample(node.translationZ);

    dst = computeModelMatrixSRT(
        scaleX, scaleY, scaleZ,
        rotationX, rotationY, rotationZ,
        transX, transY, transZ
    );

    return true;
}

bres::~bres()
{
    glDeleteTextures(1, &zbufferTex);
    glDeleteTextures(1, &id);
    zbufferTex = 0;
    id = 0;
}