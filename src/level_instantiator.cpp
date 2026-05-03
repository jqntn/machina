#include <machina/level_instantiator.hpp>

#include <machina/ecs.hpp>

namespace machina {

void
LevelInstantiator::instantiate(entt::registry& registry,
                               const LevelDescription& level) const
{
  for (const auto& description : level.entities) {
    const entt::entity entity = registry.create();
    registry.emplace<Name>(entity, description.name);
    registry.emplace<Transform>(entity, description.transform);
    registry.emplace<Renderable>(
      entity, description.mesh, description.material);
  }
}

}
