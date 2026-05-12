# opengl-raytracing

OpenGL/GLSLを用いたパストレーシングレンダラです。フレームを跨いでサンプルを蓄積するプログレッシブレンダリングを実装しています。

## 技術

| 種別 | 内容 |
|---|---|
| 言語 | C++17 |
| グラフィックスAPI | OpenGL 3.3 Core Profile / GLSL 330 core |
| ビルドシステム | CMake 3.16+ |
| ウィンドウ・入力 | GLFW 3.4（CMakeが自動取得） |
| UI | Dear ImGui |
| 数学ライブラリ | GLM |
| 画像ロード | stb_image |
| OpenGL関数ロード | glad |

## 動作要件

- OpenGL 3.3 以上に対応したGPU
- CMake 3.16 以上
- C++17 対応コンパイラ

## ビルド・実行

```bash
git clone https://github.com/keporusu/opengl-raytracing.git
cd opengl-raytracing

cmake -B build
cmake --build build --parallel

# macOS / Linux
./build/app

# Windows
.\build\Debug\app.exe
```

初回のcmakeコマンド実行時にGLFWが自動でダウンロード・ビルドされます。

## 操作方法

| 操作 | 動作 |
|---|---|
| W / A / S / D | カメラの公転（上下左右） |
| スクロール | カメラの距離調整 |
| R | カメラのリセット |
| ESC | 終了 |

カメラを動かすとサンプル蓄積がリセットされます。

## ライセンス

本リポジトリのコードは [MIT License](LICENSE) の下で公開されています。

| ライブラリ | ライセンス |
|---|---|
| [GLFW](https://www.glfw.org/) | zlib/libpng License |
| [Dear ImGui](https://github.com/ocornut/imgui) | MIT License |
| [GLM](https://github.com/g-truc/glm) | MIT License |
| [stb_image](https://github.com/nothings/stb) | MIT License / Public Domain |
| [glad](https://glad.dav1d.de/) | MIT License |
