#include "example/example.h"

namespace example {

  class Victor : public SingleSpriteEntity {
  public:

    Victor(
      const ultra::Tileset& tileset,
      const ultra::renderer::TilesetHandle* handle,
      const ultra::geometry::Vector<float>& position,
      ultra::Tileset::Attributes attributes
    ) : SingleSpriteEntity(
          tileset,
          0,
          handle,
          position,
          attributes
        ) {}

    void update(SingleSpriteEntity::UpdateContext context) {}

  };

}

extern "C" example::SingleSpriteEntity* create_entity(
  const ultra::Tileset& tileset,
  const ultra::renderer::TilesetHandle* handle,
  const ultra::geometry::Vector<float>& position,
  ultra::Tileset::Attributes attributes
) {
  return new example::Victor(tileset, handle, position, attributes);
}
