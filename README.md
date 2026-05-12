# opengl-raytracing

OpenGL/GLSLを用いたパストレーシングレンダラです。フレームを跨いでサンプルを蓄積するプログレッシブレンダリングを実装しています。

## 技術スタック

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

## 実装技術

### レンダリング方法

フラグメントシェーダ上でパストレーシングを実行する2パス構成です。

- **Pass 1（レイトレーシング）**: FBOにアタッチした RGB32F テクスチャへ `GL_ONE + GL_ONE` ブレンドで毎フレーム1サンプルを加算
- **Pass 2（出力）**: 蓄積値をサンプル数で平均化し、ガンマ補正（√）をかけてスクリーンへ出力
- カメラを動かすとサンプル蓄積をリセットし再収束

### BVH（Bounding Volume Hierarchy）

C++ 側でバイナリBVHを構築し、フラット化した配列をUBOでGPUに転送しています。

### マテリアル

| 種類 | 概要 |
|---|---|
| Lambertian（拡散） | ランバートBRDF（cosθ / π）。コサイン加重半球サンプリング |
| Metal（金属） | 鏡面反射。fuzzパラメータで反射方向にランダムノイズを加えたFuzzy Reflection |
| Dielectric（誘電体） | Schlick近似によるFresnel効果と全反射判定。 |
| Diffuse Light（発光体） | 発光のみ行い反射しない。背面からは発光しない |

### サンプリング戦略

- **NEE（Next Event Estimation）**: 各バウンスでライト上の点を直接サンプリングしてシャドウレイを飛ばす
- **MIS（Multiple Importance Sampling）**: BRDFサンプリングとライトサンプリングを 50:50 の重みで混合
- **コサイン加重半球サンプリング**: Lambertian向けに cosθ に比例した方向をサンプリング

### カメラ

- 公転移動
- **被写界深度（DOF）**: 開口をユニットディスクでサンプリングしフォーカス距離でスケール

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
