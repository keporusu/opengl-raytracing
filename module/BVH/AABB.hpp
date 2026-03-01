
#pragma once
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct AlignedBox
{
public:
    AlignedBox(const glm::vec3 p0, const glm::vec3 p1)
    {
        x_min = p0.x <= p1.x ? p0.x : p1.x;
        x_max = p0.x >= p1.x ? p0.x : p1.x;
        y_min = p0.y <= p1.y ? p0.y : p1.y;
        x_max = p0.y >= p1.y ? p0.y : p1.y;
        x_min = p0.z <= p1.z ? p0.z : p1.z;
        x_max = p0.z >= p1.z ? p0.z : p1.z;
    }

    AlignedBox(const AlignedBox &box0, const AlignedBox &box1)
    {
        x_min = box0.x_min <= box1.x_min ? box0.x_min : box1.x_min;
        x_max = box0.x_max >= box1.x_max ? box0.x_max : box1.x_max;
        y_min = box0.y_min <= box1.y_min ? box0.y_min : box1.y_min;
        y_max = box0.y_max >= box1.y_max ? box0.y_max : box1.y_max;
        z_min = box0.z_min <= box1.z_min ? box0.z_min : box1.z_min;
        z_max = box0.z_max >= box1.z_max ? box0.z_max : box1.z_max;
    }

    float x_min, x_max;
    float y_min, y_max;
    float z_min, z_max;
};