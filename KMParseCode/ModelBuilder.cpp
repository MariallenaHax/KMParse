#include "KMParseCode//ModelBuilder.h"
#include "glm/gtx/quaternion.hpp"
#include "glm/ext/scalar_constants.hpp"
GfxModel::GfxModel(const std::vector<Vertex>& verts)
{
    vertexCount = verts.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glBufferData(GL_ARRAY_BUFFER,
        verts.size() * sizeof(Vertex),
        verts.data(),
        GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void GfxModel::draw()
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

GfxModel::~GfxModel()
{
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}
glm::quat ModelBuilder::rotationFromTo(glm::vec3 from, glm::vec3 to)
{
    from = glm::normalize(from);
    to = glm::normalize(to);

    float cosTheta = glm::dot(from, to);
    glm::vec3 axis;

    if (cosTheta < -1 + 0.001f)
    {
        axis = glm::cross(glm::vec3(0, 0, 1), from);
        if (glm::length(axis) < 0.01f)
            axis = glm::cross(glm::vec3(1, 0, 0), from);

        axis = glm::normalize(axis);
        return glm::angleAxis(glm::pi<float>(), axis);
    }

    axis = glm::cross(from, to);

    float s = sqrt((1 + cosTheta) * 2);
    float invs = 1 / s;

    return glm::quat(
        s * 0.5f,
        axis.x * invs,
        axis.y * invs,
        axis.z * invs
    );
}
ModelBuilder& ModelBuilder::addTri(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
    const glm::vec4& c1 = glm::vec4(1, 1, 1, 1),
    const glm::vec4& c2 = glm::vec4(1, 1, 1, 1),
    const glm::vec4& c3 = glm::vec4(1, 1, 1, 1))
{
    vertices.push_back({ v1, glm::vec3(0), c1 });
    vertices.push_back({ v2, glm::vec3(0), c2 });
    vertices.push_back({ v3, glm::vec3(0), c3 });
    return *this;
}
ModelBuilder& ModelBuilder::addQuad(const glm::vec3& v1, const glm::vec3& v2,
    const glm::vec3& v3, const glm::vec3& v4,
    const glm::vec4& c1 = glm::vec4(1, 1, 1, 1),
    const glm::vec4& c2 = glm::vec4(1, 1, 1, 1),
    const glm::vec4& c3 = glm::vec4(1, 1, 1, 1),
    const glm::vec4& c4 = glm::vec4(1, 1, 1, 1))
{
    addTri(v1, v2, v3, c1, c2, c3);
    addTri(v1, v3, v4, c1, c3, c4);
    return *this;
}
ModelBuilder& ModelBuilder::addQuadSubdiv(
    const glm::vec3& v1,
    const glm::vec3& v2,
    const glm::vec3& v3,
    const glm::vec3& v4,
    int subdivs)
{
    for (int j = 0; j < subdivs; j++)
    {
        for (int i = 0; i < subdivs; i++)
        {
            glm::vec3 p1 = glm::mix(v1, v2, (float)i / subdivs);
            glm::vec3 p2 = glm::mix(v1, v2, (float)(i + 1) / subdivs);
            glm::vec3 p3 = glm::mix(v4, v3, (float)(i + 1) / subdivs);
            glm::vec3 p4 = glm::mix(v4, v3, (float)i / subdivs);

            glm::vec3 f1 = glm::mix(p1, p4, (float)j / subdivs);
            glm::vec3 f2 = glm::mix(p2, p3, (float)j / subdivs);
            glm::vec3 f3 = glm::mix(p2, p3, (float)(j + 1) / subdivs);
            glm::vec3 f4 = glm::mix(p1, p4, (float)(j + 1) / subdivs);

            addQuad(f1, f2, f3, f4);
        }
    }
    return *this;
}
ModelBuilder& ModelBuilder::addCube(float x1, float y1, float z1,
    float x2, float y2, float z2,
    int subdivs = 1)
{
    glm::vec3 v1Top(x1, y1, z1);
    glm::vec3 v2Top(x2, y1, z1);
    glm::vec3 v3Top(x2, y2, z1);
    glm::vec3 v4Top(x1, y2, z1);

    glm::vec3 v1Bot(x1, y1, z2);
    glm::vec3 v2Bot(x2, y1, z2);
    glm::vec3 v3Bot(x2, y2, z2);
    glm::vec3 v4Bot(x1, y2, z2);

    addQuadSubdiv(v1Top, v2Top, v3Top, v4Top, subdivs);
    addQuadSubdiv(v1Bot, v4Bot, v3Bot, v2Bot, subdivs);
    addQuadSubdiv(v2Top, v1Top, v1Bot, v2Bot, subdivs);
    addQuadSubdiv(v3Top, v2Top, v2Bot, v3Bot, subdivs);
    addQuadSubdiv(v4Top, v3Top, v3Bot, v4Bot, subdivs);
    addQuadSubdiv(v1Top, v4Top, v4Bot, v1Bot, subdivs);

    return *this;
}
ModelBuilder& ModelBuilder::addSphere(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    int subdivs = 8)
{
    size_t index = vertices.size();

    addCube(x1, y1, z1, x2, y2, z2, subdivs);

    glm::vec3 c(
        (x1 + x2) * 0.5f,
        (y1 + y2) * 0.5f,
        (z1 + z2) * 0.5f
    );

    glm::vec3 size(
        std::abs(x2 - x1) * 0.5f,
        std::abs(y2 - y1) * 0.5f,
        std::abs(z2 - z1) * 0.5f
    );

    for (size_t i = index; i < vertices.size(); i++)
    {
        glm::vec3 p = vertices[i].pos;
        glm::vec3 dir = glm::normalize(p - c);
        vertices[i].pos = c + dir * size;
    }

    return *this;
}
ModelBuilder& ModelBuilder::addCone(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    int subdivs = 8,
    glm::vec3 upVec = glm::vec3(0))
{
    size_t index = vertices.size();

    float cx = (x1 + x2) * 0.5f;
    float cy = (y1 + y2) * 0.5f;

    float sx = (x2 - x1) * 0.5f;
    float sy = (y2 - y1) * 0.5f;

    for (int i = 0; i < subdivs; i++)
    {
        float angle0 = (float)i / subdivs * (glm::pi<float>() * 2.0f);
        float angle1 = (float)(i + 1) / subdivs * (glm::pi<float>() * 2.0f);

        float cos0 = cos(angle0);
        float cos1 = cos(angle1);
        float sin0 = sin(angle0);
        float sin1 = sin(angle1);

        addTri(
            { cx + cos0 * sx, cy + sin0 * sy, z1 },
            { cx + cos1 * sx, cy + sin1 * sy, z1 },
            { cx, cy, z1 }
        );

        addTri(
            { cx + cos1 * sx, cy + sin1 * sy, z1 },
            { cx + cos0 * sx, cy + sin0 * sy, z1 },
            { cx, cy, z2 }
        );
    }

    if (upVec != glm::vec3(0))
    {
        glm::vec3 from = glm::vec3(0, 0, -1);
        glm::vec3 to = glm::normalize(upVec);

        glm::quat q = rotationFromTo(from, to);

        glm::mat4 rot = glm::mat4_cast(q);

        for (size_t i = index; i < vertices.size(); i++)
        {
            glm::vec4 p = rot * glm::vec4(vertices[i].pos, 1.0f);
            vertices[i].pos = glm::vec3(p);
        }
    }

    return *this;
}
ModelBuilder& ModelBuilder::addCylinder(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    int subdivs = 8,
    glm::vec3 upVec = glm::vec3(0))
{
    size_t index = vertices.size();

    float cx = (x1 + x2) * 0.5f;
    float cy = (y1 + y2) * 0.5f;

    float sx = (x2 - x1) * 0.5f;
    float sy = (y2 - y1) * 0.5f;

    for (int i = 0; i < subdivs; i++)
    {
        float angle0 = (float)i / subdivs * (glm::pi<float>() * 2.0f);
        float angle1 = (float)(i + 1) / subdivs * (glm::pi<float>() * 2.0f);


        float cos0 = cos(angle0);
        float cos1 = cos(angle1);
        float sin0 = sin(angle0);
        float sin1 = sin(angle1);

        addTri(
            { cx + cos0 * sx, cy + sin0 * sy, z1 },
            { cx + cos1 * sx, cy + sin1 * sy, z1 },
            { cx, cy, z1 }
        );

        addTri(
            { cx + cos1 * sx, cy + sin1 * sy, z2 },
            { cx + cos0 * sx, cy + sin0 * sy, z2 },
            { cx, cy, z2 }
        );

        addQuad(
            { cx + cos1 * sx, cy + sin1 * sy, z1 },
            { cx + cos0 * sx, cy + sin0 * sy, z1 },
            { cx + cos0 * sx, cy + sin0 * sy, z2 },
            { cx + cos1 * sx, cy + sin1 * sy, z2 }
        );
    }
    if (upVec != glm::vec3(0))
    {
        glm::vec3 from = glm::vec3(0, 0, -1);
        glm::vec3 to = glm::normalize(upVec);

        glm::quat q = rotationFromTo(from, to);

        glm::mat4 rot = glm::mat4_cast(q);

        for (size_t i = index; i < vertices.size(); i++)
        {
            glm::vec4 p = rot * glm::vec4(vertices[i].pos, 1.0f);
            vertices[i].pos = glm::vec3(p);
        }
    }

    return *this;
}
ModelBuilder& ModelBuilder::calculateNormals()
{
    for (size_t i = 0; i < vertices.size(); i += 3) {
        auto& a = vertices[i + 0];
        auto& b = vertices[i + 1];
        auto& c = vertices[i + 2];

        glm::vec3 n = glm::normalize(glm::cross(b.pos - a.pos, c.pos - a.pos));
        a.normal = b.normal = c.normal = n;
    }
    return *this;
}
GfxModel* ModelBuilder::makeModel()
{
    return new GfxModel(vertices);
}