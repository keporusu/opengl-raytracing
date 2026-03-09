#include "../BufferSizeSettings.hpp"
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#include <iostream>
#include <string>

#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2
#define MATERIAL_DIFFUSE_LIGHT 10

struct Material
{
    int material_type;
    glm::vec3 albedo;
    float fuzz;                         // ぼやかし
    float refraction_index;             // 相対屈折率
    std::string texture_label;          // テクスチャのラベル
    glm::vec3 emitted = glm::vec3(0.0); // 発光
};

struct SubUBO_Material
{
    int material_type;      // 4
    float _padding0[3];     // 16
    glm::vec3 albedo;       // 28
    float fuzz;             // 32
    float refraction_index; // 36
    int texture = -1;       // 40
    float _padding1[2];     // 48
    glm::vec3 emitted;      // 60
    float _padding2;        // 64
};

struct UBO_Materials
{
    int material_count; // offset 4の倍数
    float _padding2[3]; // パディング
    SubUBO_Material materials[MAX_MATERIALS];
};