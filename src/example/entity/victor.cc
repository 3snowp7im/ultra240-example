#include <ultra240/ultra.h>

namespace example {

  class Victor : public ultra::Entity {
  public:

    Victor(
      const ultra::World::Boundaries& boundaries,
      const ultra::Tileset& tileset,
      const ultra::geometry::Vector<float>& position,
      ultra::Entity::Attributes attributes
    ) : Entity(
      ultra::hash("collision"_h),
      boundaries,
      tileset,
      position,
      attributes
    ) {}

  };

}

extern "C" ultra::Entity* create_entity(
  const ultra::World::Boundaries& boundaries,
  const ultra::Tileset& tileset,
  const ultra::geometry::Vector<float>& position,
  ultra::Entity::Attributes attributes
) {
  return new example::Victor(boundaries, tileset, position, attributes);
}
