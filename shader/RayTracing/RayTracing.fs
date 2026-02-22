#version 330 core

////
//const
////
const float infinity = 3.402823e+38;
#define SPHERE_MAX 50
#define PLANE_MAX 50
#define ERROR_COLOR vec3(1.0,0.0,1.0)

//////////////////////////////////////////////////////
// Ray Data
//////////////////////////////////////////////////////
struct Ray {
    vec3 origin;
    vec3 direction;
};
struct HitRecord {
    vec3 point;
    vec3 normal;
    float ray_pram;
    bool front_face;
    int material;
};

//////////////////////////////////////////////////////
//Primitives
//////////////////////////////////////////////////////
struct Sphere {
    vec3 center;
    float radius;
    int material;
};

//////////////////////////////////////////////////////
//in-out
//////////////////////////////////////////////////////
layout(location = 0) out vec3 color;
in vec2 TexCoord;

//////////////////////////////////////////////////////
// uniform
//////////////////////////////////////////////////////
uniform float u_frame;
const float focal_length = 1.0;
layout(std140) uniform PrimitivesBlock {
    int sphere_count;
    Sphere spheres[SPHERE_MAX];
    // int plane_count;
    // Plane planes[PLANE_MAX];
};
layout(std140) uniform CameraBlock {
    vec3 camera_pos;
    float aspect_ratio;
    int max_depth;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 球
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
//乱数
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
float esgtsa(vec4 v) {
    return float(esgtsa_orig(esgtsa_orig(esgtsa_orig(esgtsa_orig(uint(v.x)) + uint(v.y)) + uint(v.z)) + uint(v.w))) * 2.3283064365386962890625e-10;
}
//2次元0~1
vec2 esgtsa2(vec4 v) {
    float r1 = esgtsa(v);
    float r2 = esgtsa(vec4(v.x, v.y, v.z, v.w + 1.0));
    return vec2(r1, r2);
}
//3次元0~1
vec3 esgtsa3(vec4 v) {
    float r1 = esgtsa(v);
    float r2 = esgtsa(vec4(v.x, v.y, v.z, v.w + 1.0));
    float r3 = esgtsa(vec4(v.x, v.y, v.z, v.w + 2.0));
    return vec3(r1, r2, r3);
}
//３次元ベクトル-1~1
vec3 random_unit_vector(vec4 v) {
    for(int i = 0; i < 100; i++) {
        vec3 p = esgtsa3(vec4(v.x, v.y, v.z, v.w + float(i) * 3.0)) * 2.0 - 1.0;
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//レイ発射処理
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 再帰関数が使えないのでスタック
#define STATE_CALLED 0
#define STATE_RETURN 1
#define STACK_MAX 32
int env_stack_count = 0;
struct Environment {
    int state;//状態 0:再起開始 1:再帰呼び出し終了
    Ray ray;           // 今のレイ
    vec3 attenuation; // エネルギーの減衰
    int depth;         // 再帰の深さ
};
Environment envs_stack[STACK_MAX];
void push_env(Environment env) {
    envs_stack[env_stack_count] = env;
    env_stack_count++;
}
Environment pop_env() {
    return envs_stack[--env_stack_count];
}
vec3 launch_ray(Ray ray) {

    //スタック初期化
    env_stack_count = 0;

    //const int max_depth=10;
    //最初のレイを飛ばす
    push_env(Environment(STATE_CALLED, ray, vec3(1.0), max_depth));
    vec3 result = ERROR_COLOR;

    //スタックが空になるまで（8を超えたらそれは想定外の挙動ということで処理）
    while(0 < env_stack_count && env_stack_count <= STACK_MAX) {
        Environment env = pop_env();
        switch(env.state) {

            //再起開始
            case STATE_CALLED: {
            //深さが限界に達していたら終了
                if(env.depth <= 0) {
                    result = vec3(0.0);
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
                    push_env(Environment(STATE_RETURN, env.ray, env.attenuation, env.depth));
                }
            //当たった場合
                else {
                    vec4 seed = vec4(vec3(TexCoord.xy, use_record.ray_pram) * 1000.0, float(env.depth));
                    vec3 scatter_dir = use_record.normal + random_unit_vector(seed);//ランバート分布
                    Ray new_ray = Ray(use_record.point, scatter_dir);
                    vec3 new_attenuation = env.attenuation * 0.5;
                    push_env(Environment(STATE_RETURN, env.ray, env.attenuation, env.depth));
                    push_env(Environment(STATE_CALLED, new_ray, new_attenuation, env.depth - 1));
                }
                break;
            }

            //再起終了
            case STATE_RETURN: {
                result = result * env.attenuation;
                break;
            }

            default: {
                result = ERROR_COLOR;
                break;
            }
        }
    }

    //ガンマ補正をかける
    result = sqrt(max(result, 0.0));

    if(env_stack_count > STACK_MAX)
        result = ERROR_COLOR;

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////

void main() {

    //座標系の作成
    float viewport_x = (TexCoord.x * 4.0 - 1.0) * aspect_ratio;
    float viewport_y = TexCoord.y * 4.0 - 1.0;
    vec3 viewport_loc = vec3(viewport_x, viewport_y, camera_pos.z - focal_length);

    float samples_per_pixel = 20;
    vec3 color_accum = vec3(0.0);
    for(int i = 0; i < samples_per_pixel; i++) {
        //目標発射地点
        vec2 offs = esgtsa2(vec4(viewport_x * 100.0, viewport_y * 100.0, float(i), 31.4159265358));
        offs = (offs * 2.0 - 1.0) * 0.001;
        vec3 launch_point = viewport_loc + vec3(offs, 0.0);
        //レイの作成
        vec3 ray_dir = normalize(launch_point - camera_pos);
        Ray ray = Ray(camera_pos, ray_dir);
        //レイの発射
        color_accum += launch_ray(ray);
    }

    color = color_accum / float(samples_per_pixel);

}
