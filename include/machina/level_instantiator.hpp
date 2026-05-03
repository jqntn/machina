#pragma once

#include <machina/level_description.hpp>

#include <entt/entt.hpp>

namespace machina {

class LevelInstantiator
{
public:
  void instantiate(entt::registry& registry,
                   const LevelDescription& level) const;
};

}
