#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <string>
#define MAX_MATERIALS 60

#define MATERIAL_LAMBERTIAN 1
#define MATERIAL_METAL 2
#define MATERIAL_DIELECTRIC 3

struct Material
{
    int material_type;
    glm::vec3 albedo;
    float fuzz; //ぼやかし
    float refraction_index;//絶対屈折率
};

struct SubUBO_Material
{
    int material_type;  // offset 4の倍数
    float _padding0[3]; // パディング
    glm::vec3 albedo;   // offset 16の倍数
    float fuzz;         // offset 4の倍数
    float refraction_index; //offset 4の倍数
    float _padding1[3]; //構造体のサイズを16の倍数に(48)
};

struct UBO_Materials
{
    int material_count; // offset 4の倍数
    float _padding2[3]; // パディング
    SubUBO_Material materials[MAX_MATERIALS];
};