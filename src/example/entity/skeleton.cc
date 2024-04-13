#include <ultra240/ultra.h>

namespace example {

  class Skeleton : public ultra::Entity {

    constexpr static float speed = .3f;

    int direction;

    bool grounded = false;

    ultra::geometry::LineSegment<float> floor;

    ultra::geometry::Vector<float> force;

  public:

    Skeleton(
      const ultra::World::Boundaries& boundaries,
      const ultra::Tileset& tileset,
      const ultra::geometry::Vector<float>& position,
      ultra::Entity::Attributes attributes,
      uint16_t tile_index
    ) : Entity(
      ultra::hash("collision"_h),
      boundaries,
      tileset,
      position,
      attributes,
      ultra::Entity::AnimationControls(ultra::hash("walk"_h))
    ) {}

    void update(
      ultra::World::Boundaries& boundaries,
      ultra::Entity* player,
      const std::vector<ultra::Entity*>& entities
    ) {
      force.x = 0;
      if (attributes.flip_x) {
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
      std::pair<bool, ultra::Entity::BoundaryCollision> collision;
      grounded = false;
      do {
        collision = get_boundary_collision(
          force,
          get_collision_boxes(ultra::hash("collision"_h)),
          boundaries
        );
        if (collision.first) {
          auto boundary = *collision.second.boundary;
          auto edge = collision.second.edge;
          boundaries.erase(collision.second.boundary);
          position += collision.second.distance;
          if (edge == ultra::Entity::Collision::Edge::Bottom) {
            grounded = true;
            floor = boundary;
          }
          if (attributes.flip_x
              && edge == ultra::Entity::Collision::Edge::Right) {
            attributes.flip_x = false;
          } else if (!attributes.flip_x
                     && edge == ultra::Entity::Collision::Edge::Left) {
            attributes.flip_x = true;
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
          if (!attributes.flip_x) {
            direction = -direction;
          }
          force = ultra::geometry::Vector<float>::from_slope(slope, direction);
        } else {
          position += force;
        }
      } while (collision.first);
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
  return new example::Skeleton(
    boundaries,
    tileset,
    position,
    attributes,
    tile_index
  );
}
