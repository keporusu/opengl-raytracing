#include "ShaderManager.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <glad/glad.h>

void ShaderManager::Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
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

void ShaderManager::Use()
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

void ShaderManager::DeleteShaders()
{
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}

void ShaderManager::SetUniform(const std::string &name, float value)
{
    glUniform1f(glGetUniformLocation(shaderProgram, name.c_str()), value);
}
void ShaderManager::SetUniform(const std::string &name, float value1, float value2, float value3)
{
    glUniform3f(glGetUniformLocation(shaderProgram, name.c_str()), value1, value2, value3);
}
void ShaderManager::SetUniform(const std::string &name, unsigned int value)
{
    glUniform1i(glGetUniformLocation(shaderProgram, name.c_str()), value);
}

unsigned int ShaderManager::compile(const std::string &shaderSource, const int shaderType)
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

unsigned int ShaderManager::createProgram()
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

std::string ShaderManager::getCode(const std::string &filepath) const
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