#include "Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../../third_party/stb_image.h"

Texture::Texture(std::string image_path, std::string label)
{
    // テクスチャ
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(image_path.c_str(), &width, &height, &channels, 0);
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

    textures.push_back(TextureLabelSet{
        .texture_id = texture,
        .texture_label = label,
    });
}

TextureUniformSet Texture::GetUniformData(int index)
{
    return TextureUniformSet{
        .unit_index = 10 + index,
        .uniform_name = "u_texture" + std::to_string(index)};
}

void Texture::ActivateTextures()
{
    for (int i = 0; i < textures.size(); i++)
    {
        glActiveTexture(GL_TEXTURE10 + i);
        glBindTexture(GL_TEXTURE_2D, textures[i].texture_id); // テクスチャ
    }
}

int Texture::GetTextureCount()
{
    return textures.size();
}

int Texture::GetTextureIndex(std::string &label)
{
    for (int i = 0; i < textures.size(); i++)
    {
        if (textures[i].texture_label == label)
        {
            return i;
        }
    }
    return -1;
}
