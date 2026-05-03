#pragma once

#include <entt/entity/fwd.hpp>
#include <machina/level_description.hpp>

namespace machina {

class LevelInstantiator
{
public:
  void instantiate(entt::registry& registry,
                   const LevelDescription& level) const;
};

}
