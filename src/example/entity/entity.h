#pragma once

#include <ultra240/entity.h>
#include <ultra240/renderer.h>

namespace example {

  class Entity : public ultra::Entity {
  public:

    Entity();

    virtual ~Entity();

    virtual ultra::geometry::Vector<float> get_position() const = 0;

    virtual void set_position(
      const ultra::geometry::Vector<float>& position
    ) = 0;

    virtual ultra::Tileset::Attributes get_attributes() const = 0;

    virtual void set_attributes(
      const ultra::Tileset::Attributes& attributes
    ) = 0;

    virtual const ultra::renderer::SpriteHandle* get_sprites() const = 0;
  };

}
