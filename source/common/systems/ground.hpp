#pragma once
#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

namespace our
{

    class GroundSystem {
        glm::vec4 positionCamera;
        int width = 20;
        int height = 40;

    public:

        // This should be called every frame to update all entities containing a StreetComponent.
        void update(World* world,int& score) {
            // For each entity in the world
            for(auto entity : world->getEntities()){
                // Get the camera component if it exists
                CameraComponent* camera = entity->getComponent<CameraComponent>();
                // If the street component exists
                if (camera)
                {
                    positionCamera = camera->getPosition();
                }
            }
            for(auto entity : world->getEntities()){
                // Get name of entity
                std::string name = entity->name;
                // If the street component exists
                if((name=="street"||name=="grass")  &&  (entity->localTransform.position[2]-width) >positionCamera[2] ){
                    score++; // inc score
                    for(auto entity2 : world->getEntities()){
                        if((entity2->name=="bus"|| entity2->name=="taxi" )&& (entity2->localTransform.position[2]) > ( entity->localTransform.position[2] - width/2 ) ){
                            // change taxi or bus position
                            entity2->localTransform.position[2] -= height * 4;
                        }

                    }
                    // change grass or street position
                    entity->localTransform.position[2] -= height * 4;
                }
            }
        }

    };

}
