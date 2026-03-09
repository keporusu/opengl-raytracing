#pragma once
#include "../BufferSizeSettings.hpp"
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#include "../Materials/Materials.hpp"
#include "../BVH/AABB.hpp"

struct Primitive
{
    virtual ~Primitive() = default;
    virtual AlignedBox GetAABB() const = 0;
    Material material;
    int original_index = -1; // UBO内での元のインデックス（BVHソート後も追跡可能に）
    Primitive(Material m) : material(m) {}
};

struct Sphere : Primitive
{
    glm::vec3 center;
    float radius;

    ~Sphere() override = default;
    Sphere(glm::vec3 c, float r, Material m) : Primitive(m), center(c), radius(r) {}

    // 球をすっぽり覆うようなAABBを作る
    AlignedBox GetAABB() const override
    {
        glm::vec3 rvec = glm::vec3(radius);
        return AlignedBox(center - rvec, center + rvec);
    }
};

struct Quad : Primitive
{
    glm::vec3 origin;
    glm::vec3 u, v;
    glm::vec3 normal;
    float D; // 平面のAx+By+Cz=D
    ~Quad() override = default;
    Quad(glm::vec3 o, glm::vec3 u, glm::vec3 v, Material m) : Primitive(m), origin(o), u(u), v(v)
    {
        normal = glm::normalize(glm::cross(u, v));
        D = glm::dot(origin, normal);
    }

    // Quadを覆うようなAABBを作る（厚さ0は避ける）
    AlignedBox GetAABB() const override
    {
        auto aabb1 = AlignedBox(origin, origin + u + v);
        auto aabb2 = AlignedBox(origin + u, origin + v);
        return AlignedBox(aabb1, aabb2);
    }
};

// 球のUBO
struct SubUBO_Sphere
{
    glm::vec3 center;  // 12
    float radius;      // 16
    int material;      // 20
    float _padding[3]; // 32 構造体のサイズは16の倍数に
};

// Quad用UBO
struct SubUBO_Quad
{
    glm::vec3 origin;
    float _padding0; // 16
    glm::vec3 u;
    float _padding1; // 32
    glm::vec3 v;
    float _padding2;    // 48
    glm::vec3 normal;   // 60
    float D;            // 64
    int material;       // 68
    float _padding3[3]; // 80 構造体のサイズは16の倍数に
};

struct UBO_Primitives
{
    int sphere_count;   // 4
    float _padding0[3]; // 16
    SubUBO_Sphere spheres[MAX_SPHERES];
    int quad_count;
    float _padding1[3];
    SubUBO_Quad quads[MAX_QUADS];
};