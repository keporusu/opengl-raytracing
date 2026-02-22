#include "../../third_party/glm/glm.hpp"
#include "../../third_party/glm/gtc/matrix_transform.hpp"

struct UBO_Camera{
    glm::vec3 position;
    float aspect_ratio;
};

class Camera{
    public:
    Camera(glm::vec3 position, float aspectRatio){
        this->position=position;

        this->cameraUBO.position=position;
        this->cameraUBO.aspect_ratio=aspectRatio;
    }

    void Move(glm::vec3 movement){
        position+=movement;
        cameraUBO.position=position;
    }

    UBO_Camera* GetUBO(){
        return &cameraUBO;
    }

    private:
    glm::vec3 position;
    UBO_Camera cameraUBO;

};