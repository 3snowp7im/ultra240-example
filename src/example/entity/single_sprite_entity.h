#pragma once

#include <ultra240/animated_sprite.h>
#include <ultra240/hash.h>
#include <ultra240/renderer.h>
#include "example/entity/entity.h"

namespace example {

  class SingleSpriteEntity : public Entity {
  public:

    struct UpdateContext {
      ultra::World::Boundaries& boundaries;
      SingleSpriteEntity** entities;
      size_t entities_count;
    };

    SingleSpriteEntity(
      const ultra::Tileset& tileset,
      ultra::Hash animation_name,
      const ultra::AnimatedSprite::Controls& controls,
      const ultra::renderer::TilesetHandle* handle,
      const ultra::geometry::Vector<float>& position,
      const ultra::Tileset::Attributes& attributes
    );

    SingleSpriteEntity(
      const ultra::Tileset& tileset,
      uint16_t tile_index,
      const ultra::renderer::TilesetHandle* handle,
      const ultra::geometry::Vector<float>& position,
      const ultra::Tileset::Attributes& attributes
    );

    virtual ~SingleSpriteEntity();

    virtual ultra::geometry::Vector<float> get_position() const;

    virtual void set_position(const ultra::geometry::Vector<float>& position);

    virtual ultra::Tileset::Attributes get_attributes() const;

    virtual void set_attributes(
      const ultra::Tileset::Attributes& attributes
    );

    virtual void update(UpdateContext context) = 0;

    const ultra::renderer::SpriteHandle* get_sprites() const;

    std::pair<bool, ultra::World::BoundaryCollision> get_boundary_collision(
      ultra::geometry::Vector<float> force,
      ultra::Hash collision_box_type,
      const ultra::World::Boundaries& boundaries
    ) const;

    bool animate(
      ultra::Hash name,
      const ultra::AnimatedSprite::Controls& controls,
      ultra::Hash collision_box_type,
      const ultra::World::Boundaries& boundaries,
      bool force_restart = false
    );

    bool update_animation(
      ultra::Hash collision_box_type,
      const ultra::World::Boundaries& boundaries
    );

    size_t get_collision_boxes_count(ultra::Hash type) const;

    template <typename T = float>
    void get_collision_boxes(
      ultra::Tileset::Tile::CollisionBox<T>* collision_boxes,
      ultra::Hash type
    ) const;

    ultra::AnimatedSprite sprite;

  private:

    const ultra::renderer::SpriteHandle* sprite_handle;
  };

}
