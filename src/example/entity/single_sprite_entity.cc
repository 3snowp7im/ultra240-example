#include "example/entity/single_sprite_entity.h"

namespace example {

  SingleSpriteEntity::SingleSpriteEntity(
    const ultra::Tileset& tileset,
    ultra::Hash name,
    const ultra::AnimatedSprite::Controls& controls,
    const ultra::renderer::TilesetHandle* handle,
    const ultra::geometry::Vector<float>& position,
    const ultra::Tileset::Attributes& attributes
  ) : sprite(
        tileset,
        name,
        controls,
        position,
        attributes
      ),
      sprite_handle(ultra::renderer::load_sprites(&sprite, 1, &handle, 1)),
      Entity() {}

  SingleSpriteEntity::SingleSpriteEntity(
    const ultra::Tileset& tileset,
    uint16_t tile_index,
    const ultra::renderer::TilesetHandle* handle,
    const ultra::geometry::Vector<float>& position,
    const ultra::Tileset::Attributes& attributes
  ) : sprite(
        tileset,
        tile_index,
        position,
        attributes
      ),
      sprite_handle(ultra::renderer::load_sprites(&sprite, 1, &handle, 1)),
      Entity() {}

  SingleSpriteEntity::~SingleSpriteEntity() {}

  ultra::geometry::Vector<float>
  SingleSpriteEntity::get_position() const {
    return sprite.position;
  }

  void SingleSpriteEntity::set_position(
    const ultra::geometry::Vector<float>& position
  ) {
    sprite.position = position;
  }

  ultra::Tileset::Attributes SingleSpriteEntity::get_attributes() const {
    return sprite.attributes;
  }

  void SingleSpriteEntity::set_attributes(
    const ultra::Tileset::Attributes& attributes
  ) {
    sprite.attributes = attributes;
  }

  const ultra::renderer::SpriteHandle*
  SingleSpriteEntity::get_sprites() const {
    return sprite_handle;
  }

  std::pair<bool, ultra::World::BoundaryCollision>
  SingleSpriteEntity::get_boundary_collision(
    ultra::geometry::Vector<float> force,
    ultra::Hash collision_box_type,
    const ultra::World::Boundaries& boundaries
  ) const {
    size_t collision_boxes_count = sprite.tileset.get_collision_boxes_count(
      sprite.tile_index,
      collision_box_type
    );
    ultra::Tileset::Tile::CollisionBox<float>
      collision_boxes[collision_boxes_count];
    sprite.tileset.get_collision_boxes(
      collision_boxes,
      sprite.tile_index,
      collision_box_type,
      sprite.position,
      sprite.attributes
    );
    return ultra::World::get_boundary_collision(
      force,
      collision_boxes,
      collision_boxes_count,
      boundaries
    );
  }

  static std::pair<bool, ultra::geometry::Vector<float>> can_set_tile_index(
    const ultra::geometry::Vector<float>& position,
    ultra::Hash collision_box_type,
    const ultra::Tileset& tileset,
    uint16_t prev_tile_index,
    uint16_t next_tile_index,
    const ultra::Tileset::Attributes& attributes,
    const ultra::World::Boundaries& boundaries,
    bool check_transits
  ) {
    size_t prev_boxes_count = tileset.get_collision_boxes_count(
      prev_tile_index,
      collision_box_type
    );
    ultra::Tileset::Tile::CollisionBox<float> prev_boxes[prev_boxes_count];
    tileset.get_collision_boxes(
      prev_boxes,
      prev_tile_index,
      collision_box_type,
      position,
      attributes
    );
    size_t next_boxes_count = tileset.get_collision_boxes_count(
      next_tile_index,
      collision_box_type
    );
    ultra::Tileset::Tile::CollisionBox<float> next_boxes[next_boxes_count];
    tileset.get_collision_boxes(
      next_boxes,
      next_tile_index,
      collision_box_type,
      position,
      attributes
    );
    return ultra::World::can_fit_collision_boxes(
      prev_boxes,
      prev_boxes_count,
      next_boxes,
      next_boxes_count,
      boundaries,
      check_transits
    );
  }

  bool SingleSpriteEntity::animate(
    ultra::Hash name,
    const ultra::AnimatedSprite::Controls& controls,
    ultra::Hash collision_box_type,
    const ultra::World::Boundaries& boundaries,
    bool force_restart
  ) {
    ultra::AnimatedSprite::Animation next = sprite.animation.set(
      name,
      controls,
      force_restart
    );
    auto can_set = can_set_tile_index(
      sprite.position,
      collision_box_type,
      sprite.tileset,
      sprite.animation.get_tile_index(),
      next.get_tile_index(),
      sprite.attributes,
      boundaries,
      true
    );
    if (!can_set.first) {
      return false;
    }
    sprite.position += can_set.second;
    sprite.animate(name, controls, force_restart);
    return true;
  }

  bool SingleSpriteEntity::update_animation(
    ultra::Hash collision_box_type,
    const ultra::World::Boundaries& boundaries
  ) {
    ultra::AnimatedSprite::Animation next = sprite.animation.update();
    auto can_set = can_set_tile_index(
      sprite.position,
      collision_box_type,
      sprite.tileset,
      sprite.animation.get_tile_index(),
      next.get_tile_index(),
      sprite.attributes,
      boundaries,
      true
    );
    if (!can_set.first) {
      return false;
    }
    sprite.position += can_set.second;
    sprite.update_animation();
    return true;
  }

  size_t SingleSpriteEntity::get_collision_boxes_count(ultra::Hash type) const {
    return sprite.tileset.get_collision_boxes_count(sprite.tile_index, type);
  }

  template <>
  void SingleSpriteEntity::get_collision_boxes<float>(
    ultra::Tileset::Tile::CollisionBox<float>* collision_boxes,
    ultra::Hash collision_box_type
  ) const {
    sprite.tileset.get_collision_boxes(
      collision_boxes,
      sprite.tile_index,
      collision_box_type,
      sprite.position,
      sprite.attributes
    );
  }

  template <>
  void SingleSpriteEntity::get_collision_boxes<uint16_t>(
    ultra::Tileset::Tile::CollisionBox<uint16_t>* collision_boxes,
    ultra::Hash collision_box_type
  ) const {
    sprite.tileset.get_collision_boxes<uint16_t>(
      collision_boxes,
      sprite.tile_index,
      collision_box_type
    );
  }

}
