#version 330 core
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define USE_BVH true

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// utility
////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float infinity = 3.402823e+38;
const float PI = 3.14159265359;
const vec3 v_up = vec3(0.0, 1.0, 0.0);
const float eps = 1e-4;
const float eps2 = 1e-8;

vec3 linear_to_gamma(vec3 c) {
    return sqrt(max(c, 0.0));
}
bool near_zero(vec3 v) {
    const float e = eps2;
    return abs(v.x) < e && abs(v.y) < e && abs(v.z) < e;
}
void swap(inout float a, inout float b) {
    float v = a;
    a = b;
    b = v;
}
vec3 safe_normalize(vec3 v) {
    float l = length(v);
    return l > eps ? v / l : vec3(0.0);
}

struct OrthonomalBasis {
    vec3 u, v, w;
};
//法線周りに正規直交基底を作る
OrthonomalBasis create_orthonomal_basis(vec3 n) {
    vec3 w = normalize(n);
    vec3 a = abs(n.x) > 0.9 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 v = normalize(cross(w, a));
    vec3 u = cross(w, v);
    return OrthonomalBasis(u, v, w);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 乱数
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//オリジナル
uint esgtsa_orig(uint seed) {
    uint s = seed;
    s = (s ^ 2747636419u) * 2654435769u;
    s = (s ^ s >> 16u) * 2654435769u;
    s = (s ^ s >> 16u) * 2654435769u;
    return s;
} 
//スカラー0~1
float rand(vec4 v) {
    return float(esgtsa_orig(esgtsa_orig(esgtsa_orig(esgtsa_orig(uint(v.x)) + uint(v.y)) + uint(v.z)) + uint(v.w))) * 2.3283064365386962890625e-10;
}
//2次元0~1
vec2 rand2(vec4 v) {
    float r1 = rand(v);
    float r2 = rand(vec4(v.x, v.y, v.z, v.w + 1.0));
    return vec2(r1, r2);
}
//3次元0~1
vec3 rand3(vec4 v) {
    float r1 = rand(v);
    float r2 = rand(vec4(v.x, v.y, v.z, v.w + 1.0));
    float r3 = rand(vec4(v.x, v.y, v.z, v.w + 2.0));
    return vec3(r1, r2, r3);
}
//３次元ベクトル-1~1（長さは1以下）
vec3 random_unit_vector(vec4 v) {
    for(int i = 0; i < 100; i++) {
        vec3 p = rand3(vec4(v.x, v.y, v.z, v.w + float(i) * 3.0)) * 2.0 - 1.0;
        float lensq = dot(p, p);
        if(1e-160 < lensq && lensq <= 1.0)
            return p / sqrt(lensq);
    }
    return vec3(0.0, 1.0, 0.0);
}
//3次元ベクトル半球面上
vec3 random_on_hemisphere(vec3 normal, vec4 v) {
    vec3 unit_vec_on_sphere = random_unit_vector(v);
    if(dot(unit_vec_on_sphere, normal) > 0.0) {
        return unit_vec_on_sphere;
    } else {
        return -unit_vec_on_sphere;
    }
    return vec3(0.0, 1.0, 0.0);
}
//2次元ベクトル-1~1（長さは1以下）
vec2 random_in_unit_disk(vec4 v) {
    for(int i = 0; i < 100; i++) {
        vec2 p = rand2(vec4(v.x, v.y, v.z, v.w + float(i) * 3.0)) * 2.0 - 1.0;
        if(dot(p, p) < 1.0) {
            return p;
        }
    }
    return vec2(0.0, 1.0);
}
//3次元
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////
// Const
//////////////////////////////////////////////////////
#define MAX_SPHERES 100
#define MAX_QUADS 100
#define MATERIAL_MAX 100
#define ERROR_COLOR vec3(1.0,0.0,1.0)
#define MAX_BVH_NODES 500

#define PRIM_TYPE_SPHERE 0
#define PRIM_TYPE_QUAD 1

#define MATERIAL_LAMBERTIAN 0
#define MATERIAL_METAL 1
#define MATERIAL_DIELECTRIC 2
#define MATERIAL_DIFFUSE_LIGHT 3

//////////////////////////////////////////////////////
// Background Sky
//////////////////////////////////////////////////////
vec3 blue_sky(float y) {
    float a = 0.5 * (y + 1.0);
    return (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
}
vec3 one_big_light(float y) {
    float a = 0.5 * (y + 1.0);
    a = pow(a, 10.0);
    return (1.0 - a) * vec3(0.0) + a * vec3(1.0, 1.0, 1.0);
}
vec3 dark() {
    return vec3(0.0);
}
vec3 background_sky(vec3 dir) {
    //return blue_sky(dir.y);
    //return dark();
    return one_big_light(dir.y);
}
//////////////////////////////////////////////////////
// データ構造
//////////////////////////////////////////////////////
//レイ
struct Ray {
    vec3 origin;
    vec3 direction;
};
//レイがヒットした場所の情報
struct HitRecord {
    vec3 point;
    vec3 normal;
    float ray_pram;
    bool front_face;
    int material;
    vec2 uv;
    int primitive;
};
//AABB
struct AlignedBox {
    float x_min, x_max;
    float y_min, y_max;
    float z_min, z_max;
};
struct BVHNode {
    float x_min, x_max;
    float y_min, y_max;
    float z_min, z_max;
    int left;
    int right;
    int prim_index;
    int prim_type;
};

// Primitives
struct Sphere {
    vec3 center;
    float radius;
    int material;
};
struct Quad {
    vec3 point;
    vec3 u, v;
    vec3 normal;
    float D;
    int material;
};
// Materials
struct Material {
    int material_type;
    vec3 albedo;
    float fuzz; //金属限定 ぼやかし
    float refraction_index; //誘電体限定 相対屈折率
    int texture; //テクスチャのインデックス
    vec3 emitted; //発光
};
//////////////////////////////////////////////////////
// in-out
//////////////////////////////////////////////////////
layout(location = 0) out vec3 color;
in vec2 TexCoord;

//////////////////////////////////////////////////////
// uniform
//////////////////////////////////////////////////////
uniform float u_frame;
uniform int ray_sample_number;
layout(std140) uniform BVHBlock {
    int node_count;
    BVHNode bvh_nodes[MAX_BVH_NODES];
};
layout(std140) uniform PrimitivesBlock {
    int sphere_count;
    Sphere spheres[MAX_SPHERES];
    int quad_count;
    Quad quads[MAX_QUADS];
};
const float focal_length = 1.0;
layout(std140) uniform CameraBlock {
    vec3 camera_pos;
    float aspect_ratio;
    float vfov;
    int max_depth;
    float defous_angle;
    float focus_dist;
};
layout(std140) uniform MaterialsBlock {
    int material_count;
    Material materials[MATERIAL_MAX];
};
//テクスチャ
uniform sampler2D u_texture0; //テクスチャ0番
vec3 sample_texture(int texture_index, vec2 uv) {
    if(texture_index == 0)
        return texture(u_texture0, uv).xyz;
    return ERROR_COLOR;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// プリミティブとの交点計算
////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool hit_sphere(Sphere sphere, Ray ray, out HitRecord hit_record, float ray_tmin, float ray_tmax) {
    //解: (-b +- sqrt(bb-4ac))/2a
    //線と球の交点計算
    float a = dot(ray.direction, ray.direction);
    vec3 orig_to_c = sphere.center - ray.origin;
    float h = dot(ray.direction, orig_to_c);
    float c = dot(orig_to_c, orig_to_c) - sphere.radius * sphere.radius;
    float discriminant = h * h - a * c;

    //交わらない
    if(discriminant < 0) {
        return false;
    }

    float sqrtd = sqrt(discriminant);
    // 許容範囲内の解を見つける
    float root = (h - sqrtd) / a;
    if(root <= ray_tmin || ray_tmax <= root) {
        root = (h + sqrtd) / a;
        if(root <= ray_tmin || ray_tmax <= root)
            return false;//解がない
    }

    //ヒット情報の書き込み
    hit_record.ray_pram = root;//解
    hit_record.point = root * ray.direction + ray.origin;//ヒット位置
    vec3 outward_normal = (hit_record.point - sphere.center) / sphere.radius;
    hit_record.front_face = dot(ray.direction, outward_normal) < 0; //当たったのは表面か？
    hit_record.normal = hit_record.front_face ? outward_normal : -outward_normal;//法線の向き
    hit_record.material = sphere.material;

    vec3 spherecal = safe_normalize(hit_record.point - sphere.center);
    float theta = acos(-spherecal.y);
    float phi = atan(-spherecal.z, spherecal.x) + PI;
    hit_record.uv = vec2(phi / (2.0 * PI), theta / PI);
    hit_record.primitive = PRIM_TYPE_SPHERE;

    //交わった
    return true;
}
bool hit_quad(Quad quad, Ray ray, out HitRecord hit_record, float ray_tmin, float ray_tmax) {
    //rayと平面の交わり
    //t=(D-n・P)/n・d
    float denom = dot(quad.normal, ray.direction);
    if(abs(denom) < eps2) {
        //平面とレイが平行なので交わらない
        return false;
    }
    float t = (quad.D - dot(quad.normal, ray.origin)) / denom;

    if(t < ray_tmin || ray_tmax < t) {
        return false;
    }
    //交点
    vec3 intersection = ray.origin + t * ray.direction;

    //intersectionが平行四辺形上にあるか？
    vec3 w = intersection - quad.point;
    vec3 uv_cross = cross(quad.u, quad.v); // = normal * area
    float area = dot(uv_cross, uv_cross);  // |u×v|²

    float s = dot(cross(w, quad.v), uv_cross) / area;
    float tt = dot(cross(quad.u, w), uv_cross) / area;
    if(s < 0.0 || 1.0 < s || tt < 0.0 || 1.0 < tt) {
        return false;
    }

    //ヒット情報書き込み
    hit_record.ray_pram = t;
    hit_record.point = intersection;
    vec3 outward_normal = quad.normal;
    hit_record.front_face = dot(outward_normal, ray.direction) < 0.0;
    hit_record.normal = hit_record.front_face ? outward_normal : -outward_normal;
    hit_record.primitive = PRIM_TYPE_QUAD;
    hit_record.material = quad.material;

    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// プリミティブとのヒット処理関連
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//aabbとレイが交差するか？
bool hit_aabb(AlignedBox aabb, Ray ray) {
    float ray_t_min = 1e-3;
    float ray_t_max = infinity;
    //TODO: いずれAlignedBoxの形を変えたほうが良さそう
    vec3 aabb_mins = vec3(aabb.x_min, aabb.y_min, aabb.z_min);
    vec3 aabb_maxs = vec3(aabb.x_max, aabb.y_max, aabb.z_max);

    //それぞれの軸で考える
    for(int axis = 0; axis < 3; axis++) {
        float invD = 1.0 / ray.direction[axis];
        float t0 = (aabb_mins[axis] - ray.origin[axis]) * invD;
        float t1 = (aabb_maxs[axis] - ray.origin[axis]) * invD;
        if(invD < 0.0)
            swap(t0, t1);
        ray_t_min = max(ray_t_min, t0);
        ray_t_max = min(ray_t_max, t1);
        if(ray_t_max <= ray_t_min) {
            return false;
        }
    }
    return true;
}
//BVHによるヒット処理
bool traverse_bvh(Ray ray, out HitRecord use_record) {
    //int best_primitive = -1;
    bool is_hit = false;
    float min_dist = infinity;
    //bvhのインデックスを保存するスタック（最初は0から）
    int bvh_index_stack[64];
    int stack_count = 0;
    bvh_index_stack[stack_count] = 0;
    stack_count++;
    while(stack_count > 0) {
        stack_count--;
        BVHNode node = bvh_nodes[bvh_index_stack[stack_count]];

        AlignedBox aabb = AlignedBox(node.x_min, node.x_max, node.y_min, node.y_max, node.z_min, node.z_max);

        bool hit_box = true;
        hit_box = hit_aabb(aabb, ray);

        if(!hit_box) {
            continue;
        }
        //葉に到達した場合
        if(node.prim_index >= 0) {
            HitRecord hit_record;
            bool hit = false;
            //どれかにヒットしたか
            if(node.prim_type == PRIM_TYPE_SPHERE) {
                hit = hit_sphere(spheres[node.prim_index], ray, hit_record, 1e-3, min_dist);
            } else if(node.prim_type == PRIM_TYPE_QUAD) {
                hit = hit_quad(quads[node.prim_index], ray, hit_record, 1e-3, min_dist);
            }
            //ヒットしてたら、ヒット情報を更新する
            if(hit && hit_record.ray_pram < min_dist) {
                min_dist = hit_record.ray_pram;
                use_record = hit_record;
                is_hit = true;
                //best_primitive = node.prim_index;
            }
        }
        //まだ葉に到達していない
        else {
            //スタックに左右のbvhインデックスを積む
            bvh_index_stack[stack_count] = node.right;
            stack_count++;
            bvh_index_stack[stack_count] = node.left;
            stack_count++;
        }
    }
    //return best_primitive;
    return is_hit;
}
//旧ヒット処理
bool legacy_process_hitting(Ray ray, out HitRecord use_record) {
    bool hit = false;
    float min_dist = infinity;
    //それぞれの球とレイの交点を計算、一番近いところを取る
    for(int i = 0; i < sphere_count; i++) {
        Sphere sphere = spheres[i];
        //レイ発射
        HitRecord hit_record;
        bool hit_i = false;
        hit_i = hit_sphere(sphere, ray, hit_record, 1e-3, infinity);//ray_dir=dt+origのt
        if(!hit_i)
            continue;
        hit = true;
        //一番近いレイの交点
        if(hit_record.ray_pram < min_dist) {
            use_record = hit_record;
            min_dist = hit_record.ray_pram;
        }
    }
    //それぞれの球とレイの交点を計算、一番近いところを取る
    for(int i = 0; i < quad_count; i++) {
        Quad quad = quads[i];
        //レイ発射
        HitRecord hit_record;
        bool hit_i = false;
        hit_i = hit_quad(quad, ray, hit_record, 1e-3, infinity);//ray_dir=dt+origのt
        if(!hit_i)
            continue;
        hit = true;
        //一番近いレイの交点
        if(hit_record.ray_pram < min_dist) {
            use_record = hit_record;
            min_dist = hit_record.ray_pram;
        }
    }
    return hit;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 散乱処理（マテリアルの挙動）
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Schlick近似
float reflectance(float cosine, float refraction_index) {
    // Use Schlick's approximation for reflectance.
    float r0 = (1 - refraction_index) / (1 + refraction_index);
    r0 = r0 * r0;
    return r0 + (1 - r0) * pow((1 - cosine), 5);
}
//マテリアルごとのpdf値の計算
float scattering_pdf(Ray ray, HitRecord hit_record, Ray scattered_ray) {
    float pdf_value;

    switch(materials[hit_record.material].material_type) {
        case MATERIAL_LAMBERTIAN: {
            pdf_value = 1.0 / (2.0 * PI);
        }
        default: {
            pdf_value = 0.0;
        }
    }
    return pdf_value;
}
//マテリアルによる散乱処理
bool scatter(int material, Ray ray, HitRecord hit_record, out vec3 attenuation, out Ray scattered, out float pdf_value, vec4 seed) {

    //今回使うマテリアル
    Material use_material = materials[material];
    vec3 scatter_dir;

    switch(use_material.material_type) {
        //Lambertian
        case MATERIAL_LAMBERTIAN: {
            vec3 albedo = use_material.albedo;
            //テクスチャを持っているならalbedoをテクスチャで上書き
            if(use_material.texture >= 0)
                albedo = sample_texture(use_material.texture, hit_record.uv);
            //ランバート分布による拡散反射
            scatter_dir = hit_record.normal + random_unit_vector(seed);
            //ランバート分布での反射だと、ゼロに近いベクトルが生まれることがある
            if(near_zero(scatter_dir)) {
                scatter_dir = hit_record.normal;
            }
            attenuation = albedo;
            break;
        }
        //Metal
        case MATERIAL_METAL: {
            vec3 albedo = use_material.albedo;
            //テクスチャを持っているならalbedoをテクスチャで上書き
            if(use_material.texture >= 0)
                albedo = sample_texture(use_material.texture, hit_record.uv);
            //鏡面反射
            scatter_dir = reflect(ray.direction, hit_record.normal);
            //Fuzzy Reflection
            scatter_dir = normalize(scatter_dir) + (use_material.fuzz * random_unit_vector(seed));
            attenuation = albedo;
            break;
        }
        //誘電体
        case MATERIAL_DIELECTRIC: {
            //相対屈折率 ri=n2/n1（n1の媒質からn2の媒質に入る時）
            //glslでは相対屈折率の逆数を取る
            float abs_ri = use_material.refraction_index;
            float ri = hit_record.front_face ? 1.0 / abs_ri : abs_ri;
            float cos_theta = min(dot(-ray.direction, hit_record.normal), 1.0);
            float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
            //全反射
            if(ri * sin_theta > 1.0 || reflectance(cos_theta, ri) > rand(seed)) {
                scatter_dir = reflect(ray.direction, hit_record.normal);
            }
            //透過可能
            else {
                scatter_dir = refract(ray.direction, hit_record.normal, ri);
            }
            attenuation = vec3(1.0);
            break;
        }
        default: {
            attenuation = ERROR_COLOR;
            scatter_dir = vec3(0.0, 1.0, 0.0);
            break;
        }
    }

    //新しいレイを作成
    scattered = Ray(hit_record.point, normalize(scatter_dir));
    //pdfの計算
    pdf_value = scattering_pdf(ray, hit_record, scattered);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// レイ反射処理
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 再帰関数が使えないのでスタック
#define STATE_CALLED 0
#define STATE_RETURN 1
#define STACK_MAX 32
int env_stack_count = 0;
struct Environment {
    int state;//状態 0:再起開始 1:再帰呼び出し終了
    Ray ray;           // 今のレイ
    vec3 accum_attenuation; // エネルギーの減衰
    vec3 emitted; //発光エネルギー
    float pdf_value;//全体的なサンプリング戦略
    float scattering_pdf;//散乱の仕方(BRDFに対するpdf)
    int depth;         // 再帰の深さ
};
Environment envs_stack[STACK_MAX]; //スタック
void push_env(Environment env) {
    envs_stack[env_stack_count] = env;
    env_stack_count++;
}
Environment pop_env() {
    return envs_stack[--env_stack_count];
}
vec3 launch_ray(Ray ray, int sample_number) {

    //スタック初期化
    env_stack_count = 0;

    //const int max_depth=10;
    //最初のレイを飛ばす
    push_env(Environment(STATE_CALLED, ray, vec3(1.0), vec3(0.0), 1.0, 1.0, max_depth));
    vec3 result = vec3(1.0);

    //スタックが空になるまで（8を超えたらそれは想定外の挙動ということで処理）
    while(0 < env_stack_count && env_stack_count <= STACK_MAX) {
        Environment env = pop_env();
        switch(env.state) {

            //再起開始
            case STATE_CALLED: {
            //深さが限界に達していたら終了
                if(env.depth <= 0) {
                    //result = vec3(0.0);
                    push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.emitted, env.pdf_value, env.scattering_pdf, env.depth));
                    break;
                }

                bool is_hit;
                HitRecord use_record;

                //BVHを使うか、全部走査するか
                if(USE_BVH)
                    is_hit = traverse_bvh(env.ray, use_record);
                else
                    is_hit = legacy_process_hitting(env.ray, use_record);

            //当たらなかった
                if(!is_hit) {
                //背景色
                    result = background_sky(env.ray.direction);
                    push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.emitted, env.pdf_value, env.scattering_pdf, env.depth));
                }
            //当たった場合
                else {
                    vec4 seed = vec4(TexCoord.xy * use_record.ray_pram * 10000.0, float(sample_number), float(env.depth));
                    //以下3つは計算させる
                    Ray new_ray;//次に発生するレイ
                    vec3 attenuation;//減衰
                    float pdf_value;//pdf値
                    bool is_scatterd;
                    is_scatterd = scatter(use_record.material, env.ray, use_record, attenuation, new_ray, pdf_value, seed);
                    vec3 emitted = materials[use_record.material].emitted;
                    //反射できなかった場合は、発光のエネルギーだけを返す
                    //TODO:いらない可能性高い。後で下の処理に吸収させるかも
                    if(!is_scatterd) {
                        push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.emitted, env.pdf_value, env.scattering_pdf, env.depth));
                    } 
                    //反射できた場合
                    else {
                        vec2 uv_offs = rand2(seed + 31.415926535);
                        vec3 on_light = vec3(-0.15, 0.49, 0.15) + vec3(0.3, 0.0, 0.0) * uv_offs.x + vec3(0.0, 0.0, -0.3) * uv_offs.y;
                        vec3 to_light = on_light - use_record.point;
                        float dist_squared = dot(to_light, to_light);
                        to_light = safe_normalize(to_light);
                        float light_area = 0.3 * 0.3;
                        float light_cosine = abs(to_light.y);
                        if(light_cosine < 0.000001) {
                            push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.emitted, env.pdf_value, env.scattering_pdf, env.depth));
                        }
                        //TODO: 上書きしてるので後で消す
                        pdf_value = dist_squared / (light_area * light_cosine);
                        new_ray = Ray(use_record.point, to_light);
                        float scattering_pdf = scattering_pdf(ray, use_record, new_ray);

                        vec3 new_accum_attenuation = env.accum_attenuation * attenuation;
                        push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.emitted, env.pdf_value, env.scattering_pdf, env.depth));
                        push_env(Environment(STATE_CALLED, new_ray, new_accum_attenuation, emitted, pdf_value, scattering_pdf, env.depth - 1));
                    }
                }
                break;
            }

            //再起終了
            case STATE_RETURN: {
                vec3 res_scattered = env.accum_attenuation * env.scattering_pdf * result / env.pdf_value;
                vec3 res_emitted = env.emitted;
                result = res_scattered + res_emitted;
                break;
            }

            default: {
                result = ERROR_COLOR;
                break;
            }
        }
    }

    //ガンマ補正をかける
    result = linear_to_gamma(result);

    // if(env_stack_count > STACK_MAX)
    //     result = ERROR_COLOR;

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//メイン（レイ作成・発射とサンプル平均処理）
void main() {

    //カメラ視野角（Viewportの高さ）
    float h = tan(vfov * PI / 180.0 / 2.0);

    // viewportのx方向y方向の単位ベクトルを作成、viewport_locを特定する
    vec3 lookat = vec3(0, 0, 0); //（viewport上の位置）
    vec3 z_unit = safe_normalize(camera_pos - lookat);
    vec3 y_unit = safe_normalize(cross(v_up, z_unit));
    vec3 x_unit = cross(z_unit, y_unit);

    //viewportの中心位置
    vec3 viewport_center = camera_pos - z_unit * focus_dist;

    //今回のピクセルに対応するviewportの座標（yを-1~1に合わせる）
    float viewport_x = (TexCoord.y * 4.0 - 1.0) * h * focus_dist;
    float viewport_y = (TexCoord.x * 4.0 - 1.0) * h * focus_dist * aspect_ratio;
    //vec3 viewport_loc = vec3(viewport_y, viewport_x, camera_pos.z - focus_dist);
    vec3 viewport_loc = viewport_center + viewport_x * x_unit + viewport_y * y_unit;

    //シード値
    float seed_x = (TexCoord.x + 0.5) * 65536.0;
    float seed_y = (TexCoord.y + 0.5) * 65536.0;

    //レイの目標地点（サンプル）
    vec2 aim_offs = rand2(vec4(seed_x, seed_y, float(ray_sample_number), 31.4159265358));
    aim_offs = (aim_offs * 2.0 - 1.0) * 0.001;
    vec3 aim_point = viewport_loc + vec3(aim_offs, 0.0);

    //レイの発射地点（ぼかし）
    float defous_radius = focus_dist * tan(radians(defous_angle / 2.0));
    vec2 launch_offs = random_in_unit_disk(vec4(seed_x, seed_y, float(ray_sample_number) + 2.0, 27.1828182845));
    vec3 launch_point = camera_pos + launch_offs.x * y_unit * defous_radius + launch_offs.y * x_unit * defous_radius;

    //レイの作成
    vec3 ray_dir = safe_normalize(aim_point - launch_point);
    Ray ray = Ray(launch_point, ray_dir);

    //レイの発射
    vec3 sample_color = launch_ray(ray, ray_sample_number);
    // glBlendFunc(GL_ONE, GL_ONE)で累積加算されるため、そのまま出力する
    color = sample_color;

}
