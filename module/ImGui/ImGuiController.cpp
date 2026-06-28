#include "ImGuiController.hpp"
#include <GLFW/glfw3.h>

ImGuiController::ImGuiController(GLFWwindow *window,float uiVFov,float uiFocusDist,float uiDefocusAngle)
{

    // GLFW・OpenGLの初期化後に実行
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // スタイル設定（任意）
    ImGui::StyleColorsDark();

    // バックエンドの初期化
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    //ui内部の値を初期化
    this->uiVFov=uiVFov;
    this->uiFocusDist=uiFocusDist;
    this->uiDefocusAngle=uiDefocusAngle;
}

void ImGuiController::SetCameraPosition(float x,float y,float z){
    uiCameraX=x;
    uiCameraY=y;
    uiCameraZ=z;
}
void ImGuiController::SetSampleCount(int n){
    uiSampleCount=n;
}

void ImGuiController::Draw()
{

    // ImGuiフレーム開始
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // ---- UIの定義 ----
    ImGui::SetWindowPos(ImVec2(-100, -100), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(200, 150), ImGuiCond_Once);

    ImGui::Begin("Render Data");
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Sample Count: %d", uiSampleCount);

    if(ImGui::CollapsingHeader("Scene")){
        if(ImGui::Button("Cornell Box",ImVec2(160,20))){
            if(OnClickCornellBox) OnClickCornellBox();
        }
        if(ImGui::Button("Many Balls",ImVec2(160,20))){
            if(OnClickManyBalls) OnClickManyBalls();
        }
    }

    if (ImGui::CollapsingHeader("Camera"))
    {
        ImGui::Text("Position: (%.1f,%.1f,%.1f)", uiCameraX,uiCameraY,uiCameraZ);
        // ImGui::Text("    vFOV: %.1f", camera.GetVfov());
        if (ImGui::DragFloat("vFov", &uiVFov, 0.2f, 20.f, 120.f)){
            if(OnUpdateVfov)OnUpdateVfov(uiVFov);
        }
        if (ImGui::DragFloat("Focus Dist", &uiFocusDist, 0.2f, 1.0f, 10.0f)){
            if(OnUpdateFocusDist)OnUpdateFocusDist(uiFocusDist);
        }
        if (ImGui::DragFloat("Defocus Angle", &uiDefocusAngle, 0.2f, 0.0f, 10.0f)){
            if(OnUpdateDefocusAngle)OnUpdateDefocusAngle(uiDefocusAngle);
        }
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