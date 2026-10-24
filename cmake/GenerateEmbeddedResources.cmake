# シェーダ・テクスチャを実行ファイルへ埋め込むためのC++ソースを生成する。
#
# 呼び出し方:
#   cmake -DGEN_OUTPUT=<出力する.cpp> -DGEN_ROOT=<リソースの基準ディレクトリ>
#         -DGEN_FILES=<GEN_ROOTからの相対パスをセミコロン区切りで>
#         -P GenerateEmbeddedResources.cmake
#
# 生成物は GEN_FILES の相対パスをキーとした (データ, サイズ) の表を持つ。
# キーはソースコード中のパス文字列とそのまま一致するため、呼び出し側の変更は不要。

set(BLOBS "")
set(TABLE "")
set(INDEX 0)

foreach(REL IN LISTS GEN_FILES)
    set(ABS "${GEN_ROOT}/${REL}")
    if(NOT EXISTS "${ABS}")
        message(FATAL_ERROR "埋め込むリソースが見つかりません: ${ABS}")
    endif()

    # バイナリを16進文字列として読み、0x.. のC配列初期化子へ変換する
    file(READ "${ABS}" HEX HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," BYTES "${HEX}")
    # 1行が極端に長くならないよう16バイトごとに改行を入れる
    string(REGEX REPLACE "(0x..,0x..,0x..,0x..,0x..,0x..,0x..,0x..,)" "\\1\n" BYTES "${BYTES}")

    string(APPEND BLOBS "static const unsigned char kBlob${INDEX}[] = {\n${BYTES}\n};\n\n")
    string(APPEND TABLE "    { \"${REL}\", kBlob${INDEX}, sizeof(kBlob${INDEX}) },\n")
    math(EXPR INDEX "${INDEX} + 1")
endforeach()

file(WRITE "${GEN_OUTPUT}"
"// このファイルはビルド時に自動生成されます。直接編集しないでください。
// 生成元: cmake/GenerateEmbeddedResources.cmake

#include \"module/Resources/EmbeddedResources.hpp\"

namespace
{
${BLOBS}struct Entry
{
    const char *path;
    const unsigned char *data;
    size_t size;
};

const Entry kEntries[] = {
${TABLE}};
} // namespace

namespace EmbeddedResources
{
Blob Get(const std::string &path)
{
    for (const Entry &entry : kEntries)
    {
        if (path == entry.path)
        {
            return Blob{entry.data, entry.size};
        }
    }
    return Blob{nullptr, 0};
}
} // namespace EmbeddedResources
")
