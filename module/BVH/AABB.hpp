
#pragma once
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct AlignedBox
{
public:
    AlignedBox() = default;

    AlignedBox(const glm::vec3 &p0, const glm::vec3 &p1)
    {
        x_min = p0.x <= p1.x ? p0.x : p1.x;
        x_max = p0.x >= p1.x ? p0.x : p1.x;
        y_min = p0.y <= p1.y ? p0.y : p1.y;
        y_max = p0.y >= p1.y ? p0.y : p1.y;
        z_min = p0.z <= p1.z ? p0.z : p1.z;
        z_max = p0.z >= p1.z ? p0.z : p1.z;
        expand();
    }

    AlignedBox(const AlignedBox &box0, const AlignedBox &box1)
    {
        x_min = box0.x_min <= box1.x_min ? box0.x_min : box1.x_min;
        x_max = box0.x_max >= box1.x_max ? box0.x_max : box1.x_max;
        y_min = box0.y_min <= box1.y_min ? box0.y_min : box1.y_min;
        y_max = box0.y_max >= box1.y_max ? box0.y_max : box1.y_max;
        z_min = box0.z_min <= box1.z_min ? box0.z_min : box1.z_min;
        z_max = box0.z_max >= box1.z_max ? box0.z_max : box1.z_max;
        expand();
    }

    float x_min, x_max;
    float y_min, y_max;
    float z_min, z_max;

private:
    void expand()
    {
        float delta = 0.0001;
        if (x_max - x_min < delta)
        {
            x_max += delta / 2.0f;
            x_min -= delta / 2.0f;
        }
        if (y_max - y_min < delta)
        {
            y_max += delta / 2.0f;
            y_min -= delta / 2.0f;
        }
        if (z_max - z_min < delta)
        {
            z_max += delta / 2.0f;
            z_min -= delta / 2.0f;
        }
    }
};