#pragma once
#include "GX/GXMaterial.h"
#include <glm/gtc/type_ptr.hpp>
#include "cache.h"
#include <algorithm>
class lyt {
public:
    struct TDVertex {
        glm::vec3 pos;
        glm::vec2 uv;
        Color color;
    };
    class TDDraw {
    public:
        std::vector<TDVertex> vertices;

        GLuint vao = 0;
        GLuint vbo = 0;

        TDDraw() {
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                sizeof(TDVertex), (void*)offsetof(TDVertex, pos));

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                sizeof(TDVertex), (void*)offsetof(TDVertex, uv));

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
                sizeof(TDVertex), (void*)offsetof(TDVertex, color));
        }

        void begin() {
        }

        void position3f32(float x, float y, float z) {
            current.pos = { x, y, z };
        }

        void texCoord2f32(float s, float t) {
            current.uv = { s, t };
        }

        void color4(const Color& c) {
            current.color = { c.r, c.g, c.b, c.a };
        }

        void end() {
            vertices.push_back(current);
        }

        bool hasIndicesToDraw() const {
            return !vertices.empty();
        }

        void flush() {
            if (vertices.empty())
                return;

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);

            glBufferData(GL_ARRAY_BUFFER,
                vertices.size() * sizeof(TDVertex),
                vertices.data(),
                GL_DYNAMIC_DRAW);

            glDrawArrays(GL_TRIANGLES, 0, vertices.size());

            vertices.clear();
        }

    private:
        TDVertex current;
    };
    struct MipChain;
    struct MipLevel;
    struct LoadedTexture {
        GLuint gfxTexture = 0;
    };

    static LoadedTexture loadTextureFromMipChain(const MipChain& mipChain) {
        LoadedTexture out{};

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);

        const MipLevel& base = mipChain.levels[0];
        glTexImage2D(
            GL_TEXTURE_2D,
            0,                  // mip level
            GL_RGBA8,           // internal format
            base.width,
            base.height,
            0,                  // border
            GL_RGBA,            // data format
            GL_UNSIGNED_BYTE,   // data type
            base.data.data()
        );

        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        out.gfxTexture = tex;

        return out;
    }
    class GXMaterialHelperGL {
    public:
        GLuint program = 0;

        GLint u_Color0 = -1;
        GLint u_Color1 = -1;
        GLint u_FontTex = -1;
        GLint u_MVP = -1;
        GLuint compileShader(GLenum type, const std::string& source)
        {
            const char* src = source.c_str();
            GLuint shader = glCreateShader(type);
            glShaderSource(shader, 1, &src, nullptr);
            glCompileShader(shader);

            GLint success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success) {
                char infoLog[2048];
                glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);

                std::string typeStr =
                    (type == GL_VERTEX_SHADER) ? "VERTEX" :
                    (type == GL_FRAGMENT_SHADER) ? "FRAGMENT" : "UNKNOWN";

                std::cerr << "[Shader Compile Error] " << typeStr << "\n";
                std::cerr << infoLog << "\n";

                glDeleteShader(shader);
                return 0;
            }

            return shader;
        }
        GXMaterialHelperGL(const GXMaterial& material) {
            std::string vertSrc = R"(#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in vec4 a_Color;

out vec2 v_TexCoord;
out vec4 v_Color;

uniform mat4 u_MVP;

void main() {
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
})";
            std::string fragSrc = R"(#version 330 core
in vec2 v_TexCoord;
in vec4 v_Color;

uniform sampler2D u_FontTex;
uniform vec4 u_Color0;
uniform vec4 u_Color1;

out vec4 o_Color;

void main() {
vec4 tex = texture(u_FontTex, v_TexCoord);
float a = tex.r;

vec3 rgb = mix(u_Color0.rgb, u_Color1.rgb, a);

o_Color = vec4(rgb, a);
})";
            std::string key = vertSrc + "\n---\n" + fragSrc;
            size_t hash = std::hash<std::string>{}(key);
            GLuint vs;
            GLuint fs;
            auto it = shaderCache.find(hash);
            if (it != shaderCache.end())
            {
                program = it->second;
                goto so;
            }
            program = glCreateProgram();
            vs = compileShader(GL_VERTEX_SHADER, vertSrc);
            fs = compileShader(GL_FRAGMENT_SHADER, fragSrc);
            glAttachShader(program, vs);
            glAttachShader(program, fs);
            glLinkProgram(program);

            glDeleteShader(vs);
            glDeleteShader(fs);
            so:
            u_Color0 = glGetUniformLocation(program, "u_Color0");
            u_Color1 = glGetUniformLocation(program, "u_Color1");
            u_FontTex = glGetUniformLocation(program, "u_FontTex");
            u_MVP = glGetUniformLocation(program, "u_MVP");
        }

        void bind(const glm::mat4& mvp, const Color& c0, const Color& c1) {
            glUseProgram(program);

            glUniformMatrix4fv(u_MVP, 1, GL_FALSE, glm::value_ptr(mvp));
            glUniform4f(u_Color0, c0.r, c0.g, c0.b, c0.a);
            glUniform4f(u_Color1, c1.r, c1.g, c1.b, c1.a);

            glUniform1i(u_FontTex, 0);
        }
    };
    enum class RFNTGlyphType : uint8_t {
        Glyph,
        Texture,
    };

    enum class RFNTEncoding : uint8_t {
        UTF8,
        UTF16,
        SJIS,
        CP1252,
    };

    enum class RFNTCMAPKind : uint16_t {
        Offset = 0,
        Array = 1,
        Dict = 2,
    };

    struct RFNTCWDHEntry {
        int8_t  leftSideBearing;
        uint8_t width;
        int8_t  advanceWidth;
    };

    struct GlyphInfo {
        uint16_t textureIndex;
        RFNTCWDHEntry cwdh;
        float s0;
        float t0;
        float s1m;
        float s1b;
        float t1;
    };

    struct RFNTFINF {
        uint8_t  advanceHeight;
        RFNTEncoding encoding;
        uint8_t  width;
        uint8_t  height;
        uint8_t  ascent;
        uint16_t defaultGlyphIndex;
    };

    struct RFNTTGLPTexture {
        std::string name;
        uint16_t width;
        uint16_t height;
        uint8_t format;
        std::vector<uint8_t> data;
        uint32_t mipCount;
    };

    struct RFNTTGLP {
        uint8_t glyphBaseline;
        std::vector<RFNTTGLPTexture> textures;
    };

    struct RFNT : RFNTFINF, RFNTTGLP {
        std::string name;
        std::array<uint16_t, 0x10000> cmap;
        std::vector<GlyphInfo> glyphInfo;
        uint8_t cellWidth()  const { return width; }
        uint8_t cellHeight() const { return height; }
    };
    struct MipLevel {
        int width;
        int height;
        std::vector<uint8_t> data; // RGBA8
    };

    struct MipChain {
        std::vector<MipLevel> levels;
    };
    static std::vector<uint8_t> decodeGXTextureToRGBA8(const RFNTTGLPTexture& tex) {
        bStream::CMemoryStream texData(const_cast<uint8_t*>(tex.data.data()),tex.data.size(), bStream::Endianess::Big, bStream::OpenMode::In);
        std::vector<uint8_t> rgba(tex.width * tex.height * 4);
        switch (tex.format) {
        case 0x00:    DecodeI4(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x01:    DecodeI8(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x02:    DecodeIA4(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x03:   DecodeIA8(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x04: DecodeRGB565(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x05: DecodeRGB5A3(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x06: DecodeRGBA32(&texData, tex.width, tex.height, rgba.data()); break;
        case 0x0E:  DecodeCMPR(&texData, tex.width, tex.height, rgba.data()); break;
        default:
            break;
        }
        return rgba;
    }
    class ResFont {
    public:
        RFNT rfnt;
        std::optional<GXMaterialHelperGL> materialHelper;
        std::vector<GLuint> gfxTextures;
        int glyphCellW = 0;
        int glyphCellH = 0;
        MipChain calcMipChain(const RFNTTGLPTexture& tex) {
            MipChain chain;

            MipLevel base{};

            base.width = tex.width;
            base.height = tex.height;
            base.data = decodeGXTextureToRGBA8(tex); 

            chain.levels.push_back(std::move(base));

            return chain;
        }
        ResFont(const RFNT& rfnt_) : rfnt(rfnt_) {
            for (size_t i = 0; i < rfnt.textures.size(); i++) {
                const auto& tex = rfnt.textures[i];

                auto mipChain = calcMipChain(tex);
                auto loaded = loadTextureFromMipChain(mipChain);
                gfxTextures.push_back(loaded.gfxTexture);
            }


            GXMaterialBuilder mb(rfnt.name);

            mb.setChanCtrl(
                EGXColorChannelId::Color0A0,
                false,
                EGXColorSource::Register,
                EGXColorSource::Vertex,
                0,
                EGXDiffuseFunction::None,
                EGXAttenuationFunction::None
            );

            mb.setTexCoordGen(
                EGXTexCoordSlot::TexCoord0,
                EGXTexGenType::Matrix2x4,
                EGXTexGenSrc::Tex0,
                EGXTexMatrix::Identity
            );

            // TEV stage 0
            mb.setTevOrder(0,
                EGXTexCoordSlot::TexCoord0,
                EGXTexMapSlot::TexMap0,
                EGXRasColorChannelSlot::COLOR_ZERO
            );
            mb.setTevColorIn(0, EGXCombineColorInput::C0, EGXCombineColorInput::C1, EGXCombineColorInput::TexColor, EGXCombineColorInput::Zero);
            mb.setTevAlphaIn(0, EGXCombineAlphaInput::A0, EGXCombineAlphaInput::A1, EGXCombineAlphaInput::TexAlpha, EGXCombineAlphaInput::Zero);
            mb.setTevColorOp(0, EGXTevOp::Add, EGXTevBias::Zero, EGXTevScale::Scale_1, true, EGXTevRegister::Prev);
            mb.setTevAlphaOp(0, EGXTevOp::Add, EGXTevBias::Zero, EGXTevScale::Scale_1, true, EGXTevRegister::Prev);

            // TEV stage 1
            mb.setTevOrder(1,
                EGXTexCoordSlot::TexCoord0,
                EGXTexMapSlot::Null,
                EGXRasColorChannelSlot::COLOR0A0
            );
            mb.setTevColorIn(1, EGXCombineColorInput::Zero, EGXCombineColorInput::ColorPrev, EGXCombineColorInput::RasColor, EGXCombineColorInput::Zero);
            mb.setTevAlphaIn(1, EGXCombineAlphaInput::Zero, EGXCombineAlphaInput::AlphaPrev, EGXCombineAlphaInput::RasAlpha, EGXCombineAlphaInput::Zero);
            mb.setTevColorOp(0, EGXTevOp::Add, EGXTevBias::Zero, EGXTevScale::Scale_1, true, EGXTevRegister::Prev);
            mb.setTevAlphaOp(0, EGXTevOp::Add, EGXTevBias::Zero, EGXTevScale::Scale_1, true, EGXTevRegister::Prev);

            mb.setZMode(false, EGXCompareType::Always, false);
            mb.setBlendMode(EGXBlendMode::Blend, EGXBlendModeControl::SrcAlpha, EGXBlendModeControl::InverseSrcAlpha);
            mb.setUsePnMtxIdx(false);

            GXMaterial material = mb.finish(rfnt.name);
            materialHelper.emplace(material);
        }

        void destroy() {
            for (GLuint tex : gfxTextures) {
                glDeleteTextures(1, &tex);
            }
            gfxTextures.clear();
        }
    };
    inline static uint16_t glyphIndexFromChar(const RFNT& rfnt, uint16_t ch) {
        uint16_t idx = rfnt.cmap[ch];
        if (idx == 0xFFFF)
            return rfnt.defaultGlyphIndex;
        return idx;
    }
    struct TagProcessor;
    class CharWriter {
    public:
        ResFont* font = nullptr;

        glm::vec3 cursor{ 0.0f };
        glm::vec3 origin{ 0.0f };
        glm::vec3 scale{ 1.0f };

        float charSpacing = 0.0f;
        float lineHeight = 0.0f;
        int   numVertex = 0;

        Color colorT{ 1,1,1,1 };
        Color colorB{ 1,1,1,1 };
        Color color0{ 0,0,0,0 };
        Color color1{ 1,1,1,1 };

        float normalRatio = 1.0f;
        float squeezeRatio = 0.75f;

        GLuint currentTexture = 0;

        bool materialChanged = false;

        void setFont(ResFont* f,
            std::optional<float> charSpacing_ = std::nullopt,
            std::optional<float> lineHeight_ = std::nullopt,
            std::optional<float> fontWidth = std::nullopt,
            std::optional<float> fontHeight = std::nullopt)
        {
            font = f;
            const RFNT& rfnt = font->rfnt;

            scale.x = fontWidth ? (*fontWidth / rfnt.width) : 1.0f;
            scale.y = fontHeight ? (*fontHeight / rfnt.height) : 1.0f;

            if (charSpacing_) charSpacing = *charSpacing_;
            if (lineHeight_)  lineHeight = *lineHeight_;
        }

        void setColorMapping(const Color& c0, const Color& c1) {
            color0 = c0;
            color1 = c1;
            materialChanged = true;
        }

        void calcRectFromCursor(glm::vec4& dst) const {
            dst.x = std::min<float>(dst.x, cursor.x);
            dst.y = std::min<float>(dst.y, cursor.y);
            dst.z = std::max<float>(dst.z, cursor.x);
            dst.w = std::max<float>(dst.w, cursor.y);
        }

        void advanceCharacter(glm::vec4& dst, uint16_t ch, bool addSpacing) {
            if (addSpacing)
                cursor.x += charSpacing;

            uint16_t glyphIndex = glyphIndexFromChar(font->rfnt, ch);
            const GlyphInfo& gi = font->rfnt.glyphInfo[glyphIndex];

            cursor.x += gi.cwdh.advanceWidth * scale.x;
            calcRectFromCursor(dst);
            numVertex += 4;
        }

        void calcRect(glm::vec4& dst, std::u16string_view str, TagProcessor* tagProcessor = nullptr) {
            bool needsSpacing = false;
            numVertex = 0;

            dst = glm::vec4(cursor.x, cursor.y, cursor.x, cursor.y);

            if (tagProcessor)
                tagProcessor->reset(*this, &dst);

            for (size_t i = 0; i < str.size(); i++) {
                uint16_t ch = str[i];

                if (ch < 0x20) {
                    if (tagProcessor)
                        i = tagProcessor->processTag(*this, &dst, str, i) - 1;
                    continue;
                }

                advanceCharacter(dst, ch, needsSpacing);
                needsSpacing = true;
            }

            calcRectFromCursor(dst);
        }
        void drawStringGlyph(TDDraw& ddraw, const GlyphInfo& gi, float ratio)
        {
            const auto& cwdh = gi.cwdh;

            float left = cwdh.leftSideBearing * scale.x;
            float width = cwdh.width * scale.x;

            float baselineY = cursor.y;
            float y0 = baselineY - font->rfnt.ascent * scale.y;
            float y1 = y0 + font->rfnt.height * scale.y;

            float x0 = cursor.x + left;
            float x1 = x0 + width * ratio;

            float s0 = gi.s0;
            float s1 = s0 + cwdh.width * gi.s1m;

            float zOffset = cursor.x * 0.0001f;

            TDVertex v0 = { {x0,y0,cursor.z - zOffset}, {s0,gi.t0}, colorT };
            TDVertex v1 = { {x1,y0,cursor.z - zOffset}, {s1,gi.t0}, colorT };
            TDVertex v2 = { {x1,y1,cursor.z - zOffset}, {s1,gi.t1}, colorB };
            TDVertex v3 = { {x0,y1,cursor.z - zOffset}, {s0,gi.t1}, colorB };

            ddraw.vertices.push_back(v0);
            ddraw.vertices.push_back(v1);
            ddraw.vertices.push_back(v2);
            ddraw.vertices.push_back(v0);
            ddraw.vertices.push_back(v2);
            ddraw.vertices.push_back(v3);
        }
        void writeCharacter(uint16_t ch, float ratio, bool addSpacing, TDDraw& ddraw)
        {
            if (addSpacing)
                cursor.x += charSpacing;

            uint16_t glyphIndex = glyphIndexFromChar(font->rfnt, ch);
            const GlyphInfo& gi = font->rfnt.glyphInfo[glyphIndex];

            GLuint tex = font->gfxTextures[gi.textureIndex];

            bool textureChanged = (tex != currentTexture);

            if (textureChanged || materialChanged) {
                drawStringFlush(ddraw);

                if (textureChanged) {
                    glBindTexture(GL_TEXTURE_2D, tex);
                    currentTexture = tex;
                }
                materialChanged = false;
            }

            drawStringGlyph(ddraw, gi, ratio);

            float adv = gi.cwdh.advanceWidth * scale.x;

            cursor.x += adv * ratio;
        }

        void drawString(const glm::mat4& mvp,
            TDDraw& ddraw,
            std::u16string_view str,
            const std::function<float(float)>& ratioFunc,
            TagProcessor* tagProcessor = nullptr)
        {
            beginDraw(mvp);

            if (tagProcessor)
                tagProcessor->reset(*this, nullptr);

            bool needsSpacing = false;
            for (size_t i = 0; i < str.size(); i++) {
                uint16_t ch = str[i];

                if (ch < 0x20) {
                    if (tagProcessor)
                        i = tagProcessor->processTag(*this, nullptr, str, i) - 1;
                    continue;
                }

                float ratio = ratioFunc(0.0f);

                writeCharacter(ch, ratio, needsSpacing, ddraw);
                needsSpacing = true;
            }
            drawStringFlush(ddraw);
        }

    private:
        TDDraw* currentDDraw = nullptr;

        void drawStringFlush(TDDraw& ddraw) {
            if (!ddraw.hasIndicesToDraw())
                return;

            ddraw.flush();
        }

        void beginDraw(const glm::mat4& mvp) {
            font->materialHelper->bind(mvp, color0, color1);

            glActiveTexture(GL_TEXTURE0);
            currentTexture = 0;
        }
    };
    struct TagProcessor {
        virtual ~TagProcessor() = default;

        virtual void reset(CharWriter& writer, glm::vec4* rect) = 0;

        virtual size_t processTag(CharWriter& writer,
            glm::vec4* rect,
            std::u16string_view str,
            size_t i) = 0;
    };
    static lyt::RFNT parseBRFNT(bStream::CStream* stream, const std::string& name);
};