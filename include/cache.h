#pragma once
#include "glad/glad.h"
#include "string"
#include <iostream>
#include <unordered_map>
static std::unordered_map<size_t, GLuint> shaderCache;
GLuint CompileShader(GLenum type, const char* source);
GLuint CreateShaderProgram(const char* vertexSrc, const char* fragmentSrc);