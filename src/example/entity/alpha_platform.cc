#include <array>
#include <ultra240/util.h>
#include "example/example.h"

namespace example::alpha {

  class Platform : public SingleSpriteEntity {

    ultra::geometry::Vector<float> speed;

    ultra::geometry::Vector<std::array<uint8_t, 2>> range;

    ultra::geometry::Vector<float> start_pos;

  public:

    Platform(
      const ultra::Tileset& tileset,
      uint16_t tile_index,
      const ultra::renderer::TilesetHandle* handle,
      const ultra::geometry::Vector<float>& position,
      const ultra::Tileset::Attributes& attributes,
      uint32_t state
    ) : start_pos(position),
        speed({
          ultra::util::read_signed_bits<float>(state, 4)
          / ultra::util::read_signed_bits<float>(state, 4),
          ultra::util::read_signed_bits<float>(state, 4)
          / ultra::util::read_signed_bits<float>(state, 4)
        }),
        range({{
          ultra::util::read_unsigned_bits<uint8_t>(state, 4),
          ultra::util::read_unsigned_bits<uint8_t>(state, 4)
        }, {
          ultra::util::read_unsigned_bits<uint8_t>(state, 4),
          ultra::util::read_unsigned_bits<uint8_t>(state, 4)
        }}),
        SingleSpriteEntity(
          tileset,
          tile_index,
          handle,
          position,
          attributes
        ) {}

    void update(UpdateContext context) {
      sprite.position += speed;
      if (sprite.position.x < start_pos.x - range.x[0] * 16) {
        sprite.position.x = 2 * (start_pos.x - range.x[0] * 16)
          - sprite.position.x;
        speed.x = -speed.x;
      } else if (sprite.position.x > start_pos.x + range.x[1] * 16) {
        sprite.position.x = 2 * (start_pos.x + range.x[1] * 16)
          - sprite.position.x;
        speed.x = -speed.x;
      }
      if (sprite.position.y < start_pos.y - range.y[0] * 16) {
        sprite.position.y = 2 * (start_pos.y - range.y[0] * 16)
          - sprite.position.y;
        speed.y = -speed.y;
      } else if (sprite.position.y > start_pos.y + range.y[1] * 16) {
        sprite.position.y = 2 * (start_pos.y + range.y[1] * 16)
          - sprite.position.y;
        speed.y = -speed.y;
      }
    }

  };

}

extern "C" example::SingleSpriteEntity* create_entity(
  const ultra::Tileset& tileset,
  const ultra::renderer::TilesetHandle* handle,
  const ultra::geometry::Vector<float>& position,
  ultra::Tileset::Attributes attributes,
  uint16_t tile_index,
  uint16_t type,
  uint16_t id,
  uint32_t state,
  const ultra::World::Boundaries& boundaries
) {
  return new example::alpha::Platform(
    tileset,
    tile_index,
    handle,
    position,
    attributes,
    state
  );
}
