#pragma once
#include "GX/GXMaterial.h"
#include "bstream.h"
#include "cache.h"
#include "sstream"
class posteffects {
public:
    posteffects();
    ~posteffects();
    struct BBLM {
        float thresholdAmount;
        Color thresholdColor;
        Color compositeColor;
        uint16_t blurFlags;
        float blur0Radius;
        float blur0Intensity;
        float blur1Radius;
        float blur1Intensity;
        uint8_t compositeBlendMode;
        uint8_t blur1NumPasses;
        float bokehColorScale0;
        float bokehColorScale1;
    };
    struct BDOF {
        uint16_t flags;
        uint8_t blurAlpha[2];
        uint8_t drawMode;
        uint8_t blurDrawAmount;
        uint8_t depthCurveType;
        float focusCenter;
        float focusRange;
        float blurRadius;
        float indTexTransSScroll;
        float indTexTransTScroll;
        float indTexIndScaleS;
        float indTexIndScaleT;
        float indTexScaleS;
        float indTexScaleT;
    };
    const char* fullscreenVS = R"(
#version 330 core
out vec2 uv;

void main() {
    uv.x = (gl_VertexID == 1) ? 2.0 : 0.0;
    uv.y = (gl_VertexID == 2) ? 2.0 : 0.0;

    gl_Position.xy = uv * 2.0 - 1.0;
    gl_Position.z  = 1.0;
    gl_Position.w = 1.0;
}
)";
    const char* fullscreenVS2 = R"(
#version 330 core
out vec2 uv;
uniform float u_FocusZClipSpace;

void main() {
    uv.x = (gl_VertexID == 1) ? 2.0 : 0.0;
    uv.y = (gl_VertexID == 2) ? 2.0 : 0.0;

    gl_Position.xy = uv * 2.0 - 1.0;
    gl_Position.z  = u_FocusZClipSpace;
    gl_Position.w = 1.0;
}
)";
    GLuint thresholdProgram = CreateShaderProgram(fullscreenVS, R"(
#version 330 core
uniform sampler2D u_Texture;
uniform vec4 u_ThresholdColor;
in vec2 uv;
out vec4 o_Color;
vec3 saturate(vec3 v) { return clamp(v, vec3(0.0), vec3(1.0)); }
float GXIntensity(vec3 t_Color) {
    // https://github.com/dolphin-emu/dolphin/blob/4cd48e609c507e65b95bca5afb416b59eaf7f683/Source/Core/VideoCommon/TextureConverterShaderGen.cpp#L237-L241
    return dot(t_Color, vec3(0.257, 0.504, 0.098)) + 16.0/255.0;
}
void main() {
    vec4 c = texture(u_Texture, uv);
    o_Color.rgb = c.rgb * (2.0 * (saturate(vec3(GXIntensity(c.rgb)) - u_ThresholdColor.rgb)));
    o_Color.a = 1.0;
}
)");
    GLuint blitProgram = CreateShaderProgram(fullscreenVS, R"(
#version 330 core
uniform sampler2D u_Texture;
in vec2 uv;
out vec4 o_Color;
void main() {
    o_Color = texture(u_Texture, uv);
}
)");
    GLuint blur0Program;
    GLuint blur1Program;
    GLuint compositeProgram;
    GLuint bdofBlitProgram = CreateShaderProgram(fullscreenVS2, R"(
#version 330 core

uniform sampler2D u_Texture;

in vec2 uv;
out vec4 o_Color;

void main() {
    o_Color = texture(u_Texture, uv);
}
)");
    GLuint bdofProgram;
    GLuint bdofProgram2;
    bool useIndWarpTex;
    GLuint mViewTex;
    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    GLuint posteffectVAO = 0;
    GLuint posteffectVBO = 0;
    GLuint posteffectVAO2 = 0;
    GLuint bloomFbo2, bloomTex2;
    GLuint bloomFbo4, bloomTex4;
    GLuint bloomFbo8, bloomTex8;

    GLuint bloomTex4A, bloomTex4B;
    GLuint bloomFbo4A, bloomFbo4B;

    GLuint bloomTex8A, bloomTex8B;
    GLuint bloomFbo8A, bloomFbo8B;

    GLuint dofFboA;
    GLuint dofFboB;
    GLuint dofTexA;
    GLuint dofTexB;

    Color currentThresholdColor;

    posteffects::BBLM ParseBBLM(bStream::CStream* stream);
    std::string GenerateBlurFunction(const std::string& funcName, int tapCount, float radius, float intensity);
    std::string GenerateBloomBlurFrag(int tapCount, int numPasses, float radius, float intensity);
    std::string GenerateBloomCombineFrag(int numPasses);
    posteffects::BDOF ParseBDOF(bStream::CStream* stream);
    std::string GenerateDOFBlurFrag(int tapCount, float radius);
    std::string GenerateDOFCombineFrag(bool useIndWarpTex);
    void InitFbo(int width, int height);
    void Render(int fbw, int fbh, GLuint mFbo, BBLM loadedBBLM, GLuint View);
    void updateBDOFUniforms(glm::mat4 pm, float time, BDOF bdof);
    void LoadBBLM(BBLM bblm);
    void LoadBDOF(BDOF bdof);
};