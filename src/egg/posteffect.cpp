#include "egg/posteffect.h"
#include <iomanip>

inline void colorFromRGBA8(Color& dst, uint32_t n)
{
    dst.r = float((n >> 24) & 0xFF) / 255.0f;
    dst.g = float((n >> 16) & 0xFF) / 255.0f;
    dst.b = float((n >> 8) & 0xFF) / 255.0f;
    dst.a = float((n >> 0) & 0xFF) / 255.0f;
}

inline Color colorNewFromRGBA8_UINT32(uint32_t n)
{
    Color dst{};
    colorFromRGBA8(dst, n);
    return dst;
}

posteffects::BBLM posteffects::ParseBBLM(bStream::CStream* stream)
{
    stream->seek(0x08);
    uint8_t version = stream->readUInt8();
    if (version != 1)
        throw std::runtime_error("Unsupported BBLM version");

    BBLM out{};
    stream->seek(0x10);
    out.thresholdAmount = stream->readFloat();
    out.thresholdColor = colorNewFromRGBA8_UINT32(stream->readUInt32());
    out.compositeColor = colorNewFromRGBA8_UINT32(stream->readUInt32());
    out.blurFlags = stream->readUInt16();
    stream->seek(0x20);
    out.blur0Radius = stream->readFloat();
    out.blur0Intensity = stream->readFloat();
    stream->seek(0x40);
    out.blur1Radius = stream->readFloat();
    out.blur1Intensity = stream->readFloat();
    stream->seek(0x80);
    out.compositeBlendMode = stream->readUInt8();
    out.blur1NumPasses = stream->readUInt8();
    stream->seek(0x9C);
    out.bokehColorScale0 = stream->readFloat();
    out.bokehColorScale1 = stream->readFloat();

    return out;
}
std::string posteffects::GenerateBlurFunction(
    const std::string& funcName,
    int tapCount,
    float radius,
    float intensityPerTap)
{
    if (!std::isfinite(radius) || radius <= 0.0f)
        radius = 0.0001f;
    if (!std::isfinite(intensityPerTap))
        intensityPerTap = 0.0f;

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);

    ss << "vec3 " << funcName << "(sampler2D tex, vec2 uv, vec2 aspect) {\n";
    ss << "    vec3 c = vec3(0.0);\n";

    ss << "    const float TAU = 6.28318530717958648;\n";
    ss << "    for (int i = 0; i < " << tapCount << "; i++) {\n";
    ss << "        float theta = TAU * (float(i) / " << tapCount << ".0);\n";
    ss << "        float x = cos(theta);\n";
    ss << "        float y = -sin(theta);\n";
    ss << "        c += texture(tex, uv + aspect * vec2(x * " << radius
        << ", y * " << radius << ")).rgb * " << intensityPerTap << ";\n";
    ss << "    }\n";

    ss << "    return c;\n";
    ss << "}\n";
    return ss.str();
}
std::string posteffects::GenerateBloomBlurFrag(
    int tapCount,
    int numPasses,
    float radius,
    float intensity)
{
    std::ostringstream funcs;
    std::ostringstream code;

    float intensityPerTap = intensity / tapCount * 8.0f;



    for (int i = 0; i < numPasses; i++) {
        int passTapCount = (i + 1) * tapCount;
        std::string funcName = "BlurPass" + std::to_string(i);

        funcs << GenerateBlurFunction(funcName, passTapCount, radius, intensityPerTap);

        code << "    c += saturate(" << funcName
            << "(u_Texture, uv, t_Aspect));\n";
    }

    std::ostringstream frag;

    frag <<
        R"(#version 330 core
uniform sampler2D u_Texture;
in vec2 uv;
out vec4 o_Color;

vec3 saturate(vec3 v) { return clamp(v, vec3(0.0), vec3(1.0)); }

)" << funcs.str() << R"(

void main() {
    vec2 t_Size = vec2(textureSize(u_Texture, 0));
    vec2 t_Aspect = vec2(1.0) / t_Size;

    vec3 c = vec3(0.0);
)" << code.str() << R"(
    o_Color = vec4(saturate(c), 1.0);
}
)";
    return frag.str();
}
std::string posteffects::GenerateBloomCombineFrag(int numPasses)
{
    std::ostringstream frag;

    frag <<
        R"(#version 330 core
uniform sampler2D u_Texture;
uniform sampler2D u_Texture2;
uniform vec4 u_CompositeColor;
uniform float u_CompositeColorScale[2];

in vec2 uv;
out vec4 o_Color;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, 0.0, 1.0); }

void main() {
    vec3 c = vec3(0.0);
    c += saturate(texture(u_Texture, uv).rgb) * u_CompositeColor.rgb * u_CompositeColorScale[0];
)";

    if (numPasses >= 2) {
        frag <<
            R"(
    c += saturate(texture(u_Texture2, uv).rgb) * u_CompositeColor.rgb * u_CompositeColorScale[1];
)";
    }

    frag <<
        R"(
    o_Color = vec4(saturate(c), 1.0);
}
)";
    return frag.str();
}


posteffects::BDOF posteffects::ParseBDOF(bStream::CStream* stream)
{
    stream->seek(0x08);
    uint8_t version = stream->readUInt8();
    if (version != 0)
        throw std::runtime_error("Unsupported BDOF version");

    BDOF out{};
    stream->seek(0x10);
    out.flags = stream->readUInt16();
    out.blurAlpha[0] = stream->readUInt8();
    out.blurAlpha[1] = stream->readUInt8();
    out.drawMode = stream->readUInt8();
    out.blurDrawAmount = stream->readUInt8();
    out.depthCurveType = stream->readUInt8();
    stream->seek(0x18);
    out.focusCenter = stream->readFloat();
    out.focusRange = stream->readFloat();
    stream->seek(0x24);
    out.blurRadius = stream->readFloat();
    out.indTexTransSScroll = stream->readFloat();
    out.indTexTransTScroll = stream->readFloat();
    out.indTexIndScaleS = stream->readFloat();
    out.indTexIndScaleT = stream->readFloat();
    out.indTexScaleS = stream->readFloat();
    out.indTexScaleT = stream->readFloat();

    return out;
};

std::string posteffects::GenerateDOFBlurFrag(int tapCount, float radius)
{
    float intensityPerTap = 1.0f / tapCount;

    std::string blurFunc = GenerateBlurFunction(
        "BlurPass0",
        tapCount,
        radius,
        intensityPerTap
    );

    std::ostringstream frag;

    frag <<
        R"(#version 330 core
uniform sampler2D u_Texture;
in vec2 uv;
out vec4 o_Color;

float saturate(float x) { return clamp(x, 0.0, 1.0); }
vec3 saturate(vec3 x) { return clamp(x, 0.0, 1.0); }
)" << blurFunc << R"(

void main() {
    vec2 t_Size = vec2(textureSize(u_Texture, 0));
    vec2 t_Aspect = vec2(1.0) / t_Size;

    vec3 c = BlurPass0(u_Texture, uv, t_Aspect);
    o_Color = vec4(saturate(c), 1.0);
}
)";
    return frag.str();
}
std::string posteffects::GenerateDOFCombineFrag(bool useIndWarpTex)
{
    std::ostringstream frag;

    frag <<
        R"(#version 330 core
uniform sampler2D u_Texture;
uniform sampler2D u_Texture2;

struct Mat2x4 { vec4 mx; vec4 my; };

uniform Mat2x4 u_IndTexMat;
uniform vec4 u_Misc0;

#define u_FocusZClipSpace (u_Misc0.x)
#define u_IndTexIndScale  (u_Misc0.yz)

in vec2 uv;
out vec4 o_Color;

mat4x2 UnpackMatrix(Mat2x4 m) {
    return mat4x2(transpose(mat4(m.mx, m.my, vec4(0,0,0,0), vec4(0,0,0,1))));
}

void main() {
    vec2 t_TexCoord = uv;
)";

    if (useIndWarpTex)
    {
        frag <<
            R"(
    vec2 t_WarpTexCoord = UnpackMatrix(u_IndTexMat) * vec4(uv, 0.0, 1.0);
    vec2 t_IndTexOffs = ((255.0 * texture(u_Texture2, t_WarpTexCoord).ba) - 128.0) * u_IndTexIndScale;
    t_TexCoord += t_IndTexOffs;
)";
    }

    frag <<
        R"(
    o_Color = vec4(texture(u_Texture, t_TexCoord).rgb, 1.0);
}
)";
    return frag.str();
}

void posteffects::InitFbo(int width,int height)
{
	glGenFramebuffers(1, &bloomFbo2);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo2);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	glGenTextures(1, &bloomTex2);
	glBindTexture(GL_TEXTURE_2D, bloomTex2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 2, height / 2, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloomTex2, 0);

	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}

	glGenFramebuffers(1, &bloomFbo4A);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo4A);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	glGenTextures(1, &bloomTex4A);
	glBindTexture(GL_TEXTURE_2D, bloomTex4A);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 4, height / 4, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloomTex4A, 0);

	glGenFramebuffers(1, &bloomFbo4B);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo4B);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	glGenTextures(1, &bloomTex4B);
	glBindTexture(GL_TEXTURE_2D, bloomTex4B);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 4, height / 4, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloomTex4B, 0);

	glGenFramebuffers(1, &bloomFbo8A);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo8A);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	glGenTextures(1, &bloomTex8A);
	glBindTexture(GL_TEXTURE_2D, bloomTex8A);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 8, height / 8, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloomTex8A, 0);

	glGenFramebuffers(1, &bloomFbo8B);
	glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo8B);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}
	glGenTextures(1, &bloomTex8B);
	glBindTexture(GL_TEXTURE_2D, bloomTex8B);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 8, height / 8, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, bloomTex8B, 0);

	glGenFramebuffers(1, &dofFboA);
	glBindFramebuffer(GL_FRAMEBUFFER, dofFboA);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}

	glGenTextures(1, &dofTexA);
	glBindTexture(GL_TEXTURE_2D, dofTexA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 2, height / 2, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, dofTexA, 0);

	glGenFramebuffers(1, &dofFboB);
	glBindFramebuffer(GL_FRAMEBUFFER, dofFboB);
	{
		GLenum bufs[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, bufs);
	}

	glGenTextures(1, &dofTexB);
	glBindTexture(GL_TEXTURE_2D, dofTexB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width / 2, height / 2, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, dofTexB, 0);
}

void posteffects::Render(int fbw,int fbh,GLuint fbo, BBLM loadedBBLM, GLuint View)
{

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glBindVertexArray(posteffectVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, View);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	if (bdofProgram != 0)
	{
		glBindVertexArray(posteffectVAO2);
		glDisable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ZERO);

		glBindFramebuffer(GL_FRAMEBUFFER, dofFboA);
		glViewport(0, 0, fbw / 2, fbh / 2);

		glUseProgram(bdofBlitProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, View);
		glUniform1i(glGetUniformLocation(bdofBlitProgram, "u_Texture"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, dofFboB);
		glViewport(0, 0, fbw / 2, fbh / 2);

		glUseProgram(bdofProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dofTexA);
		glUniform1i(glGetUniformLocation(bdofProgram, "u_Texture"), 0);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, fbw, fbh);

		glUseProgram(bdofProgram2);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, dofTexB);
		glUniform1i(glGetUniformLocation(bdofProgram2, "u_Texture"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, useIndWarpTex ? mViewTex : 0);
		glUniform1i(glGetUniformLocation(bdofProgram2, "u_Texture2"), 1);
	}
	if (blur0Program != 0)
	{
		glBindVertexArray(posteffectVAO);
		glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo2);
		glViewport(0, 0, fbw / 2, fbh / 2);

		glUseProgram(thresholdProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, View);
		glUniform1i(glGetUniformLocation(thresholdProgram, "u_Texture"), 0);
		glUniform4f(glGetUniformLocation(thresholdProgram, "u_ThresholdColor"),
			currentThresholdColor.r, currentThresholdColor.g,
			currentThresholdColor.b, currentThresholdColor.a);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo4A);
		glViewport(0, 0, fbw / 4, fbh / 4);

		glUseProgram(blitProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomTex2);
		glUniform1i(glGetUniformLocation(blitProgram, "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo4B);
		glViewport(0, 0, fbw / 4, fbh / 4);

		glUseProgram(blur0Program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomTex4A);
		glUniform1i(glGetUniformLocation(blur0Program, "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo8A);
		glViewport(0, 0, fbw / 8, fbh / 8);

		glUseProgram(blitProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomTex4B);
		glUniform1i(glGetUniformLocation(blitProgram, "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, bloomFbo8B);
		glViewport(0, 0, fbw / 8, fbh / 8);

		glUseProgram(blur1Program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomTex8A);
		glUniform1i(glGetUniformLocation(blur1Program, "u_Texture"), 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, fbw, fbh);

		glUseProgram(compositeProgram);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, bloomTex4B);
		glUniform1i(glGetUniformLocation(compositeProgram, "u_Texture"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, bloomTex8B);
		glUniform1i(glGetUniformLocation(compositeProgram, "u_Texture2"), 1);

		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);

		switch (loadedBBLM.compositeBlendMode) {
		case 0:
			glBlendFunc(GL_ONE, GL_ONE);
			break;
		case 1:
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
			break;
		case 2:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case 3:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			break;
		case 4:
			glBlendFunc(GL_DST_COLOR, GL_ONE);
			break;
		}
		glUniform4f(glGetUniformLocation(compositeProgram, "u_CompositeColor"),
			loadedBBLM.compositeColor.r,
			loadedBBLM.compositeColor.g,
			loadedBBLM.compositeColor.b,
			loadedBBLM.compositeColor.a);

		GLint locScale = glGetUniformLocation(compositeProgram, "u_CompositeColorScale");
		float scales[2] = { loadedBBLM.bokehColorScale0, loadedBBLM.bokehColorScale1 };
		glUniform1fv(locScale, 2, scales);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
}
static inline void transformVec3ByMat4(glm::vec3& out, const glm::vec3& v, const glm::mat4& m)
{
	glm::vec4 t = m * glm::vec4(v, 1.0f);
	out = glm::vec3(t);
}
void posteffects::updateBDOFUniforms(glm::mat4 pm, float time, BDOF bdof)
{
	float focusZ = (bdof.focusCenter + bdof.focusRange * 0.5f) * 0.1f;

	glm::vec3 clip = { 0,0,-focusZ };
	glm::vec3 transformed;
	transformVec3ByMat4(transformed, clip, pm);
	float focusZClipSpace = transformed.z;

	float indTexShift = 1.0f / 64.0f * 0.5f;
	float indScaleS = (bdof.indTexIndScaleS / 832.0f) * indTexShift;
	float indScaleT = (bdof.indTexIndScaleT / 456.0f) * indTexShift * -1.0f;

	float misc0[4] = {
		focusZClipSpace,
		indScaleS,
		indScaleT,
		0.0f
	};

	glUseProgram(bdofProgram2);

	GLint loc_mx = glGetUniformLocation(bdofProgram2, "u_IndTexMat.mx");
	GLint loc_my = glGetUniformLocation(bdofProgram2, "u_IndTexMat.my");

	float mx[4] = {
		bdof.indTexScaleS, 0.0f, 0.0f, bdof.indTexTransSScroll * time
	};

	float my[4] = {
		0.0f, bdof.indTexScaleT, 0.0f, bdof.indTexTransTScroll * time * -1.0f
	};

	glUniform4fv(loc_mx, 1, mx);
	glUniform4fv(loc_my, 1, my);


	GLint locMisc = glGetUniformLocation(bdofProgram2, "u_Misc0");
	glUniform4fv(locMisc, 1, misc0);

	GLint locFocusZClipSpace = glGetUniformLocation(bdofProgram2, "u_FocusZClipSpace");
	glUniform1f(locFocusZClipSpace, focusZClipSpace);
}
void posteffects::LoadBBLM(BBLM bblm)
{
	blur0Program = CreateShaderProgram(
		fullscreenVS, GenerateBloomBlurFrag(
			8,
			1,
			bblm.blur0Radius,
			bblm.blur0Intensity
		).c_str()
	);

	blur1Program = CreateShaderProgram(
		fullscreenVS, GenerateBloomBlurFrag(
			8,
			bblm.blur1NumPasses,
			bblm.blur1Radius,
			bblm.blur1Intensity
		).c_str()
	);
	const int numPasses = (bblm.blurFlags & 1) ? 2 : 1;
	compositeProgram = CreateShaderProgram(
		fullscreenVS, GenerateBloomCombineFrag(numPasses).c_str()
	);
	float scale = ((int)(bblm.thresholdAmount * 219.0f + 16.0f)) / 255.0f;

	currentThresholdColor.r = bblm.thresholdColor.r * scale;
	currentThresholdColor.g = bblm.thresholdColor.g * scale;
	currentThresholdColor.b = bblm.thresholdColor.b * scale;
	currentThresholdColor.a = 1.0f;

	if (bblm.blurFlags & 0x10)
		currentThresholdColor.a = 0.0f;
}
int GetBlurTapCount(int drawAmount) {
	static const int table[] = { 4, 2 };
	if (drawAmount < 0 || drawAmount >= 2)
		return 4;
	return table[drawAmount];
}
void posteffects::LoadBDOF(BDOF bdof)
{
	int tapCount = GetBlurTapCount(bdof.blurDrawAmount);
	bdofProgram = CreateShaderProgram(
		fullscreenVS, GenerateDOFBlurFrag(tapCount, bdof.blurRadius).c_str()
	);

	useIndWarpTex = (bdof.flags & 2) != 0;
	bdofProgram2 = CreateShaderProgram(
		fullscreenVS, GenerateDOFCombineFrag(useIndWarpTex).c_str()
	);
}
posteffects::posteffects()
{
	glGenVertexArrays(1, &posteffectVAO);
	glGenVertexArrays(1, &posteffectVAO2);
}
posteffects::~posteffects()
{
	if (posteffectVBO) {
		glDeleteBuffers(1, &posteffectVBO);
		posteffectVBO = 0;
	}
	if (posteffectVAO) {
		glDeleteVertexArrays(1, &posteffectVAO);
		posteffectVAO = 0;
	}
	if (posteffectVAO2) {
		glDeleteVertexArrays(1, &posteffectVAO2);
		posteffectVAO2 = 0;
	}
	glDeleteTextures(1, &mViewTex);
	glDeleteTextures(1, &bloomTex2);
	glDeleteTextures(1, &bloomTex4);
	glDeleteTextures(1, &bloomTex8);
	glDeleteTextures(1, &bloomTex4B);
	glDeleteTextures(1, &bloomTex4A);
	glDeleteTextures(1, &bloomTex8B);
	glDeleteTextures(1, &bloomTex8A);
	glDeleteTextures(1, &dofTexA);
	glDeleteTextures(1, &dofTexB);
	glDeleteFramebuffers(1, &bloomFbo2);
	glDeleteRenderbuffers(1, &bloomFbo4);
	glDeleteFramebuffers(1, &bloomFbo8);
	glDeleteRenderbuffers(1, &bloomFbo4A);
	glDeleteFramebuffers(1, &bloomFbo4B);
	glDeleteRenderbuffers(1, &bloomFbo8A);
	glDeleteRenderbuffers(1, &bloomFbo8B);
	glDeleteRenderbuffers(1, &dofFboA);
	glDeleteRenderbuffers(1, &dofFboB);
	mViewTex = 0;
	bloomTex2 = 0;
	bloomTex4 = 0;
	bloomTex8 = 0;
	bloomTex4A = 0;
	bloomTex4B = 0;
	bloomTex8A = 0;
	bloomTex8B = 0;
	dofTexA = 0;
	dofTexB = 0;
	bloomFbo2 = 0;
	bloomFbo4 = 0;
	bloomFbo8 = 0;
	bloomFbo4A = 0;
	bloomFbo4B = 0;
	bloomFbo8A = 0;
	bloomFbo8B = 0;
	dofFboA = 0;
	dofFboB = 0;
}