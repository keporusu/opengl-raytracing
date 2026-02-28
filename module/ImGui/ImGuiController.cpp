#include "ImGuiController.hpp"
#include "../Camera/Camera.hpp"
#include <GLFW/glfw3.h>

ImGuiController::ImGuiController(GLFWwindow *window)
{

    // GLFW・OpenGLの初期化後に実行
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // スタイル設定（任意）
    ImGui::StyleColorsDark();

    // バックエンドの初期化
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void ImGuiController::Draw(Camera &camera, int sample_count)
{

    // ImGuiフレーム開始
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ---- UIの定義 ----
    ImGui::SetWindowPos(ImVec2(-100, -100), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(200, 50), ImGuiCond_Once);

    ImGui::Begin("Render Data");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Smaple Count: %d", sample_count);

    if (ImGui::CollapsingHeader("Camera"))
    {
        glm::vec3 pos = camera.GetPosition();
        ImGui::Text("Position: (%.1f,%.1f,%.1f)", pos.x, pos.y, pos.z);
        // ImGui::Text("    vFOV: %.1f", camera.GetVfov());
        float vfov = camera.GetVfov(), focusDist = camera.GetFocusDist(), defousAngle = camera.GetDefocusAngle();
        if (ImGui::DragFloat("vFov", &vfov, 0.2f, 20.f, 120.f))
            camera.SetVfov(vfov);
        if (ImGui::DragFloat("Focus Dist", &focusDist, 0.2f, 1.0f, 10.0f))
            camera.SetFocusDist(focusDist);
        if (ImGui::DragFloat("Defocus Angle", &defousAngle, 0.2f, 0.0f, 10.0f))
            camera.SetDefocusAngle(defousAngle);
    }
    ImGui::End();
    //------------------

    // レンダリング
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiController::Destroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}