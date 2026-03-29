#include "Scene.hpp"
#include "../Texture/Texture.hpp"
#include <random>

// シーン記述
Scene::Scene()
{
    //threeBalls();
    //manyBalls();
    cornellBox();
    //showcase();
    //mirrorCorridor();
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

void Scene::addPrimitive(Quad quad, Rotation rotation)
{
    if (quadCount == MAX_QUADS)
    {
        return;
    }

    // Quadの中心を軸に回転
    glm::vec3 center = quad.origin + (quad.u + quad.v) * 0.5f;

    glm::mat4 rotMat = glm::mat4(1.0f);
    rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0, 0, 1));

    quad.origin = glm::vec3(rotMat * glm::vec4(quad.origin - center, 1.0f)) + center;
    quad.u = glm::vec3(rotMat * glm::vec4(quad.u, 0.0f));
    quad.v = glm::vec3(rotMat * glm::vec4(quad.v, 0.0f));
    quad.normal = glm::normalize(glm::cross(quad.u, quad.v));
    quad.D = glm::dot(quad.origin, quad.normal);

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

void Scene::addBox(glm::vec3 p1, glm::vec3 p2, Material material, Rotation rotation = Rotation())
{
    float min_x = std::min(p1.x, p2.x);
    float min_y = std::min(p1.y, p2.y);
    float min_z = std::min(p1.z, p2.z);
    float max_x = std::max(p1.x, p2.x);
    float max_y = std::max(p1.y, p2.y);
    float max_z = std::max(p1.z, p2.z);
    auto dx = glm::vec3(max_x - min_x, 0.0f, 0.0f);
    auto dy = glm::vec3(0.0f, max_y - min_y, 0.0f);
    auto dz = glm::vec3(0.0f, 0.0f, max_z - min_z);

    std::vector<Quad> quads;
    quads.push_back(Quad{
        glm::vec3(min_x, min_y, max_z), dx, dy, material}); // 前
    quads.push_back(Quad{
        glm::vec3(max_x, min_y, max_z), -dz, dy, material}); // 右
    quads.push_back(Quad{
        glm::vec3(max_x, min_y, min_z), -dx, dy, material}); // 後
    quads.push_back(Quad{
        glm::vec3(min_x, min_y, min_z), dz, dy, material}); // 左
    quads.push_back(Quad{
        glm::vec3(min_x, max_y, max_z), dx, -dz, material}); // 上
    quads.push_back(Quad{
        glm::vec3(min_x, min_y, min_z), dx, dz, material}); // 下

    glm::vec3 center = (p1 + p2) * 0.5f;

    glm::mat4 rotMat = glm::mat4(1.0f);
    rotMat = glm::rotate(rotMat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rotMat = glm::rotate(rotMat, glm::radians(rotation.z), glm::vec3(0, 0, 1));

    for (auto quad : quads)
    {
        // ボックス中心を原点とした座標系でquadを回転させる
        quad.origin = glm::vec3(rotMat * glm::vec4(quad.origin - center, 1.0f)) + center;
        quad.u = glm::vec3(rotMat * glm::vec4(quad.u, 0.0f));
        quad.v = glm::vec3(rotMat * glm::vec4(quad.v, 0.0f));
        quad.normal = glm::normalize(glm::cross(quad.u, quad.v));
        quad.D = glm::dot(quad.origin, quad.normal);
        addPrimitive(quad);
    }
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
            materials_ubo.materials[materialCount] = SubUBO_Material{
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
    auto red = glm::vec3(0.9f, 0.05f, 0.05f);
    auto green = glm::vec3(0.12f, 0.9f, 0.15f);

    // 後ろ
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});

    // 左
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = green,
        }});

    // 右
    addPrimitive(Quad{
        glm::vec3(0.5f, -0.5f, 0.5f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = red,
        }});

    // 下
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 0.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});

    // 上
    addPrimitive(Quad{
        glm::vec3(-0.5f, 0.5f, 0.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
        }});

    // 上ライト
    addPrimitive(Quad{
        glm::vec3(-0.15f, 0.49f, 0.15f),
        glm::vec3(0.3f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -0.3f),
        Material{
            .material_type = MATERIAL_DIFFUSE_LIGHT,
            .emitted = glm::vec3(15.f),
        }},Rotation{.x=180.f});

    addBox(glm::vec3(-0.35f, -0.5f, -0.3f), glm::vec3(-0.1f, 0.0f, -0.1f),
           Material{
               .material_type = MATERIAL_LAMBERTIAN,
               .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
           },
           Rotation{.y = 30.0f});

    addBox(glm::vec3(0.3f, -0.5f, -0.1f), glm::vec3(0.05f, -0.25f, 0.15f),
           Material{
               .material_type = MATERIAL_LAMBERTIAN,
               .albedo = glm::vec3(0.73f, 0.73f, 0.73f),
           },
           Rotation{.y = -30.0f});
}

void Scene::showcase()
{
    // 金属床（鏡面反射）
    addPrimitive(Quad{
        glm::vec3(-1.5f, -0.3f, 1.5f),
        glm::vec3(3.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -3.0f),
        Material{
            .material_type = MATERIAL_METAL,
            .albedo = glm::vec3(0.8f, 0.8f, 0.8f),
            .fuzz = 0.0f,
        }});

    // 後ろの壁
    addPrimitive(Quad{
        glm::vec3(-1.5f, -0.3f, -1.5f),
        glm::vec3(3.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 2.0f, 0.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.9f, 0.9f, 0.9f),
        }});

    // ガラス球（左）
    addPrimitive(Sphere(
        glm::vec3(-0.6f, 0.0f, 0.0f),
        0.3f,
        Material{
            .material_type = MATERIAL_DIELECTRIC,
            .refraction_index = 1.5f,
        }));

    // マット球（中央）
    addPrimitive(Sphere(
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.3f,
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.8f, 0.3f, 0.1f),
        }));

    // 金属球（右）
    addPrimitive(Sphere(
        glm::vec3(0.6f, 0.0f, 0.0f),
        0.3f,
        Material{
            .material_type = MATERIAL_METAL,
            .albedo = glm::vec3(0.9f, 0.7f, 0.2f),
            .fuzz = 0.0f,
        }));

    // 上ライト
    addPrimitive(Quad{
        glm::vec3(-0.4f, 1.5f, 0.4f),
        glm::vec3(0.8f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -0.8f),
        Material{
            .material_type = MATERIAL_DIFFUSE_LIGHT,
            .emitted = glm::vec3(10.0f, 10.0f, 10.0f),
        }});
}

void Scene::mirrorCorridor()
{
    auto mirror = Material{
        .material_type = MATERIAL_METAL,
        .albedo = glm::vec3(0.95f, 0.95f, 0.95f),
        .fuzz = 0.0f,
    };

    // 床
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 1.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -3.0f),
        mirror});

    // 天井
    addPrimitive(Quad{
        glm::vec3(-0.5f, 0.5f, 1.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -3.0f),
        mirror});

    // 左壁
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, 1.5f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -3.0f),
        mirror});

    // 右壁
    addPrimitive(Quad{
        glm::vec3(0.5f, -0.5f, 1.5f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -3.0f),
        mirror});

    // 奥の壁
    addPrimitive(Quad{
        glm::vec3(-0.5f, -0.5f, -1.5f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        Material{
            .material_type = MATERIAL_LAMBERTIAN,
            .albedo = glm::vec3(0.7f, 0.7f, 0.9f),
        }});

    // 中央のガラス球
    addPrimitive(Sphere(
        glm::vec3(0.0f, 0.0f, 0.0f),
        0.25f,
        Material{
            .material_type = MATERIAL_DIELECTRIC,
            .refraction_index = 1.5f,
        }));

    // 天井ライト
    addPrimitive(Quad{
        glm::vec3(-0.15f, 0.49f, 0.15f),
        glm::vec3(0.3f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -0.3f),
        Material{
            .material_type = MATERIAL_DIFFUSE_LIGHT,
            .emitted = glm::vec3(15.0f, 15.0f, 15.0f),
        }});
}