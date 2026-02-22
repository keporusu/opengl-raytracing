#include "Scene.hpp"

// シーン記述
Scene::Scene()
{
    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f, 0.0f, 1.0f),
            .radius = 1.0f,
            .material = 1});
    addPrimitive(
        Sphere{
            .center = glm::vec3(0.0f, -101.f, 1.0f),
            .radius = 100.f,
            .material = 0});
}

void Scene::addPrimitive(Sphere sphere)
{
    if (sphereCount == MAX_SPHERES)
        return;
    primitives_ubo.spheres[sphereCount] = sphere;
    primitives_ubo.sphere_count = sphereCount + 1;

    sphereCount++;
}
