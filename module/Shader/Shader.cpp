#include "Shader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <glad/glad.h>
#include "../../third_party/glm/gtc/type_ptr.hpp"

void Shader::Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
{
    // ソースコードの抽出
    auto vertexShaderSource = getCode(vertexShaderPath);
    auto fragmentShaderSource = getCode(fragmentShaderPath);

    // ソースコードのコンパイル
    vertexShader = compile(vertexShaderSource, GL_VERTEX_SHADER);
    fragmentShader = compile(fragmentShaderSource, GL_FRAGMENT_SHADER);

    // シェーダのリンク、プログラムの作成
    shaderProgram = createProgram();
}

void Shader::Use()
{
    if (!shaderProgram)
    {
        std::string errorMsg = "ERROR::プログラムが存在しません";
        std::cerr << errorMsg << std::endl;
        throw std::runtime_error(errorMsg);
        return;
    }
    glUseProgram(shaderProgram);
}

void Shader::Delete()
{
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}

void Shader::SetUniform(const std::string &name, float value)
{
    glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void Shader::SetUniform(const std::string &name, float value1, float value2, float value3)
{
    glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), value1, value2, value3);
}
void Shader::SetUniform(const std::string &name, glm::vec3 value)
{
    glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), value.x, value.y, value.z);
}
void Shader::SetUniform(const std::string &name, unsigned int value)
{
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void Shader::SetUniform(const std::string &name, int value)
{
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void Shader::SetUniform(const std::string &name, glm::mat4 value)
{
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
}
void Shader::BindUniformBlock(const std::string &name, unsigned bindingPoint)
{
    GLuint blockIndex = glGetUniformBlockIndex(shaderProgram, name.c_str());
    glUniformBlockBinding(shaderProgram, blockIndex, bindingPoint);
}

unsigned int Shader::compile(const std::string &shaderSource, const int shaderType)
{

    const char *sourcePtr = shaderSource.c_str();
    unsigned int shader = glCreateShader(shaderType);

    glShaderSource(shader, 1, &sourcePtr, NULL);
    glCompileShader(shader);

    // コンパイルチェック
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::string errorMsg = "NONE";
        if (shaderType == GL_VERTEX_SHADER)
        {
            errorMsg = "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n";
        }
        if (shaderType == GL_FRAGMENT_SHADER)
        {
            errorMsg = "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n";
        }
        std::cerr << errorMsg << infoLog << std::endl;
        throw std::runtime_error(errorMsg);
    }
    return shader;
}

unsigned int Shader::createProgram()
{
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    {
        // リンクチェック
        int success;
        char infoLog[512];
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            std::string errorMsg = "ERROR::SHADER::LINKING_FAILED\n";
            std::cerr << errorMsg << infoLog << std::endl;
            throw std::runtime_error(errorMsg);
        }
    }
    return program;
}

std::string Shader::getCode(const std::string &filepath) const
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::string errorMsg = "Failed to open shader file: " + filepath;
        std::cerr << errorMsg << std::endl;
        throw std::runtime_error(errorMsg);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();

    if (code.empty())
    {
        std::string errorMsg = "Shader file is empty: " + filepath;
        std::cerr << errorMsg << std::endl;
        throw std::runtime_error(errorMsg);
    }

    return code;
}