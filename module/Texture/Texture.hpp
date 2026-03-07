#pragma once
#include <string>
#include <vector>
#include <glad/glad.h>
#include "../Shader/Shader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

struct TextureUniformSet
{
    int unit_index;
    std::string uniform_name;
};

class Texture
{

public:
    Texture(std::string image_path)
    {
        // テクスチャ
        stbi_set_flip_vertically_on_load(true);
        int width, height, channels;
        unsigned char *data = stbi_load("resources/textures/earthmap.png", &width, &height, &channels, 0);
        // チャンネル数に応じてフォーマットを選択
        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);

        textures.push_back(texture);
    }

    static TextureUniformSet GetUniformData(int index)
    {
        return TextureUniformSet{
            .unit_index = 10 + index,
            .uniform_name = "u_texture" + std::to_string(index)};
    }
    static void ActivateTextures()
    {
        for (int i = 0; i < textures.size(); i++)
        {
            glActiveTexture(GL_TEXTURE10 + i);
            glBindTexture(GL_TEXTURE_2D, textures[i]); // テクスチャ
        }
    }
    static int GetTextureCount() { return textures.size(); }

private:
    inline static std::vector<GLuint> textures;
};