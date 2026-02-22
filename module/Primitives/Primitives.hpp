#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"
#define MAX_SPHERES 50
#define MAX_PLANES 50

struct Sphere{
    glm::vec3 center;
    float radius;
    int material; //この時点で16byteを超えるため、次は32byteで合わせる
    float _padding[3]; // std140 layout padding (12 bytes)
};

struct UBO_Primitives{
    int sphere_count;
    float _padding0[3];//12byte
    Sphere spheres[MAX_SPHERES];
};