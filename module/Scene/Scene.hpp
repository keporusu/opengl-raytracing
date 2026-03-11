#pragma once
#include "../Primitives/Primitives.hpp"
#include "../BVH/BVHNode.hpp"
#include <memory>
#include <vector>

// 度数法
struct Rotation
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class Scene
{

public:
    Scene();

    UBO_BVH *GetBVHUBO() { return &bvh_ubo; }
    UBO_Primitives *GetPrimitivesUBO() { return &primitives_ubo; }
    UBO_Materials *GetMateialsUBO() { return &materials_ubo; }

private:
    std::vector<std::shared_ptr<Primitive>> primitives;

    UBO_BVH bvh_ubo;
    UBO_Primitives primitives_ubo;
    UBO_Materials materials_ubo;
    BVHNode bvh;

    int sphereCount = 0;
    int quadCount = 0;
    int materialCount = 0;

    // 図形の追加
    void addPrimitive(Sphere sphere);
    void addPrimitive(Quad quad);
    void addBox(glm::vec3 p1, glm::vec3 p2, Material material, Rotation rotation);

    // マテリアルのバインド作成
    void createMaterialMap();
    // BVHの作成
    void createBVH();

    // シーン記述
    void threeBalls();
    void manyBalls();
    void cornellBox();
};
