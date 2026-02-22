#include "../Primitives/Primitives.hpp"
#include <memory>

class Scene{

    public:
    Scene();

    UBO_Primitives* GetPrimitivesUBO() {return &primitives_ubo;}

    private:
    UBO_Primitives primitives_ubo;

    int sphereCount=0;
    void addPrimitive(Sphere sphere);
};

