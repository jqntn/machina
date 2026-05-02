#pragma once

#include "machina/LevelDescription.hpp"

#include <entt/entt.hpp>

namespace machina {

class LevelInstantiator
{
public:
  void instantiate(entt::registry& registry,
                   const LevelDescription& level) const;
};

}
