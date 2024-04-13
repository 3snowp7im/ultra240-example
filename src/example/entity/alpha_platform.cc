#include <array>
#include <ultra240/ultra.h>

using namespace ultra::util;

namespace example::alpha {

  class Platform : public ultra::Entity {

    ultra::geometry::Vector<float> speed;

    ultra::geometry::Vector<std::array<uint8_t, 2>> range;

    ultra::geometry::Vector<float> start_pos;

  public:

    Platform(
      const ultra::World::Boundaries& boundaries,
      const ultra::Tileset& tileset,
      const ultra::geometry::Vector<float>& position,
      ultra::Entity::Attributes attributes,
      uint16_t tile_index,
      uint32_t state
    ) : Entity(
      ultra::hash("collision"_h),
      boundaries,
      tileset,
      position,
      attributes,
      tile_index
    ),
    start_pos(position),
    speed({
      read_signed_bits<float>(state, 4) / read_signed_bits<float>(state, 4),
      read_signed_bits<float>(state, 4) / read_signed_bits<float>(state, 4)
    }),
    range({{
      read_unsigned_bits<uint8_t>(state, 4),
      read_unsigned_bits<uint8_t>(state, 4)
    }, {
      read_unsigned_bits<uint8_t>(state, 4),
      read_unsigned_bits<uint8_t>(state, 4)
    }}) {}

    void update(
      ultra::World::Boundaries& boundaries,
      ultra::Entity* player,
      const std::vector<ultra::Entity*>& entities
    ) {
      position += speed;
      if (position.x < start_pos.x - range.x[0] * 16) {
        position.x = 2 * (start_pos.x - range.x[0] * 16) - position.x;
        speed.x = -speed.x;
      } else if (position.x > start_pos.x + range.x[1] * 16) {
        position.x = 2 * (start_pos.x + range.x[1] * 16) - position.x;
        speed.x = -speed.x;
      }
      if (position.y < start_pos.y - range.y[0] * 16) {
        position.y = 2 * (start_pos.y - range.y[0] * 16) - position.y;
        speed.y = -speed.y;
      } else if (position.y > start_pos.y + range.y[1] * 16) {
        position.y = 2 * (start_pos.y + range.y[1] * 16) - position.y;
        speed.y = -speed.y;
      }
    }

  };

}

extern "C" ultra::Entity* create_entity(
  const ultra::World::Boundaries& boundaries,
  const ultra::Tileset& tileset,
  const ultra::geometry::Vector<float>& position,
  ultra::Entity::Attributes attributes,
  uint16_t tile_index,
  uint16_t type,
  uint16_t id,
  uint32_t state
) {
  return new example::alpha::Platform(
    boundaries,
    tileset,
    position,
    attributes,
    tile_index,
    state
  );
}
