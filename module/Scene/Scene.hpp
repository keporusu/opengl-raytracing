#include "../Primitives/Primitives.hpp"
#include <memory>
#include <vector>

class Scene
{

public:
    Scene();

    UBO_Primitives *GetPrimitivesUBO() { return &primitives_ubo; }
    UBO_Materials *GetMateialsUBO() { return &materials_ubo; }

private:
    std::vector<Sphere> spheres;

    UBO_Primitives primitives_ubo;
    UBO_Materials materials_ubo;

    int sphereCount = 0;
    int materialCount = 0;
    void addPrimitive(Sphere sphere);
    void createMaterialMap();

    //シーン
    void three_balls();
    void many_balls();
};
