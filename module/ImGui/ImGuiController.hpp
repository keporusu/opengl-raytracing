#pragma once
#include <iostream>
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_glfw.h"
#include "../../third_party/imgui/backends/imgui_impl_opengl3.h"

class Camera;

class ImGuiController{

    public:

    ImGuiController(GLFWwindow *window,float uiVFov,float uiFocusDist,float uiDefocusAngle);

    void SetCameraPosition(float x,float y,float z);
    void SetSampleCount(int n);
    void Draw();
    void Destroy();

    //コールバック登録
    std::function<void(float)> OnUpdateVfov;
    std::function<void(float)> OnUpdateFocusDist;
    std::function<void(float)> OnUpdateDefocusAngle;

    private:
    float uiCameraX,uiCameraY,uiCameraZ;
    int uiSampleCount;
    float uiVFov;
    float uiFocusDist;
    float uiDefocusAngle;
};

