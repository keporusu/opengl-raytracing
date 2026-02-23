#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include "module/Loader/ModelLoader.hpp"
#include "module/Shader/Shader.hpp"
#include "module/Scene/Scene.hpp"
#include "module/Camera/Camera.hpp"
#define STB_IMAGE_IMPLEMENTATION

#define GL_NO_BINDING 0
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

void frameBufferSizeCallback(GLFWwindow *window, int width, int height);
void scrollCallback(GLFWwindow *window, double x_offset, double y_offset);
void processInput(GLFWwindow *window, Camera &camera);

int main()
{
    glfwInit();

    ////
    // OpenGLのバージョン・機能を指定
    ////
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    ////
    // ウィンドウの作成
    ////
    auto windowDeleter = [](GLFWwindow *w)
    {
        if (w)
            glfwDestroyWindow(w);
    };
    std::unique_ptr<GLFWwindow, decltype(windowDeleter)> window(
        glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Renderer", NULL, NULL),
        windowDeleter);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window.get());

    ////
    // GLADの初期化
    ////
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    ////
    // 蓄積用テクスチャ用意
    ////
    unsigned accumTexture;
    glGenTextures(1, &accumTexture);
    glBindTexture(GL_TEXTURE_2D, accumTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, GL_NO_BINDING);

    ////
    // モデルのロード、バッファとのバインド
    ////
    std::vector<float> vertices = ModelLoader::GetQuadVertices();
    std::vector<unsigned int> indices = ModelLoader::GetQuadIndices();
    Scene scene;
    Camera camera(glm::vec3(0.0f, 0.0f, 3.0f), (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT);

    GLuint VBO, VAO, EBO;

    // ID生成
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // 頂点設定の記録場所を指定
    glBindVertexArray(VAO);

    // バッファとバインド、バッファへ頂点データ送信
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // バッファの解釈方法指定、属性インデックスの有効化（layoutと対応）
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0); // 頂点座標
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1); // uv座標

    // バッファとのバインドを解除
    glBindBuffer(GL_ARRAY_BUFFER, GL_NO_BINDING);
    glBindVertexArray(GL_NO_BINDING);
    glBindTexture(GL_TEXTURE_2D, GL_NO_BINDING);

    // フレームバッファ設定-解除
    GLuint accumFBO;
    glGenFramebuffers(1, &accumFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glBindFramebuffer(GL_FRAMEBUFFER, GL_NO_BINDING);

    // UBO設定
    GLuint primitivesUBO, cameraUBO, materialsUBO;
    // プリミティブ
    glGenBuffers(1, &primitivesUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, primitivesUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO_Primitives), scene.GetPrimitivesUBO(), GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, primitivesUBO); // BindingPoint 0
    // カメラ
    glGenBuffers(1, &cameraUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO_Camera), camera.GetUBO(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, cameraUBO); // BindingPoint 1
    // マテリアル
    glGenBuffers(1, &materialsUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, materialsUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(UBO_Materials), scene.GetMateialsUBO(), GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, materialsUBO); // BindingPoint 2

    ////
    // シェーダのコンパイル・オブジェクト作成
    ////
    auto raytracing_program = Shader("shader/RayTracing/RayTracing.vs", "shader/RayTracing/RayTracing.fs");
    auto output_program = Shader("shader/Output/Output.vs", "shader/Output/Output.fs");
    raytracing_program.BindUniformBlock("PrimitivesBlock", 0);
    raytracing_program.BindUniformBlock("CameraBlock", 1);
    raytracing_program.BindUniformBlock("MaterialsBlock", 2);

    ////
    // コールバック
    ////
    glfwSetFramebufferSizeCallback(window.get(), frameBufferSizeCallback);
    glfwSetWindowUserPointer(window.get(), &camera);
    glfwSetScrollCallback(window.get(), scrollCallback);

    ////
    // Rendering Loop
    ////
    int sample_count = 0;
    int frame = 0;
    int fpsCount = 0;
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window.get()))
    {
        // FPS計測
        double currentTime = glfwGetTime();
        fpsCount++;
        if (currentTime - lastTime >= 1.0) // 1秒ごとに出力
        {
            std::cout << "FPS: " << fpsCount << std::endl;
            fpsCount = 0;
            lastTime = currentTime;
        }

        // 入力処理
        processInput(window.get(), camera);

        // uniform関連
        {
            frame++;

            if (camera.IsChanged())
                sample_count = 0;
            sample_count++;
        }

        ////
        // レンダリング
        ////
        ////レイトレーシングパス
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO); // フレームバッファの指定
        
        //////1サンプル目ならフレームバッファは初期化する
        if (sample_count == 1)
        {
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // この色で
            glClear(GL_COLOR_BUFFER_BIT); // FrameBufferを初期化
        }

        //////ブレンディング有効化, ブレンド設定（フレームバッファに足す）
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        //////シェーダ
        raytracing_program.Use();
        raytracing_program.SetUniform("reset_frame_buffer", camera.IsChanged() ? 1u : 0u);
        raytracing_program.SetUniform("ray_sample_number", (unsigned)sample_count);
        raytracing_program.SetUniform("u_frame", (float)frame);

        //////カメラUBO送信
        glBindBuffer(GL_UNIFORM_BUFFER, cameraUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(UBO_Camera), camera.GetUBO());
        //////描画
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(GL_NO_BINDING);

        ////描画パス
        glBindFramebuffer(GL_FRAMEBUFFER, GL_NO_BINDING); // デフォルトのフレームバッファに戻す
        glDisable(GL_BLEND);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT); // 画面を初期化

        //////シェーダ
        output_program.Use();
        output_program.SetUniform("accumTexture", 0u);
        output_program.SetUniform("ray_sample_number", (unsigned)sample_count);
        //////テクスチャをバインドしてシェーダーに渡す
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        //////描画
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(GL_NO_BINDING);

        camera.ChangeFlagOff();

        // glfw: イベントのトリガをチェック、フレームバッファの入れ替え（ここで初めて画面に見える）
        glfwPollEvents();
        glfwSwapBuffers(window.get());
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    output_program.Delete();

    glfwTerminate();
    return 0;
}

void frameBufferSizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}
void scrollCallback(GLFWwindow *window, double x_offset, double y_offset)
{
    Camera *camera = static_cast<Camera *>(glfwGetWindowUserPointer(window));
    if (camera)
    {
        camera->Zoom((float)y_offset * 0.2f);
    }
}

void processInput(GLFWwindow *window, Camera &camera)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera.Move(glm::vec3(-0.01f, 0.0f, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera.Move(glm::vec3(0.01f, 0.0f, 0.0f));
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera.Move(glm::vec3(0.0f, 0.0f, -0.01f));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera.Move(glm::vec3(0.0f, 0.0f, 0.01f));
    }

    int viewport_width, viewport_height;
    glfwGetWindowSize(window, &viewport_width, &viewport_height);
    camera.SetAspectRatio(float(viewport_width) / float(viewport_height));
}