#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#include "../Materials/Materials.hpp"
#define MAX_SPHERES 60
#define MAX_PLANES 60

struct Sphere
{
    glm::vec3 center;
    float radius;
    Material material;
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