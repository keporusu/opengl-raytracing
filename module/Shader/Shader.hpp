#pragma once
#include <string>
#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#include <variant>

class Shader
{

public:
    Shader(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
    {
        Load(vertexShaderPath, fragmentShaderPath);
    }
    virtual ~Shader()
    {
        Delete();
    }

    void Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    void Use();
    void Delete();

    void SetUniform(const std::string &name, const float value);                        // スカラー入力
    void SetUniform(const std::string &name, float value1, float value2, float value3); // ３次元入力
    void SetUniform(const std::string &name, glm::vec3 value);                          // ３次元入力
    void SetUniform(const std::string &name, unsigned int value);
    void SetUniform(const std::string &name, int value);                       // 整数スカラー入力
    void SetUniform(const std::string &name, glm::mat4 value);                          // 行列

    void BindUniformBlock(const std::string &name, unsigned bindingPoint);


private:
    unsigned int compile(const std::string &shaderSource, const int shaderType);
    unsigned int createProgram();

    unsigned int vertexShader = NULL;
    unsigned int fragmentShader = NULL;
    unsigned int shaderProgram = NULL;

    // ソースコードの抽出
    std::string getCode(const std::string &filepath) const;
};