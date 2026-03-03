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

struct SubUBO_Sphere
{
    glm::vec3 center;  // offset 16の倍数
    float radius;      // offset 4の倍数
    int material;      // offset 4の倍数
    float _padding[3]; // 構造体のサイズは16の倍数に
};

struct UBO_Primitives
{
    int sphere_count;   // 4byte
    float _padding0[3]; // 12byte /合計16byte
    SubUBO_Sphere spheres[MAX_SPHERES];
};