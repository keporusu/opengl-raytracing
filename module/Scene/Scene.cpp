#include "Scene.hpp"
#include "../Texture/Texture.hpp"
#include <random>

// シーン記述
Scene::Scene()
{
    //manyBalls();
    cornellBox();
    createMaterialMap();
    createBVH();
}

void Scene::addPrimitive(Sphere sphere)
{
    if (sphereCount == MAX_SPHERES)
    {
        return;
    }
    sphere.original_index = sphereCount;
    primitives.push_back(std::make_shared<Sphere>(sphere));

    // 追加したプリミティブをUBOに反映
    primitives_ubo.spheres[sphereCount] = SubUBO_Sphere{
        .center = sphere.center,
        .radius = sphere.radius,
        .material = 0 // material index will be set in createMaterialMap
    };
    primitives_ubo.sphere_count = sphereCount + 1;

    sphereCount++;
}

void Scene::addPrimitive(Quad quad)
{
    if (quadCount == MAX_QUADS)
    {
        return;
    }
    quad.original_index = quadCount;
    primitives.push_back(std::make_shared<Quad>(quad));

    // 追加したプリミティブをUBOに反映
    primitives_ubo.quads[quadCount] = SubUBO_Quad{
        .origin = quad.origin,
        .u = quad.u,
        .v = quad.v,
        .normal = quad.normal,
        .D = quad.D,
        .material = 0 // material index will be set in createMaterialMap
    };
    primitives_ubo.quad_count = quadCount + 1;

    quadCount++;
}

void Scene::createBVH()
{
    bvh = BVHNode(primitives, 0, primitives.size());
    auto nodes = bvh.FlattenAndGetUBO();
    std::copy(nodes.begin(), nodes.end(), bvh_ubo.nodes);
    bvh_ubo.node_count = nodes.size();
}

void Scene::createMaterialMap()
{
    // 各プリミティブに追加されているマテリアルを見て、UBOに反映
    // 各プリミティブのUBOにマテリアル番号を追加
    for (auto primitive : primitives)
    {
        // マテリアルごとにUBOを設定
        switch (primitive->material.material_type)
        {
        case MATERIAL_LAMBERTIAN:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = primitive->material.material_type,
                .albedo = primitive->material.albedo};
            break;
        }
        case MATERIAL_METAL:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = primitive->material.material_type,
                .albedo = primitive->material.albedo,
                .fuzz = primitive->material.fuzz};
            break;
        }
        case MATERIAL_DIELECTRIC:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material{
                .material_type = primitive->material.material_type,
                .fuzz = 0.0f, // fuzz
                .refraction_index = primitive->material.refraction_index};
            break;
        }
        case MATERIAL_DIFFUSE_LIGHT:
        {
            materials_ubo.materials[materialCount] = SubUBO_Material
            {
                .material_type = primitive->material.material_type,
                .emitted = primitive->material.emitted,
            };
            break;
        }
        default:
        {
            throw std::logic_error("存在しないマテリアルタイプ: " + std::to_string(primitive->material.material_type));
            break;
        }
        }

        // テクスチャのバインド
        std::string texLabel = primitive->material.texture_label;
        if (!texLabel.empty())
        {
            materials_ubo.materials[materialCount].texture = Texture::GetTextureIndex(texLabel);
        }
        else
        {
            materials_ubo.materials[materialCount].texture = -1;
        }

        // プリミティブのタイプに応じて正しいUBO配列にマテリアルインデックスを設定
        if (primitive->GetPrimitiveType() == PRIM_TYPE_SPHERE)
        {
            primitives_ubo.spheres[primitive->original_index].material = materialCount;
        }
        else if (primitive->GetPrimitiveType() == PRIM_TYPE_QUAD)
        {
            primitives_ubo.quads[primitive->original_index].material = materialCount;
        }
        materialCount++;
    }
    materials_ubo.material_count = materialCount;
}

void Scene::threeBalls()
{
    // 真ん中の球
    addPrimitive(
        Sphere(
            glm::vec3(0.0f, 0.0f, 1.0f),
            1.0f,
            Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.1f, 0.2f, 0.5f),
            }));
    // 地面
    addPrimitive(
        Sphere(
            glm::vec3(0.0f, -101.f, 1.0f),
            100.f,
            Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.8f, 0.8f, 0.0f)}));
    // ガラス
    addPrimitive(
        Sphere(
            glm::vec3(-2.0f, 0.0f, 1.0f),
            1.0f,
            Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.5f // refraction_index
            }));
    // ガラス（中）
    addPrimitive(
        Sphere(
            glm::vec3(-2.0f, 0.0f, 1.0f),
            0.8f,
            Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.0f / 1.5f // refraction_index
            }));
    // 金属
    addPrimitive(
        Sphere(
            glm::vec3(2.0f, 0.0f, 1.0f),
            1.0f,
            Material{
                .material_type = MATERIAL_METAL,
                .albedo = glm::vec3(0.8f, 0.6f, 0.2f),
                .fuzz = 1.0f // fuzz
            }));
}

void Scene::manyBalls()
{
    addPrimitive(Quad{
        glm::vec3(0.0f, 0.0f, 1.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Material{
            .material_type = MATERIAL_DIFFUSE_LIGHT,
            .emitted = glm::vec3(0.0f, 7.0f, 0.0f),
        }});

    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

    // 地面
    addPrimitive(
        Sphere(
            glm::vec3(0.0f, -1000.f, 1.0f),
            1000.f,
            Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .albedo = glm::vec3(0.5f, 0.5f, 0.5f)}));

    glm::vec3 glass_pos1 = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 earth_pos = glm::vec3(2.0f, 0.7f, 2.0f);

    addPrimitive(
        Sphere(
            glass_pos1,
            1.0f,
            Material{
                .material_type = MATERIAL_DIELECTRIC,
                .refraction_index = 1.5f // refraction_index
            }));

    addPrimitive(
        Sphere(
            earth_pos,
            0.7f,
            Material{
                .material_type = MATERIAL_LAMBERTIAN,
                .texture_label = "earth",
                //.refraction_index = 1.5f // refraction_index
            }));

    for (int i = -4; i <= 4; i++)
    {
        for (int j = -4; j <= 4; j++)
        {
            glm::vec3 pos = {i * 1.7f + 0.9f * dist(rng), 0.2f, j * 1.7f + 0.9f * dist(rng)};
            float mat = dist(rng) * 0.5f + 0.5f;
            glm::vec3 albedo = glm::vec3(dist(rng), dist(rng), dist(rng)) * glm::vec3(0.5f) + glm::vec3(0.5f);

            if (glm::length(glm::vec2(glass_pos1.x - pos.x, glass_pos1.y - pos.y)) < 1.5f)
                continue;
            if (glm::length(glm::vec2(earth_pos.x - pos.x, earth_pos.y - pos.y)) < 1.0f)
                continue;

            if (mat < 0.7f)
            {
                addPrimitive(
                    Sphere(
                        pos,
                        0.2f,
                        Material{
                            .material_type = MATERIAL_LAMBERTIAN,
                            .albedo = albedo,
                        }));
            }
            else if (mat < 0.85f)
            {
                addPrimitive(
                    Sphere(
                        pos,
                        0.2f,
                        Material{
                            .material_type = MATERIAL_METAL,
                            .albedo = albedo,
                            .fuzz = (dist(rng) + 1.0f) * 0.25f,
                        }));
            }
            else
            {
                addPrimitive(
                    Sphere(
                        pos,
                        0.2f,
                        Material{
                            .material_type = MATERIAL_DIELECTRIC,
                            .refraction_index = 1.5f // refraction_index
                        }));
            }
        }
    }
}

void Scene::cornellBox()
{
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 3.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});

    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 4.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.12f, 0.45f, 0.15f),
        }});

    addPrimitive(Quad{
        glm::vec3(0.5f, -0.5f, 4.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.65f, 0.05f, 0.05f),
        }});

    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 4.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});

    addPrimitive(Quad{
        glm::vec3(-0.5f, 0.5f, 4.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});
    
    addPrimitive(Quad{
        glm::vec3(-0.15f, 0.49f, 3.65f),
        glm::vec3(0.3f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -0.3f),
        Material{
            .material_type = MATERIAL_DIFFUSE_LIGHT,
            .emitted = glm::vec3(15.f, 15.f, 15.f),
        }});
    
}