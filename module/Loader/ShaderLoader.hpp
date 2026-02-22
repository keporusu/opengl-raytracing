#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

class ShaderLoader
{

public:
    // ただシェーダファイルを文字列に変換するだけ
    static std::string LoadShaderSource(const char *filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open shader file: " << filepath << std::endl;
            return "";
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};