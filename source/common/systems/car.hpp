#pragma once

#include "../ecs/world.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <unordered_set>

namespace our
{

    // The Car System is responsible for Appearing and Disappearing of cars
    // This system is added as a simple example for how use the ECS framework to implement logic.
    class CarsSystem
    {
        int length = 80; // length of max postion which can reach after which we rerender the car from the start postion again (as if a new car appears <3)
        int start = 50; // start postion which rerender it again :D

    public:
        // This should be called every frame to update all entities containing a MovementComponent.
        void update(World *world)
        {
            for (auto entity : world->getEntities())
            {
                //If this entity is a Car or a Taxi
                if((entity->name=="car"|| entity->name=="taxi")){
                    // rerender cars after reach max border of street
                    if(entity->localTransform.position[0] >length)
                    {
                        // render to left again
                        entity->localTransform.position[0] = -1*start;
                    }
                    else if(entity->localTransform.position[0] < -length)
                    {
                        // render to right again
                        entity->localTransform.position[0] = start;
                    }
                }
            }

        }
    };

}
