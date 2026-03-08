#include "cache.h"
GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
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

GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc) {
    std::string key = std::string(vertexSrc) + "\n---\n" + std::string(fragmentSrc);
    size_t hash = std::hash<std::string>{}(key);

    auto it = shaderCache.find(hash);
    if (it != shaderCache.end())
        return it->second;

    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSrc);
    if (!vertexShader) return 0;

    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSrc);
    if (!fragmentShader) {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[2048];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);

        std::cerr << "[Program Link Error]\n";
        std::cerr << infoLog << "\n";

        glDeleteProgram(program);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    shaderCache[hash] = program;

    return program;
}