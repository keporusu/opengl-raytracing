#pragma once
#include <GLFW/glfw3.h>
#include "../Camera/Camera.hpp"
#include <array>

class InputSystem
{

public:
    InputSystem()
    {
    }

    void Init(GLFWwindow *window)
    {

        // ポインタ登録
        glfwSetWindowUserPointer(window, this);

        // フレームバッファの変更
        glfwSetFramebufferSizeCallback(window, [](GLFWwindow *w, int width, int hegiht)
                                       {
            auto* self=static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
            self->frameBufferSize={width,hegiht}; });
        // スクロール
        glfwSetScrollCallback(window, [](GLFWwindow *w, double x_offset, double y_offset)
                              {
            auto* self = static_cast<InputSystem*>(glfwGetWindowUserPointer(w));
            self->scrollDelta = (float)y_offset; });
    }

    void Update(GLFWwindow *window)
    {
        float nowSecond = (float)glfwGetTime();
        deltaSecond = nowSecond - lastSecond;
        lastSecond = nowSecond;

        // 前フレームの状態を保存
        prevKeys = currKeys;
        prevMouseButtons = currMouseButtons;
        prevMousePos = currMousePos;

        // GLFWから現在状態を収集（ここだけGLFW依存）
        for (int i = 0; i < GLFW_KEY_LAST; ++i)
            currKeys[i] = glfwGetKey(window, i) == GLFW_PRESS;

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        currMousePos = {(float)x, (float)y};

        // escは問答無用で終了させる
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    bool IsKeyDown(int key) const { return currKeys[key]; }
    bool IsKeyPressed(int key) const { return currKeys[key] && !prevKeys[key]; }
    bool IsKeyReleased(int key) const { return !currKeys[key] && prevKeys[key]; }

    glm::vec2 GetMouseDelta() const { return currMousePos - prevMousePos; }
    glm::vec2 GetFrameBufferSize() const { return frameBufferSize; }
    float GetScrollDelta() const { return scrollDelta; }
    float GetDeltaTime() const { return deltaSecond; }

private:
    glm::vec2 frameBufferSize;
    float scrollDelta;
    float lastSecond = 0.0f;
    float deltaSecond;

    std::array<bool, GLFW_KEY_LAST> currKeys{}, prevKeys{};
    std::array<bool, GLFW_MOUSE_BUTTON_LAST> currMouseButtons{}, prevMouseButtons{};
    glm::vec2 currMousePos{}, prevMousePos{};
};