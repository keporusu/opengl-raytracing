#pragma once
#include <string>
#include <vector>
#include <map>
#include <glad/glad.h>
#include "../Shader/Shader.hpp"

struct TextureUniformSet
{
    int unit_index;
    std::string uniform_name;
};

struct TextureLabelSet
{
    GLuint texture_id;
    std::string texture_label;
};

class Texture
{
public:
    Texture(std::string image_path, std::string label);
    static TextureUniformSet GetUniformData(int index);
    static void ActivateTextures();
    static int GetTextureCount();
    static int GetTextureIndex(std::string &label);

private:
    inline static std::vector<TextureLabelSet> textures;
};