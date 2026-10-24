cmake_minimum_required(VERSION 3.16)
project(opengl-raytracing)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 単体で配布できる実行ファイルにするため、MSVCではCRTを静的リンクする。
# これによりVC++再頒布可能パッケージのインストールが不要になる。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(OpenGL REQUIRED)

include(FetchContent)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

####
# シェーダ・テクスチャの埋め込み
#
# 実行ファイル単体で動作させるため、外部ファイルを一切参照しないようにする。
# リソースを更新すると再生成される。
####
set(EMBEDDED_RESOURCES
    shader/RayTracing/RayTracing.vs
    shader/RayTracing/RayTracing.fs
    shader/Output/Output.vs
    shader/Output/Output.fs
    resources/textures/earthmap.png
)

set(EMBEDDED_RESOURCES_CPP ${CMAKE_CURRENT_BINARY_DIR}/generated/EmbeddedResources.cpp)

# 依存関係を絶対パスで列挙し、リソース変更時に再生成されるようにする
set(EMBEDDED_RESOURCES_DEPS "")
foreach(RES IN LISTS EMBEDDED_RESOURCES)
    list(APPEND EMBEDDED_RESOURCES_DEPS ${CMAKE_SOURCE_DIR}/${RES})
endforeach()

add_custom_command(
    OUTPUT ${EMBEDDED_RESOURCES_CPP}
    COMMAND ${CMAKE_COMMAND}
            -DGEN_OUTPUT=${EMBEDDED_RESOURCES_CPP}
            -DGEN_ROOT=${CMAKE_SOURCE_DIR}
            "-DGEN_FILES=${EMBEDDED_RESOURCES}"
            -P ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedResources.cmake
    DEPENDS ${EMBEDDED_RESOURCES_DEPS}
            ${CMAKE_SOURCE_DIR}/cmake/GenerateEmbeddedResources.cmake
    COMMENT "リソースを実行ファイルへ埋め込んでいます"
    VERBATIM
)

set(SOURCES
    ${EMBEDDED_RESOURCES_CPP}
    main.cpp
    module/Loader/ModelLoader.cpp
    module/Loader/ShaderManager.cpp
    module/ImGui/ImGuiController.cpp
    module/Scene/Scene.cpp
    module/Shader/Shader.cpp
    module/Texture/Texture.cpp
    third_party/glad.c
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
    third_party/imgui/backends/imgui_impl_glfw.cpp
    third_party/imgui/backends/imgui_impl_opengl3.cpp
)

add_executable(app ${SOURCES})

if(MSVC)
    # ソースはUTF-8。既定ではコードページ932として解釈され警告C4819が出るため明示する
    target_compile_options(app PRIVATE /utf-8)
endif()

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/third_party
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(app PRIVATE
    OpenGL::GL
    glfw
)

if(APPLE)
    target_link_libraries(app PRIVATE
        z
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreFoundation"
    )
endif()
