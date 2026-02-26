#include "Scene.hpp"
#include <random>

// シーン記述
Scene::Scene()
{
    many_balls();
    createMaterialMap();
}

void Scene::addPrimitive(Sphere sphere)
{
    if (sphereCount == MAX_SPHERES)
    {
        return;
    }
    spheres.push_back(sphere);

    // 追加したプリミティブをUBOに反映
    primitives_ubo.spheres[sphereCount] = SubUBO_Sphere{
        .center = sphere.center,
        .radius = sphere.radius,
    };
    primitives_ubo.sphere_count = sphereCount + 1;

    sphereCount++;
}

void Scene::createMaterialMap()
{

    // 各プリミティブに追加されているマテリアルを見て、UBOに反映
    // 各プリミティブのUBOにマテリアル番号を追加
    int i = 0;
    for (auto sphere : spheres)
    {
        switch (sphere.material.material_type)
        {
        case MATERIAL_LAMBERTIAN:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = sphere.material.material_type,
                .albedo = sphere.material.albedo};
            break;
        }
        case MATERIAL_METAL:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = sphere.material.material_type,
                .albedo = sphere.material.albedo,
                .fuzz = sphere.material.fuzz};
            break;
        }
        case MATERIAL_DIELECTRIC:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = sphere.material.material_type,
                .refraction_index = sphere.material.refraction_index};
            break;
        }
        default:
        {
            throw std::logic_error("存在しないマテリアルタイプ: " + std::to_string(sphere.material.material_type));
            break;
        }
        }
        primitives_ubo.spheres[i].material = materialCount;
        materialCount++;
        i++;
    }
    materials_ubo.material_count = materialCount;
}

void Scene::three_balls()
{
    // 真ん中の球
    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f, 0.0f, 1.0f),
            .radius = 1.0f,
            .material = Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.1f, 0.2f, 0.5f),
            }});
    // 地面
    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f, -101.f, 1.0f),
            .radius = 100.f,
            .material = Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.8f, 0.8f, 0.0f)}});
    // ガラス
    addPrimitive(
        Sphere{
            .center = glm::vec3(-2.0f, 0.0f, 1.0f),
            .radius = 1.0f,
            .material = Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.5f}});
    // ガラス（中）
    addPrimitive(
        Sphere{
            .center = glm::vec3(-2.0f, 0.0f, 1.0f),
            .radius = 0.8f,
            .material = Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.0f / 1.5f}});
    // 金属
    addPrimitive(
        Sphere{
            .center = glm::vec3(2.0f, 0.0f, 1.0f),
            .radius = 1.0f,
            .material = Material{
                .material_type = MATERIAL_METAL,
                .albedo = glm::vec3(0.8f, 0.6f, 0.2f),
                .fuzz = 1.0f}});
}

void Scene::many_balls()
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // 地面
    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f, -1000.f, 1.0f),
            .radius = 1000.f,
            .material = Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.5f, 0.5f, 0.5f)}});

    for (int i = -3; i <= 3; i++)
    {
        for (int j = -3; j <= 3; j++)
        {
            glm::vec3 pos = {i * 2.0f + 0.9f * dist(rng), 0.2f, j * 2.0f + 0.9f * dist(rng)};
            float mat = dist(rng) * 0.5f + 0.5f;
            glm::vec3 albedo = glm::vec3(dist(rng), dist(rng), dist(rng)) * glm::vec3(0.5f) + glm::vec3(0.5f);
            if (mat < 0.75f)
            {
                addPrimitive(
                    Sphere{
                        .center = pos,
                        .radius = 0.2f,
                        .material = Material{
                            .material_type = MATERIAL_LAMBERTIAN,
                            .albedo = albedo,
                        }});
            }
            else if (mat < 0.85f)
            {
                addPrimitive(
                    Sphere{
                        .center = pos,
                        .radius = 0.2f,
                        .material = Material{
                            .material_type = MATERIAL_METAL,
                            .albedo = albedo,
                            .fuzz = (dist(rng) + 1.0f) * 0.25f,
                        }});
            }
            else
            {
                addPrimitive(
                    Sphere{
                        .center = pos,
                        .radius = 0.2f,
                        .material = Material{
                            .material_type = MATERIAL_DIELECTRIC,
                            .refraction_index = 1.5,
                        }});
            }
        }
    }

    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f,0.5f,0.0f),
            .radius = 0.5f,
            .material = Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.5,
            }});

    addPrimitive(
        Sphere{
            .center = glm::vec3(2.0f,1.0f,2.0f),
            .radius = 1.0f,
            .material = Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.5,
            }});
}
