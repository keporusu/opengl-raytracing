#include <string>
// #include<iostream>

class ShaderManager
{

public:
    ShaderManager(const std::string &vertexShaderPath, const std::string &fragmentShaderPath)
    {
        Load(vertexShaderPath,fragmentShaderPath);
    }
    ~ShaderManager()
    {
        DeleteShaders();
    }

    void Load(const std::string &vertexShaderPath, const std::string &fragmentShaderPath);
    void Use();
    void DeleteShaders();

    void SetUniform(const std::string &name, const float value);//スカラー入力
    void SetUniform(const std::string &name, float value1, float value2, float value3);//３次元入力
    void SetUniform(const std::string &name, unsigned int value);//整数スカラー入力

private:
    unsigned int compile(const std::string &shaderSource, const int shaderType);
    unsigned int createProgram();

    unsigned int vertexShader = NULL;
    unsigned int fragmentShader = NULL;
    unsigned int shaderProgram = NULL;

    // ソースコードの抽出
    std::string getCode(const std::string &filepath) const;
};