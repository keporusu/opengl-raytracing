#pragma once
#include "../Primitives/Primitives.hpp"
#include "../BVH/BVHNode.hpp"
#include <memory>
#include <vector>

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
    void addPrimitive(Sphere sphere);
    void addPrimitive(Quad quad);
    void createMaterialMap();
    void createBVH();

    // シーン
    void threeBalls();
    void manyBalls();
    void cornellBox();
};
