#include "heart.hpp"
#include "../ecs/entity.hpp"
#include "../deserialize-utils.hpp"

namespace our {
    // Reads id from the given json object
    void HeartComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        id = data.value("id", id);
    }
}