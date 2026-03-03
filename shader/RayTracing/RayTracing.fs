#version 330 core
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// utility
////////////////////////////////////////////////////////////////////////////////////////////////////////////
const float infinity = 3.402823e+38;
const float PI = 3.14159265359;
const vec3 v_up = vec3(0.0, 1.0, 0.0);

vec3 linear_to_gamma(vec3 c) {
    return sqrt(max(c, 0.0));
}
bool near_zero(vec3 v) {
    const float e = 1e-8;
    return abs(v.x) < e && abs(v.y) < e && abs(v.z) < e;
}
void swap(inout float a, inout float b) {
    float v = a;
    a = b;
    b = v;
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
#define MAX_SPHERES 60
#define MAX_PLANES 60
#define MATERIAL_MAX 60
#define ERROR_COLOR vec3(1.0,0.0,1.0)
#define MAX_BVH_NODES 500
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
};
//AABB
struct AlignedBox {
    float x_min, x_max;
    float y_min, y_max;
    float z_min, z_max;
};
struct BVHNode {
    AlignedBox aabb;
    int left;
    int right;
    int prim_index;
    //int pad0, pad1, pad2;
};

// Primitives
struct Sphere {
    vec3 center;
    float radius;
    int material;
};
// Materials
#define MATERIAL_LAMBERTIAN 1
#define MATERIAL_METAL 2
#define MATERIAL_DIELECTRIC 3
struct Material {
    int material_type;
    vec3 albedo;
    float fuzz; //金属限定 ぼやかし
    float refraction_index; //誘電体限定 相対屈折率　
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
    // int plane_count;
    // Plane planes[MAX_PLANES];
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// プリミティブとの交点計算
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool hit_sphere(Sphere sphere, Ray ray, out HitRecord hitRecord, float ray_tmin, float ray_tmax) {
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
    hitRecord.ray_pram = root;//解
    hitRecord.point = root * ray.direction + ray.origin;//ヒット位置
    vec3 outward_normal = (hitRecord.point - sphere.center) / sphere.radius;
    hitRecord.front_face = dot(ray.direction, outward_normal) < 0; //当たったのは表面か？
    hitRecord.normal = hitRecord.front_face ? outward_normal : -outward_normal;//法線の向き
    hitRecord.material = sphere.material;

    //交わった
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// プリミティブとのヒット処理関連
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//aabbとレイが交差するか？
bool hit_aabb(AlignedBox aabb, Ray ray) {
    float ray_t_min = infinity;
    float ray_t_max = 1e-3;
    //TODO: いずれAlignedBoxの形を変えたほうが良さそう
    vec3 aabb_mins = vec3(aabb.x_min, aabb.y_min, aabb.z_min);
    vec3 aabb_maxs = vec3(aabb.x_max, aabb.y_max, aabb.z_max);

    //それぞれの軸で考える
    for(int axis = 0; axis < 3; axis++) {
        float t0 = (aabb_mins[axis] - ray.origin[axis]) / ray.direction[axis];
        float t1 = (aabb_maxs[axis] - ray.origin[axis]) / ray.direction[axis];
        if(t0 > t1)
            swap(t0, t1);
        ray_t_min = min(ray_t_min, t0);
        ray_t_max = max(ray_t_max, t1);
        if(ray_t_max <= ray_t_min) {
            return false;
        }
    }
    return true;
}
//BVHによるヒット処理
bool traverse_bvh(Ray ray, out HitRecord hit_record) {
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

        if(!hit_aabb(node.aabb, ray)) {
            continue;
        }
        //葉に到達した場合
        if(node.prim_index >= 0) {
            bool hit = hit_sphere(spheres[node.prim_index], ray, hit_record, 1e-3, infinity);
            //ヒットしてたら、最も近いプリミティブを更新する
            if(hit && hit_record.ray_pram < min_dist) {
                min_dist = hit_record.ray_pram;
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
bool legacy_process_hitting(Ray ray, out HitRecord hit_record) {
    bool hit = false;
    float min_dist = infinity;
    //それぞれのプリミティブとレイの交点を計算、一番近いところを取る
    for(int i = 0; i < sphere_count; i++) {
        Sphere sphere = spheres[i];

        //レイ発射
        HitRecord hit_record_i;
        bool hit_i = hit_sphere(sphere, ray, hit_record_i, 1e-3, infinity);//ray_dir=dt+origのt

        if(!hit_i)
            continue;

        hit = true;
        //一番近いレイの交点
        if(hit_record_i.ray_pram < min_dist) {
            hit_record = hit_record_i;
            min_dist = hit_record_i.ray_pram;
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
//マテリアルによる散乱処理
bool scatter(int material, Ray ray, HitRecord hitRecord, out vec3 attenuation, out Ray scattered, vec4 seed) {

    //今回使うマテリアル
    Material use_material = materials[material];
    vec3 scatter_dir;

    switch(use_material.material_type) {
        //Lambertian
        case MATERIAL_LAMBERTIAN: {
            vec3 albedo = use_material.albedo;
            //ランバート分布による拡散反射
            scatter_dir = hitRecord.normal + random_unit_vector(seed);
            //ランバート分布での反射だと、ゼロに近いベクトルが生まれることがある
            if(near_zero(scatter_dir)) {
                scatter_dir = hitRecord.normal;
            }
            attenuation = albedo;
            break;
        }
        //Metal
        case MATERIAL_METAL: {
            vec3 albedo = use_material.albedo;
            //鏡面反射
            scatter_dir = reflect(ray.direction, hitRecord.normal);
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
            float ri = hitRecord.front_face ? 1.0 / abs_ri : abs_ri;
            float cos_theta = min(dot(-ray.direction, hitRecord.normal), 1.0);
            float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
            //全反射
            if(ri * sin_theta > 1.0 || reflectance(cos_theta, ri) > rand(seed)) {
                scatter_dir = reflect(ray.direction, hitRecord.normal);
            }
            //透過可能
            else {
                scatter_dir = refract(ray.direction, hitRecord.normal, ri);
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
    scattered = Ray(hitRecord.point, normalize(scatter_dir));

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
    push_env(Environment(STATE_CALLED, ray, vec3(1.0), max_depth));
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
                    push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.depth));
                    break;
                }

                bool is_hit;
                HitRecord use_record;

                is_hit = traverse_bvh(env.ray,use_record);
                //is_hit = legacy_process_hitting(env.ray, use_record);

            //当たらなかった
                if(!is_hit) {
                //背景色
                    float a = 0.5 * (env.ray.direction.y + 1.0);
                    result = (1.0 - a) * vec3(1.0, 1.0, 1.0) + a * vec3(0.5, 0.7, 1.0);
                    push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.depth));
                }
            //当たった場合
                else {
                    vec4 seed = vec4(TexCoord.xy * use_record.ray_pram * 10000.0, float(sample_number), float(env.depth));
                    Ray new_ray;
                    vec3 attenuation;
                    scatter(use_record.material, env.ray, use_record, attenuation, new_ray, seed);
                    vec3 new_accum_attenuation = env.accum_attenuation * attenuation;
                    push_env(Environment(STATE_RETURN, env.ray, env.accum_attenuation, env.depth));
                    push_env(Environment(STATE_CALLED, new_ray, new_accum_attenuation, env.depth - 1));
                }
                break;
            }

            //再起終了
            case STATE_RETURN: {
                result = result * env.accum_attenuation;
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
    vec3 z_unit = normalize(camera_pos - lookat);
    vec3 y_unit = normalize(cross(v_up, z_unit));
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
    vec3 ray_dir = normalize(aim_point - launch_point);
    Ray ray = Ray(launch_point, ray_dir);

    //レイの発射
    vec3 sample_color = launch_ray(ray, ray_sample_number);
    // glBlendFunc(GL_ONE, GL_ONE)で累積加算されるため、そのまま出力する
    color = sample_color;

}
