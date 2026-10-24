#pragma once
#include <cstddef>
#include <string>

// 実行ファイルへ埋め込まれたシェーダ・テクスチャへのアクセス。
// 実体は cmake/GenerateEmbeddedResources.cmake がビルド時に生成する。
namespace EmbeddedResources
{
struct Blob
{
    const unsigned char *data; // 見つからない場合は nullptr
    size_t size;
};

// キーはソースコード中のパス文字列（例: "shader/Output/Output.fs"）
Blob Get(const std::string &path);
} // namespace EmbeddedResources
