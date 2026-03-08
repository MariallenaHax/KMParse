#pragma once
#include "glm/glm.hpp"
#include "glad/glad.h"
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 color;
};
class GfxModel {
public:
    GLuint vao = 0;
    GLuint vbo = 0;
    GLsizei vertexCount = 0;
    GfxModel(const std::vector<Vertex>& verts);
    ~GfxModel();
    void draw();
};

class ModelBuilder {
public:
    std::vector<Vertex> vertices;
    glm::quat rotationFromTo(glm::vec3 from, glm::vec3 to);
    ModelBuilder& addTri(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec4& c1, const glm::vec4& c2, const glm::vec4& c3);
    ModelBuilder& addQuad(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec4& c1, const glm::vec4& c2, const glm::vec4& c3, const glm::vec4& c4);
    ModelBuilder& addQuadSubdiv(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, int subdivs);
    ModelBuilder& addCube(float x1, float y1, float z1, float x2, float y2, float z2, int subdivs);
    ModelBuilder& addSphere(float x1, float y1, float z1, float x2, float y2, float z2, int subdivs);
    ModelBuilder& addCone(float x1, float y1, float z1, float x2, float y2, float z2, int subdivs, glm::vec3 upVec);
    ModelBuilder& addCylinder(float x1, float y1, float z1, float x2, float y2, float z2, int subdivs, glm::vec3 upVec);
    ModelBuilder& calculateNormals();
    GfxModel* makeModel();
};