#pragma once
#include "../../third_party/imgui/imgui.h"
#include "../../third_party/imgui/backends/imgui_impl_glfw.h"
#include "../../third_party/imgui/backends/imgui_impl_opengl3.h"

class Camera;

class ImGuiController{

    public:
    ImGuiController(GLFWwindow* window);
    void Draw(Camera &camera);
    void Destroy();

};

