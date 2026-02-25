#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct UBO_Camera
{
    glm::vec3 position;
    float aspect_ratio;
    float vfov;
    int max_depth;
    float defocus_angle;
    float focus_dist;
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
        this->cameraUBO.defocus_angle = 1.0f;
        this->cameraUBO.focus_dist = 1.0f;

        InitialPosition = position;
        InitialVfov = vfov;
        InitialFocusDist = 1.0f;
        InitialDefocusAngle = 1.0f;
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

    void SetDefocusAngle(float defocus_angle)
    {
        cameraUBO.defocus_angle = defocus_angle;
        is_changed = true;
    }

    void SetFocusDist(float focus_dist)
    {
        cameraUBO.focus_dist = focus_dist;
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

    void Reset()
    {
        cameraUBO.position = InitialPosition;
        cameraUBO.vfov = InitialVfov;
        cameraUBO.focus_dist = InitialFocusDist;
        cameraUBO.defocus_angle = InitialDefocusAngle;
        is_changed = true;
    }

    UBO_Camera *GetUBO()
    {
        return &cameraUBO;
    }

    float GetFocusDist() const { return cameraUBO.focus_dist; }
    float GetDefocusAngle() const { return cameraUBO.defocus_angle; }
    glm::vec3 GetPosition() const { return cameraUBO.position; }
    float GetVfov() const { return cameraUBO.vfov; }

private:
    UBO_Camera cameraUBO;
    bool is_changed = false;

    glm::vec3 InitialPosition;
    float InitialVfov;
    float InitialFocusDist;
    float InitialDefocusAngle;
};