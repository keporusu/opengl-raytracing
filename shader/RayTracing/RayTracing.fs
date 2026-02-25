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
#define SPHERE_MAX 50
#define PLANE_MAX 50
#define MATERIAL_MAX 10
#define ERROR_COLOR vec3(1.0,0.0,1.0)

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
layout(std140) uniform PrimitivesBlock {
    int sphere_count;
    Sphere spheres[SPHERE_MAX];
    // int plane_count;
    // Plane planes[PLANE_MAX];
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

                bool no_hit = true;
                float min_dist = infinity;
                HitRecord use_record;
            //それぞれのプリミティブとレイの交点を計算、一番近いところを取る
                for(int i = 0; i < sphere_count; i++) {
                    Sphere sphere = spheres[i];

                //レイ発射
                    HitRecord hitRecord;
                    bool hit = hit_sphere(sphere, env.ray, hitRecord, 1e-3, infinity);//ray_dir=dt+origのt

                    if(!hit)
                        continue;

                    no_hit = false;
                //一番近いレイの交点
                    if(hitRecord.ray_pram < min_dist) {
                        use_record = hitRecord;
                        min_dist = hitRecord.ray_pram;
                    }
                }

            //当たらなかった
                if(no_hit) {
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

    //TODO: viewportのx方向y方向の単位ベクトルを作って、viewport_locを特定する
    //座標系の作成（yを-1~1に合わせる）
    float viewport_x = (TexCoord.y * 4.0 - 1.0) * h * focus_dist;
    float viewport_y = (TexCoord.x * 4.0 - 1.0) * h * focus_dist * aspect_ratio;
    vec3 viewport_loc = vec3(viewport_y, viewport_x, camera_pos.z - focus_dist);

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
    launch_offs *= defous_radius;
    vec3 launch_point = camera_pos + vec3(launch_offs, 0.0);

    //レイの作成
    vec3 ray_dir = normalize(aim_point - launch_point);
    Ray ray = Ray(launch_point, ray_dir);

    //レイの発射
    vec3 sample_color = launch_ray(ray, ray_sample_number);
    // glBlendFunc(GL_ONE, GL_ONE)で累積加算されるため、そのまま出力する
    color = sample_color;

    // float samples_per_pixel = 50;
    // vec3 color_accum = vec3(0.0);
    // for(int i = 0; i < samples_per_pixel; i++) {
    //     //目標発射地点
    //     vec2 offs = rand2(vec4(viewport_x * 1000.0, viewport_y * 1000.0, float(i), 31.4159265358));
    //     offs = (offs * 2.0 - 1.0) * 0.001;
    //     vec3 launch_point = viewport_loc + vec3(offs, 0.0);
    //     //レイの作成
    //     vec3 ray_dir = normalize(launch_point - camera_pos);
    //     Ray ray = Ray(camera_pos, ray_dir);
    //     //レイの発射
    //     color_accum += launch_ray(ray, i);
    // }

    // color = color_accum / float(samples_per_pixel);

}
