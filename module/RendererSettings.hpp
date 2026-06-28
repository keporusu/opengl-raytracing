#pragma once

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

//以下の定数はシェーダと一致させる
//TODO: シェーダ読込み時に値を書き換える処理が必要

//BVHを使うか？
#define USE_BVH true

// プリミティブの最大描画数
#define MAX_SPHERES 100
#define MAX_QUADS 100

// 使えるマテリアルの最大数
#define MAX_MATERIALS 100
//BVHに使えるノードの最大数
#define MAX_BVH_NODES 500

// プリミティブタイプ定数
#define PRIM_TYPE_SPHERE 0
#define PRIM_TYPE_QUAD 1

// マテリアルタイプ定数
#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2
#define MATERIAL_DIFFUSE_LIGHT 10

// レイが当たらなかったときにサンプルする色（背景）のタイプ定数
#define SKY_TYPE_BLUE 0
#define SKY_TYPE_DARK 1
#define SKY_TYPE_A_BIG_LIGHT 2