#pragma once
#include "Camera.hpp"
#include "../InputSystem/InputSystem.hpp"

class CameraController
{

public:
    CameraController()
    {
    }

    void ApplyInput(Camera &camera, InputSystem &inputSystem)
    {
        float deltaTime = inputSystem.GetDeltaTime();

        // 公転移動
        if (inputSystem.IsKeyDown(GLFW_KEY_D))
        {
            camera.OrbitRight(deltaTime);
        }
        if (inputSystem.IsKeyDown(GLFW_KEY_A))
        {
            camera.OrbitLeft(deltaTime);
        }
        if (inputSystem.IsKeyDown(GLFW_KEY_S))
        {
            camera.OrbitDown(deltaTime);
        }
        if (inputSystem.IsKeyDown(GLFW_KEY_W))
        {
            camera.OrbitUp(deltaTime);
        }

        // 近づく
        // camera.Zoom(inputSystem.GetScrollDelta() * deltaTime * 8.0f);
        camera.OrbitRadius(inputSystem.GetScrollDelta() * deltaTime);

        // リセット
        if (inputSystem.IsKeyDown(GLFW_KEY_R))
        {
            camera.Reset();
        }
    }
};