#pragma once
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct UBO_Camera
{
    glm::vec3 position;
    float aspect_ratio;
    float vfov;
    int max_depth;
    float defocus_angle;
    float focus_dist;
};

class Camera
{
public:
    Camera(float orbitRadius, float aspectRatio, float vfov = 90.0, int max_depth = 10)
    {
        initialRadius = radius = orbitRadius;
        float horizontalRadius = cos(orbitPointVertical) * radius;

        initialVfov = vfov;
        initialFocusDist = 1.0f;
        initialDefocusAngle = 0.0f;

        this->cameraUBO.position = {sin(orbitPointHorizontal) * horizontalRadius, sin(orbitPointVertical) * radius, cos(orbitPointHorizontal) * horizontalRadius};
        this->cameraUBO.aspect_ratio = aspectRatio;
        this->cameraUBO.max_depth = max_depth;
        this->cameraUBO.vfov = vfov;
        this->cameraUBO.defocus_angle = initialDefocusAngle;
        this->cameraUBO.focus_dist = initialFocusDist;
    }

    // 水平移動
    void Move(glm::vec3 movement)
    {
        cameraUBO.position += movement;
        is_changed = true;
    }

    void OrbitLeft(float deltaTIme) { Orbit(deltaTIme, -1.0f, 0.0f); }
    void OrbitRight(float deltaTime) { Orbit(deltaTime, 1.0f, 0.0f); }
    void OrbitUp(float deltaTime) { Orbit(deltaTime, 0.0f, 1.0f); }
    void OrbitDown(float deltaTime) { Orbit(deltaTime, 0.0f, -1.0f); };
    void Orbit(float deltaTime, float horizontalDir, float verticalDir)
    {
        orbitPointHorizontal += deltaTime * horizontalDir;
        orbitPointVertical += deltaTime * verticalDir;
        // 90度（真上・真下）に到達する直前で止める
        float limit = glm::radians(89.0f);
        orbitPointVertical = glm::clamp(orbitPointVertical, -limit, limit);

        setPosition(orbitPointHorizontal, orbitPointVertical, radius);
        is_changed = true;
    };

    // 半径変更
    void OrbitRadius(float change)
    {
        float newRadius = radius + change;
        if (newRadius > 15.0f)
        {
            newRadius = 15.0f;
        }
        if (newRadius < 0.5f)
        {
            newRadius = 0.5f;
        }

        if (abs(radius - newRadius) > 0.02f)
        {
            is_changed = true;
            radius = newRadius;
            setPosition(orbitPointHorizontal, orbitPointVertical, radius);
        }
    }

    // 視野角変更
    void SetVfov(float vfov)
    {
        cameraUBO.vfov = vfov;
        is_changed = true;
    }

    void SetDefocusAngle(float defocus_angle)
    {
        cameraUBO.defocus_angle = defocus_angle;
        is_changed = true;
    }

    void SetFocusDist(float focus_dist)
    {
        cameraUBO.focus_dist = focus_dist;
        is_changed = true;
    }

    void SetAspectRatio(float ratio)
    {
        cameraUBO.aspect_ratio = ratio;
    }
    void ChangeFlagOff()
    {
        is_changed = false;
    }
    bool IsChanged() const
    {
        return is_changed;
    }

    void Reset()
    {
        radius = initialRadius;
        orbitPointHorizontal = initialOrbitPointHorizontal;
        orbitPointVertical = initialOrbitPointVertical;
        setPosition(orbitPointHorizontal, orbitPointVertical, radius);
        cameraUBO.vfov = initialVfov;
        cameraUBO.focus_dist = initialFocusDist;
        cameraUBO.defocus_angle = initialDefocusAngle;
        is_changed = true;
    }

    UBO_Camera *GetUBO()
    {
        return &cameraUBO;
    }

    float GetFocusDist() const { return cameraUBO.focus_dist; }
    float GetDefocusAngle() const { return cameraUBO.defocus_angle; }
    glm::vec3 GetPosition() const { return cameraUBO.position; }
    float GetVfov() const { return cameraUBO.vfov; }

private:
    UBO_Camera cameraUBO;
    bool is_changed = false;

    float initialVfov;
    float initialFocusDist;
    float initialDefocusAngle;
    float initialOrbitPointHorizontal = 0.0f;
    float initialOrbitPointVertical = 0.0f;
    float initialRadius;

    // カメラの公転移動の位置（radius）
    float orbitPointHorizontal = 0.0f;
    float orbitPointVertical = 0.0f;
    // 公転移動の半径
    float radius;

    void setPosition(float orbitPointH, float orbitPointV, float radius)
    {
        float horizontalRadius = cos(orbitPointV) * radius;
        cameraUBO.position.x = sin(orbitPointH) * horizontalRadius;
        cameraUBO.position.y = sin(orbitPointV) * radius;
        cameraUBO.position.z = cos(orbitPointH) * horizontalRadius;
    }
};