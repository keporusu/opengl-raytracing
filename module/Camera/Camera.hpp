#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct UBO_Camera
{
    glm::vec3 position;
    float aspect_ratio;
    float vfov;
    int max_depth;
};

class Camera
{
public:
    Camera(glm::vec3 position, float aspectRatio, float vfov = 90.0, int max_depth = 10)
    {
        this->cameraUBO.position = position;
        this->cameraUBO.aspect_ratio = aspectRatio;
        this->cameraUBO.max_depth = max_depth;
        this->cameraUBO.vfov = vfov;
    }

    void Move(glm::vec3 movement)
    {
        cameraUBO.position += movement;
        is_changed = true;
    }

    void Zoom(float movement)
    {
        float newFOV = cameraUBO.vfov + movement;
        if (newFOV > 120.0f)
        {
            newFOV = 120.0f;
        }
        if (newFOV < 30.0f)
        {
            newFOV = 30.0f;
        }
        cameraUBO.vfov = newFOV;
        is_changed = true;
    }

    void SetAspectRatio(float ratio)
    {
        cameraUBO.aspect_ratio = ratio;
    }
    void ChangeFlagOff()
    {
        is_changed = false;
    }
    bool IsChanged() const
    {
        return is_changed;
    }

    UBO_Camera *GetUBO()
    {
        return &cameraUBO;
    }

private:
    UBO_Camera cameraUBO;
    bool is_changed = false;
};