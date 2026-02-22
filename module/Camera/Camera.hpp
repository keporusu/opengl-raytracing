#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct UBO_Camera
{
    glm::vec3 position;
    float aspect_ratio;
    int max_depth;
};

class Camera
{
public:
    Camera(glm::vec3 position, float aspectRatio, int max_depth = 10)
    {
        this->cameraUBO.position = position;
        this->cameraUBO.aspect_ratio = aspectRatio;
        this->cameraUBO.max_depth = max_depth;
    }

    void Move(glm::vec3 movement)
    {
        cameraUBO.position += movement;
    }

    void SetAspectRatio(float ratio)
    {
        cameraUBO.aspect_ratio = ratio;
    }

    UBO_Camera *GetUBO()
    {
        return &cameraUBO;
    }

private:
    UBO_Camera cameraUBO;
};