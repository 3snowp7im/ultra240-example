#include <ultra240/ultra.h>
#include "example/example.h"

namespace example {

  class Skeleton : public SingleSpriteEntity {

    constexpr static float speed = .3f;

    int direction;

    bool grounded = false;

    ultra::geometry::LineSegment<float> floor;

    ultra::geometry::Vector<float> force;

  public:

    Skeleton(
      const ultra::Tileset& tileset,
      const ultra::renderer::TilesetHandle* handle,
      const ultra::geometry::Vector<float>& position,
      ultra::Tileset::Attributes attributes,
      uint16_t tile_index,
      const ultra::World::Boundaries& boundaries
    ) : SingleSpriteEntity(
          tileset,
          handle,
          position,
          attributes,
          tile_index
        ) {
      animate(
        ultra::hash("collision"_h),
        boundaries,
        ultra::AnimatedSprite::AnimationControls(ultra::hash("walk"_h))
      );
    }

    void update(UpdateContext context) {
      force.x = 0;
      if (sprite.attributes.flip_x) {
        if (grounded) {
          force = ultra::geometry::Vector<float>::from_slope(
            floor.to_line().slope(),
            speed
          );
        } else {
          force.x = speed;
        }
      } else {
        if (grounded) {
          force = ultra::geometry::Vector<float>::from_slope(
            floor.to_line().slope(),
            -speed
          );
        } else {
          force.x = -speed;
        }
      }
      if (!grounded) {
        force.y += .3f;
        if (force.y > 5.6f) {
          force.y = 5.6f;
        }
      }
      std::pair<bool, ultra::World::BoundaryCollision> collision;
      auto boundaries = context.boundaries;
      grounded = false;
      do {
        collision = get_boundary_collision(
          force,
          ultra::hash("collision"_h),
          boundaries
        );
        if (collision.first) {
          auto boundary = *collision.second.boundary;
          auto edge = collision.second.edge;
          boundaries.erase(collision.second.boundary);
          sprite.position += collision.second.distance;
          if (edge == ultra::World::Collision::Edge::Bottom) {
            grounded = true;
            floor = boundary;
          }
          if (sprite.attributes.flip_x
              && edge == ultra::World::Collision::Edge::Right) {
            sprite.attributes.flip_x = false;
          } else if (!sprite.attributes.flip_x
                     && edge == ultra::World::Collision::Edge::Left) {
            sprite.attributes.flip_x = true;
          }
          auto cross = boundary.to_vector().unit().cross(force.unit());
          if (cross == 0) {
            continue;
          }
          auto slope = boundary.to_line().slope();
          if (slope == std::numeric_limits<float>::infinity()) {
            force = force.as_y();
            continue;
          }
          auto direction = force.length();
          if (!sprite.attributes.flip_x) {
            direction = -direction;
          }
          force = ultra::geometry::Vector<float>::from_slope(slope, direction);
        } else {
          sprite.position += force;
        }
      } while (collision.first);
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
  return new example::Skeleton(
    tileset,
    handle,
    position,
    attributes,
    tile_index,
    boundaries
  );
}
