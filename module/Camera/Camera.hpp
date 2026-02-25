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
        radius = orbitRadius;
        float horizontalRadius = cos(orbitPointVertical) * radius;

        initialVfov = vfov;
        initialFocusDist = 1.0f;
        initialDefocusAngle = 1.0f;

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

    void OrbitLeft(float deltaTIme) { Orbit(deltaTIme, 1.0f, 0.0f); }
    void OrbitRight(float deltaTime) { Orbit(deltaTime, -1.0f, 0.0f); }
    void OrbitUp(float deltaTime) { Orbit(deltaTime, 0.0f, -1.0f); }
    void OrbitDown(float deltaTime) { Orbit(deltaTime, 0.0f, 1.0f); };
    void Orbit(float deltaTime, float horizontalDir, float verticalDir)
    {
        orbitPointHorizontal += deltaTime * horizontalDir;
        orbitPointVertical += deltaTime * verticalDir;
        // 90度（真上・真下）に到達する直前で止める
        float limit = glm::radians(89.0f);
        orbitPointVertical = glm::clamp(orbitPointVertical, -limit, limit);

        // 水平方向の半径を計算（縦回転の影響で、中心軸に寄る分を考慮）
        float horizontalRadius = cos(orbitPointVertical) * radius;

        // 座標に代入
        cameraUBO.position.x = sin(orbitPointHorizontal) * horizontalRadius;
        cameraUBO.position.y = sin(orbitPointVertical) * radius;
        cameraUBO.position.z = cos(orbitPointHorizontal) * horizontalRadius;

        is_changed = true;
    };

    // 視野角変更
    void Zoom(float movement)
    {

        float newFOV = cameraUBO.vfov + movement;
        if (newFOV > 120.0f)
        {
            newFOV = 120.0f;
        }
        if (newFOV < 30.0f)
        {
            newFOV = 30.0f;
        }

        if (abs(cameraUBO.vfov - newFOV) > 0.1f)
        {
            is_changed = true;
            cameraUBO.vfov = newFOV;
        }
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
        cameraUBO.position = {sin(initialOrbitPointHorizontal) * radius, sin(initialOrbitPointVertical) * radius, cos(initialOrbitPointHorizontal) * radius};
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

    // カメラの公転移動の位置（radius）
    float orbitPointHorizontal = 0.0f;
    float orbitPointVertical = 0.0f;
    // 公転移動の半径
    float radius;
};