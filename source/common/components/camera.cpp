#include "camera.hpp"
#include "../ecs/entity.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
using namespace std;

namespace our
{
    // Reads camera parameters from the given json object
    void CameraComponent::deserialize(const nlohmann::json &data)
    {
        if (!data.is_object())
            return;
        std::string cameraTypeStr = data.value("cameraType", "perspective");
        if (cameraTypeStr == "orthographic")
        {
            cameraType = CameraType::ORTHOGRAPHIC;
        }
        else
        {
            cameraType = CameraType::PERSPECTIVE;
        }
        near = data.value("near", 0.01f);
        far = data.value("far", 100.0f);
        fovY = data.value("fovY", 90.0f) * (glm::pi<float>() / 180);
        orthoHeight = data.value("orthoHeight", 1.0f);
    }

    // Creates and returns the camera view matrix
    glm::mat4 CameraComponent::getViewMatrix() const
    {
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix(); // Transform Camera from local to world using its getLocalToWorldMatrix since camera is a component
        // TODO: (Req 8) Complete this function
        // HINT:
        //  In the camera space:
        //  - eye is the origin (0,0,0)
        //  - center is any point on the line of sight. So center can be any point (0,0,z) where z < 0. For simplicity, we let center be (0,0,-1)
        //  - up is the direction (0,1,0)
        //  but to use glm::lookAt, we need eye, center and up in the world state.
        //  Since M (see above) transforms from the camera to thw world space, you can use M to compute:
        //  - the eye position which is the point (0,0,0) but after being transformed by M
        //  - the center position which is the point (0,0,-1) but after being transformed by M
        //  - the up direction which is the vector (0,1,0) but after being transformed by M
        //  then you can use glm::lookAt

        glm::vec4 eye = M * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Vec4*4 *4*1=4*1 Postion of the Camera
        glm::vec4 center = M * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
        glm::vec4 up = M * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        // Syntax:glm::mat4 view_matrix = glm::lookAt(eye, center, up);
        // Multiplying this mat view_matrix with any object in the world space gets it to Eye space where the camera is the Origin
        glm::mat4 view_matrix = glm::lookAt(glm::vec3(eye), glm::vec3(center), glm::vec3(up));
        return view_matrix;
    }

    // Creates and returns the camera projection matrix
    // "viewportSize" is used to compute the aspect ratio
    glm::mat4 CameraComponent::getProjectionMatrix(glm::ivec2 viewportSize) const
    {
        // TODO: (Req 8) Write this function
        // Compute Aspect Ratio
        float aspect_ratio = viewportSize[0] / float(viewportSize[1]); // w/h

        glm::mat4 projection_matrix; // projection _matrix diifers according to the type of the Camera either orthographic or perspective

        if (this->cameraType == CameraType::ORTHOGRAPHIC)
        {
            // NOTE: The function glm::ortho can be used to create the orthographic projection matrix
            // It takes left, right, bottom, top. Bottom is -orthoHeight/2 and Top is orthoHeight/2.
            // Left and Right are the same but after being multiplied by the aspect ratio
            float bottom = -1 * (orthoHeight / 2);
            float top = (orthoHeight / 2);
            float left = bottom * aspect_ratio;
            float right = top * aspect_ratio;
            // Syntax:glm::mat4 projection_matrix = glm::ortho(left, right, bottom, top, near, far) Defines the cuboid view volume :D
            projection_matrix = glm::ortho(left, right, bottom, top, near, far);
        }
        else if (this->cameraType == CameraType::PERSPECTIVE)
        {
            // For the perspective camera, you can use glm::perspective
            // glm::mat4 projection_matrix = glm::perspective(I(Field of View), aspect_ratio(w/h), near, far);
            // The camera can’t see anything nearer than “near” and farther than “far”
            projection_matrix = glm::perspective(fovY, aspect_ratio, near, far);
        }
        return projection_matrix;
        // return glm::mat4(1.0f);
    }

    // get Postion of camera
    glm::vec4 CameraComponent::getPosition() const
    {
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix(); // Transform Camera from local to world using its getLocalToWorldMatrix since camera is a component
        glm::vec4 position = M * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f); // Vec4*4 *4*1=4*1 Postion of the Camera
        return position;
    }
}