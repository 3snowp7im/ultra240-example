#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <SDL.h>
#include <SDL_syswm.h>
#include <thread>
#include <ultra240/ultra.h>
#include <wayland-egl.h>
#include "example/example.h"

#ifndef NDEBUG
#include <bx/debug.h>
#endif

namespace shader {
#include "shader/vs.c"
#include "shader/tile.c"
#include "shader/rt.c"
#include "shader/box.c"
}

#define MAX_TILES     8192
#define MAX_VERTICES  (6 * MAX_TILES)

struct Vertex {
  float a_position[4];
  float a_texcoord0[4];
  float a_color0[4];
};

static struct { float x, y; } draw_size, draw_offset;

static ultra::renderer::Transform vertex_transforms[MAX_TILES];
static ultra::renderer::Transform tex_transforms[MAX_TILES];

static float quad_vertices[4][4] = {
  {0, 0, 1, 1},
  {0, 1, 1, 1},
  {1, 0, 1, 1},
  {1, 1, 1, 1},
};

static float proj[16];

static bgfx::VertexLayout vertex_layout;
static bgfx::TextureHandle tilesets_texture;
static bgfx::TextureHandle frame_buffer_textures[2];
static bgfx::FrameBufferHandle frame_buffer;
static bgfx::VertexBufferHandle rendered_vertex_buffer;
static bgfx::ProgramHandle tile_program;
static bgfx::ProgramHandle rt_program;
static bgfx::ProgramHandle box_program;
static bgfx::UniformHandle s_tex;
static ultra::geometry::Vector<float> camera_pos;
static bool render_collision_boxes = false;

static int dbg_line;
static int txt_top;
static int txt_left;

struct PlayerState {
  bool walking;
  bool grounded;
  bool jump_reset;
  int jump_counter;
  ultra::geometry::LineSegment<float> floor;
};

#ifndef NDEBUG
#define DEBUG(...) bgfx::dbgTextPrintf(txt_left, dbg_line++, 0x0e, __VA_ARGS__);
#else
#define DEBUG(...)
#endif

void print_debug_text(
  const ultra::geometry::Vector<float>& player_pos,
  const ultra::World::Map& map,
  const PlayerState& state
) {
  DEBUG("Move: <wasd> or directional pad");
  DEBUG("Jump: space bar or face button");
  DEBUG("Pause: <f>");
  DEBUG("Frame advance: <n> (while paused)");
  DEBUG("Show collision boxes: <b>");
  DEBUG("Quit: <q>");
  DEBUG("Player position: %s", player_pos.to_string().c_str());
  DEBUG("Camera position: %s", camera_pos.to_string().c_str());
  DEBUG("Map position: %s", map.position.to_string().c_str());
  std::string ground;
  if (state.grounded) {
    ground = state.floor.as<int>().to_string();
  } else {
    ground = "(nil)";
  }
  DEBUG("Ground: %s", ground.c_str());
}

enum ButtonIndex {
  Invalid = -1,
  A,
  B,
  X,
  Y,
  Back,
  Guide,
  Start,
  LeftStick,
  RightStick,
  LeftShoulder,
  RightShoulder,
  DpadUp,
  DpadDown,
  DpadLeft,
  DpadRight,
};

struct Platform {
  const example::SingleSpriteEntity* entity;
  ultra::geometry::Vector<float> position;
  ultra::Tileset::Tile::CollisionBox<uint16_t> collision_box;
  ultra::World::Boundaries::const_iterator boundary;
};

static void reset_boundaries(
  ultra::World::Boundaries& boundaries,
  ultra::World& world,
  std::list<Platform>& platforms) {
  boundaries.clear();
  std::copy(
    world.get_boundaries().cbegin(),
    world.get_boundaries().cend(),
    std::back_inserter(boundaries)
  );
  for (auto& platform : platforms) {
    auto box = platform.entity->sprite.tileset.adjust_collision_box(
      platform.collision_box,
      platform.entity->get_position(),
      platform.entity->get_attributes()
    );
    platform.position = platform.entity->get_position();
    platform.boundary = boundaries.insert(
      boundaries.end(),
      ultra::World::Boundary(
        ultra::World::Boundary::Flags::OneWay,
        box.position,
        box.position + box.size.as_x()
      )
    );
  }
}

static ultra::geometry::Vector<float> get_camera_position(
  const ultra::geometry::Vector<float>& player_pos,
  const ultra::geometry::Vector<int16_t>& map_pos,
  const ultra::geometry::Vector<uint16_t>& map_size
) {
  // Center camera to player.
  auto camera = player_pos
    - map_pos.as<float>() * 16.f
    - ultra::geometry::Vector<float>(128, 120);
  // Keep camera in map bounds.
  if (camera.x < 0) {
    camera.x = 0;
  } else if (camera.x > (map_size.x - 16) * 16) {
    camera.x = (map_size.x - 16) * 16;
  }
  if (camera.y < 0) {
    camera.y = 0;
  } else if (camera.y > (map_size.y - 15) * 16) {
    camera.y = (map_size.y - 15) * 16;
  }
  return camera;
}

static void render(
  const ultra::World::Map& map,
  example::SingleSpriteEntity** entities,
  size_t entities_count,
  bool advance_time
) {
  size_t id = 0;
  size_t sprite_layer_index;
  for (size_t layer_index = 0; layer_index < map.layers.size(); layer_index++) {

    // Get view transform.
    float view[16];
    ultra::renderer::get_view_transform(view, camera_pos, layer_index);

    size_t count = 0;

    // Render tiles.
    auto tile_count = ultra::renderer::get_map_transforms(
      &vertex_transforms[count],
      &tex_transforms[count],
      MAX_TILES - count,
      layer_index
    );
    count += tile_count;

    if (layer_index == map.layers.size() - 1
        || map.layers[layer_index + 1].name != ultra::hash("background"_h)) {
      // Get sprite handles.
      const ultra::renderer::SpriteHandle* sprites[entities_count];
      for (size_t i = 0; i < entities_count; i++) {
        sprites[i] = entities[i]->get_sprites();
      }
      // Render sprites.
      auto sprite_count = ultra::renderer::get_sprite_transforms(
        &vertex_transforms[count],
        &tex_transforms[count],
        MAX_TILES - count,
        sprites,
        entities_count,
        layer_index
      );
      count += entities_count;
      sprite_layer_index = layer_index;
    }

    // Allocate vertices.
    count = bgfx::getAvailTransientVertexBuffer(6 * count, vertex_layout) / 6;
    bgfx::TransientVertexBuffer vertex_buffer;
    bgfx::allocTransientVertexBuffer(&vertex_buffer, 6 * count, vertex_layout);
    Vertex* vertices = reinterpret_cast<Vertex*>(vertex_buffer.data);

    // Transform vertices.
    for (size_t i = 0; i < count; i++) {
      bx::vec4MulMtx(
        vertices[6 * i + 0].a_position,
        quad_vertices[0],
        vertex_transforms[i]
      );
      bx::vec4MulMtx(
        vertices[6 * i + 0].a_texcoord0,
        quad_vertices[0],
        tex_transforms[i]
      );
      memset(vertices[6 * i + 0].a_color0, 0, 4 * sizeof(float));

      bx::vec4MulMtx(
        vertices[6 * i + 1].a_position,
        quad_vertices[1],
        vertex_transforms[i]
      );
      bx::vec4MulMtx(
        vertices[6 * i + 1].a_texcoord0,
        quad_vertices[1],
        tex_transforms[i]
      );
      memset(vertices[6 * i + 1].a_color0, 0, 4 * sizeof(float));

      bx::vec4MulMtx(
        vertices[6 * i + 2].a_position,
        quad_vertices[2],
        vertex_transforms[i]
      );
      bx::vec4MulMtx(
        vertices[6 * i + 2].a_texcoord0,
        quad_vertices[2],
        tex_transforms[i]
      );
      memset(vertices[6 * i + 2].a_color0, 0, 4 * sizeof(float));

      vertices[6 * i + 3] = vertices[6 * i + 2];
      vertices[6 * i + 4] = vertices[6 * i + 1];

      bx::vec4MulMtx(
        vertices[6 * i + 5].a_position,
        quad_vertices[3],
        vertex_transforms[i]
      );
      bx::vec4MulMtx(
        vertices[6 * i + 5].a_texcoord0,
        quad_vertices[3],
        tex_transforms[i]
      );
      memset(vertices[6 * i + 5].a_color0, 0, 4 * sizeof(float));
    }

    // Draw quads.
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_BLEND_ALPHA);
    bgfx::setVertexBuffer(0, &vertex_buffer);
    bgfx::setTexture(
      0,
      s_tex,
      tilesets_texture,
      BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
    );
    bgfx::setViewFrameBuffer(id, frame_buffer);
    bgfx::setViewTransform(id, view, proj);
    bgfx::setViewRect(id, 0, 0, 256, 240);
    bgfx::setViewClear(id, BGFX_CLEAR_DEPTH, 0x00000000);
    bgfx::submit(id, tile_program);
    id++;
  }

  // Draw rendered texture to screen.
  bgfx::setState(BGFX_STATE_WRITE_RGB);
  bgfx::setVertexBuffer(0, rendered_vertex_buffer);
  bgfx::setTexture(
    0,
    s_tex,
    bgfx::getTexture(frame_buffer, 0),
    BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
  );
  bgfx::setViewRect(
    id,
    draw_offset.x,
    draw_offset.y,
    draw_size.x,
    draw_size.y
  );
  bgfx::setViewClear(id, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff);
  bgfx::submit(id, rt_program);
  id++;

  // Draw collision boxes
  if (render_collision_boxes) {
    // Get view transform.
    float view[16];
    ultra::renderer::get_view_transform(view, camera_pos, sprite_layer_index);
    // Count all boxes.
    size_t count = 0;
    for (size_t i = 0; i < entities_count; i++) {
      count += entities[i]->get_collision_boxes_count(
        ultra::hash("collision"_h)
      );
    }
    // Allocate vertex buffer.
    count = bgfx::getAvailTransientVertexBuffer(8 * count, vertex_layout) / 8;
    bgfx::TransientVertexBuffer vertex_buffer;
    bgfx::allocTransientVertexBuffer(&vertex_buffer, 8 * count, vertex_layout);
    // Create geometry.
    Vertex* vertices = reinterpret_cast<Vertex*>(vertex_buffer.data);
    size_t vertices_count = 0;
    for (size_t i = 0; i < entities_count; i++) {
      size_t boxes_count = entities[i]->get_collision_boxes_count(
        ultra::hash("collision"_h)
      );
      ultra::Tileset::Tile::CollisionBox<float> boxes[boxes_count];
      entities[i]->get_collision_boxes(boxes, ultra::hash("collision"_h));
      for (size_t j = 0; j < boxes_count; j++) {
        if (vertices_count >= 8 * count) {
          break;
        }
        auto box = boxes[j];
        auto pos = box.position;

        float tl[] = {pos.x, pos.y, 0, 1};
        float tr[] = {pos.x + box.size.x, pos.y, 0, 1};
        float br[] = {pos.x + box.size.x, pos.y + box.size.y, 0, 1};
        float bl[] = {pos.x, pos.y + box.size.y, 0, 1};
        float red[] = {1, 0, 0, 1};

        memcpy(vertices->a_position, tl, sizeof(tl));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;
        memcpy(vertices->a_position, tr, sizeof(tr));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;

        memcpy(vertices->a_position, tr, sizeof(tr));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;
        memcpy(vertices->a_position, br, sizeof(br));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;

        memcpy(vertices->a_position, br, sizeof(br));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;
        memcpy(vertices->a_position, bl, sizeof(bl));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;

        memcpy(vertices->a_position, bl, sizeof(bl));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;
        memcpy(vertices->a_position, tl, sizeof(tl));
        memset(vertices->a_texcoord0, 0, 16 * sizeof(float));
        memcpy(vertices->a_color0, red, sizeof(red));
        vertices++;

        vertices_count += 8;
      }
    }
    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
    bgfx::setVertexBuffer(0, &vertex_buffer);
    bgfx::setViewTransform(id, view, proj);
    bgfx::setViewRect(
      id,
      draw_offset.x,
      draw_offset.y,
      draw_size.x,
      draw_size.y
    );
    bgfx::setViewClear(id, BGFX_CLEAR_DEPTH, 0x00000000);
    bgfx::submit(id, box_program);
    id++;
  }

  // Render.
  bgfx::frame();

  // Advance time.
  if (advance_time) {
    ultra::renderer::advance();
  }
}

int main() {
  // Initialize SDL.
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
    std::string sdl_error(SDL_GetError());
    throw std::runtime_error("Could not initialize SDL: " + sdl_error);
  }

  // Get display size.
  SDL_Rect bounds;
  if (SDL_GetDisplayBounds(0, &bounds) != 0) {
    std::string sdl_error(SDL_GetError());
    throw std::runtime_error("Could not get display bounds: " + sdl_error);
  }

  // Create window.
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
    SDL_CreateWindow(
      "ULTRA240",
      0,
      0,
      bounds.w,
      bounds.h,
      SDL_WINDOW_FULLSCREEN
    ),
    SDL_DestroyWindow
  );
  if (window == nullptr) {
    std::string sdl_error(SDL_GetError());
    throw std::runtime_error("Could not create window: " + sdl_error);
  }

  // Get window info.
  SDL_SysWMinfo wmi;
  SDL_VERSION(&wmi.version);
  if (!SDL_GetWindowWMInfo(window.get(), &wmi)) {
    std::string sdl_error(SDL_GetError());
    throw std::runtime_error("Could not get window info: " + sdl_error);
  }

  // Get native graphics info.
  bgfx::NativeWindowHandleType::Enum type;
  void* ndt;
  void* nwh;
  if (wmi.subsystem == SDL_SYSWM_WAYLAND) {
    wl_egl_window* win_impl = reinterpret_cast<wl_egl_window*>(
      SDL_GetWindowData(window.get(), "wl_egl_window")
    );
    if (!win_impl) {
      int width, height;
      SDL_GetWindowSize(window.get(), &width, &height);
      struct wl_surface* surface = wmi.info.wl.surface;
      if (!surface) {
        throw std::runtime_error("Could not create wayland surface");
      }
      win_impl = wl_egl_window_create(surface, width, height);
      SDL_SetWindowData(window.get(), "wl_egl_window", win_impl);
    }
    type = bgfx::NativeWindowHandleType::Wayland;
    ndt = wmi.info.wl.display;
    nwh = win_impl;
  } else {
    type = bgfx::NativeWindowHandleType::Default;
    ndt = wmi.info.x11.display;
    nwh = reinterpret_cast<void*>(wmi.info.x11.window);
  }

  // Calculate draw offset.
  float ratio = 16. / 15.;
  if (bounds.w > bounds.h) {
    draw_size.x = bounds.h * ratio;
    draw_size.y = bounds.h;
  } else {
    draw_size.x = bounds.w;
    draw_size.y = bounds.w * ratio;
  }
  txt_left = (bounds.w - draw_size.x) / 16 + 3;
  txt_top = (bounds.h - draw_size.y) / 32 + 1;
  draw_offset.x = (bounds.w - draw_size.x) / 2.;
  draw_offset.y = (bounds.h - draw_size.y) / 2.;

  // Prevent bgfx from creating a renderer thread.
  bgfx::renderFrame();

  // Initialize bgfx.
  bgfx::Init init;
  init.type = bgfx::RendererType::OpenGL;
  init.platformData.ndt = ndt;
  init.platformData.nwh = nwh;
  init.resolution.width = bounds.w;
  init.resolution.height = bounds.h;
  init.resolution.reset = BGFX_RESET_VSYNC;
  if (!bgfx::init(init)) {
    throw std::runtime_error("Could not init bgfx");
  }
#ifdef BX_CONFIG_DEBUG
  bgfx::setDebug(BGFX_DEBUG_TEXT);
#endif

  // Create shaders.
  auto vs_shader = bgfx::createShader(
    bgfx::makeRef(shader::vs, sizeof(shader::vs))
  );
  auto tile_shader = bgfx::createShader(
    bgfx::makeRef(shader::tile, sizeof(shader::tile))
  );
  auto rt_shader = bgfx::createShader(
    bgfx::makeRef(shader::rt, sizeof(shader::rt))
  );
  auto box_shader = bgfx::createShader(
    bgfx::makeRef(shader::box, sizeof(shader::box))
  );

  // Create tile drawing program.
  tile_program = bgfx::createProgram(
    vs_shader,
    tile_shader
  );

  // Create render target drawing program.
  rt_program = bgfx::createProgram(
    vs_shader,
    rt_shader
  );

  // Create collision box drawing program.
  box_program = bgfx::createProgram(
    vs_shader,
    box_shader
  );

  // Free shaders.
  bgfx::destroy(vs_shader);
  bgfx::destroy(tile_shader);
  bgfx::destroy(rt_shader);
  bgfx::destroy(box_shader);

  // Create texture uniform.
  s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

  // Create tileset texture.
  tilesets_texture = bgfx::createTexture2D(
    ultra::renderer::texture_width,
    ultra::renderer::texture_height,
    false,
    ultra::renderer::texture_count,
    bgfx::TextureFormat::RGBA8
  );

  // Create frame buffer color attachment.
  frame_buffer_textures[0] = bgfx::createTexture2D(
    256,
    240,
    false,
    1,
    bgfx::TextureFormat::RGBA8,
    BGFX_TEXTURE_RT
  );

  // Create frame buffer depth attachment.
  frame_buffer_textures[1] = bgfx::createTexture2D(
    256,
    240,
    false,
    1,
    bgfx::TextureFormat::D16,
    BGFX_TEXTURE_RT
  );

  // Create frame buffer.
  frame_buffer = bgfx::createFrameBuffer(
    2,
    frame_buffer_textures,
    true
  );

  // Define the common vertex layout.
  vertex_layout
    .begin()
    .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
    .add(bgfx::Attrib::TexCoord0, 4, bgfx::AttribType::Float)
    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
    .end();

  // Get the screen space transform matrix.
  ultra::renderer::get_projection_transform(proj);

  // Create vertices for drawing render texture.
  Vertex rendered_vertices[] = {{
    .a_position = {-1, +1, 0, 1},
    .a_texcoord0 = {0, 1, 0, 1},
    .a_color0 = {0},
  }, {
    .a_position = {-1, -1, 0, 1},
    .a_texcoord0 = {0, 0, 0, 1},
    .a_color0 = {0},
  }, {
    .a_position = {+1, +1, 0, 1},
    .a_texcoord0 = {1, 1, 0, 1},
    .a_color0 = {0},
  }, {
    .a_position = {+1, +1, 0, 1},
    .a_texcoord0 = {1, 1, 0, 1},
    .a_color0 = {0},
  }, {
    .a_position = {-1, -1, 0, 1},
    .a_texcoord0 = {0, 0, 0, 1},
    .a_color0 = {0},
  }, {
    .a_position = {+1, -1, 0, 1},
    .a_texcoord0 = {1, 0, 0, 1},
    .a_color0 = {0},
  }};
  rendered_vertex_buffer = bgfx::createVertexBuffer(
    bgfx::makeRef(rendered_vertices, sizeof(rendered_vertices)),
    vertex_layout
  );

  bgfx::frame();

  // Init ULTRA240.
  ultra::init("ultra240-example");
  bgfx::overrideInternal(tilesets_texture, ultra::renderer::get_texture());

  // Load world.
  ultra::World alpha("alpha");
  ultra::renderer::load_world(alpha);
  ultra::World::Boundaries boundaries(
    ultra::VectorAllocator<ultra::World::Boundary>(1024)
  );

  // Declare stuff needed for loaded map entities.
  const ultra::renderer::TilesetHandle* map_tileset_handle;
  std::vector<std::shared_ptr<example::SingleSpriteEntity>> map_entities;
  std::vector<example::SingleSpriteEntity*> loaded_entities;
  std::vector<const example::SingleSpriteEntity*> new_entities;
  std::list<Platform> platforms;
  ssize_t curr_map_index = -1;
  struct {
    struct {
      size_t min, max;
    } x, y;
  } entity_window;

  // Allocate enough storage for entities so that reallocations are avoided.
  loaded_entities.reserve(512);
  new_entities.reserve(512);

  // Load player tileset.
  ultra::Tileset victor_tileset("victor");
  auto victor_tileset_handle = ultra::renderer::load_tilesets(
    &victor_tileset,
    1
  );

  // Create player entity.
  std::shared_ptr<example::SingleSpriteEntity> victor(
    ultra::Entity::Factory<example::SingleSpriteEntity>::from_tileset(
      victor_tileset,
      victor_tileset_handle,
      {32, 512},
      {},
      alpha.get_boundaries()
    )
  );
  loaded_entities.push_back(victor.get());

  // Declare input struct.
  struct {
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool jump = false;
  } input;

  // Player gravity.
  const float gravity_accel = .3f;
  const float terminal_velocity = 5.6f;

  // Player movement parameters.
  const float jump_accel = 2.15f;
  const float jump_momentum_loss_factor = .88f;
  //const int jump_accel_frames = 45;
  const int jump_accel_frames = 25;
  const int jump_rise_frames = 20;

  // Declare player state
  PlayerState state = {
    .walking = false,
    .grounded = false,
    .jump_reset = false,
    .jump_counter = 0,
  };

  // Force vector.
  ultra::geometry::Vector<float> force;

  // Collision detection.
  float ground_cross = std::numeric_limits<float>::infinity();
  std::pair<bool, ultra::World::BoundaryCollision> collision;

  // Event loop.
  bool frame_advancing = false;
  while (true) {

    // Clear debug text buffer.
    bgfx::dbgTextClear(0);
    dbg_line = txt_top;

    // Determine map based on player's world position.
    auto victor_pos = victor->get_position()
      + victor->sprite.tileset.tile_size.as_x().as<int>() / 2
      - victor->sprite.tileset.tile_size.as_y().as<int>() / 2;
    for (size_t i = 0; i < alpha.maps.size(); i++) {
      auto& map = alpha.maps[i];
      if (victor_pos.x >= 16 * map.position.x
          && victor_pos.y >= 16 * map.position.y
          && victor_pos.x < 16 * (map.position.x + map.size.x)
          && victor_pos.y < 16 * (map.position.y + map.size.y)) {
        if (curr_map_index != i) {
          curr_map_index = i;
          map_tileset_handle = ultra::renderer::set_map(curr_map_index);
          platforms.clear();
          const ultra::renderer::SpriteHandle* handles[map_entities.size()];
          for (size_t j = 0; j < map_entities.size(); j++) {
            handles[j] = map_entities[j]->get_sprites();
          }
          ultra::renderer::unload_sprites(handles, map_entities.size());
          map_entities.clear();
          map_entities.resize(map.entities.size());
          loaded_entities.clear();
          loaded_entities.push_back(victor.get());
          auto pos = victor_pos - map.position;
          if (pos.x < 16 * map.size.x / 2) {
            entity_window.x.min = 0;
            entity_window.x.max = 0;
          } else {
            entity_window.x.min = map.entities.size();
            entity_window.x.max = map.entities.size();
          }
          if (pos.y < 16 * map.size.y / 2) {
            entity_window.y.min = 0;
            entity_window.y.max = 0;
          } else {
            entity_window.y.min = map.entities.size();
            entity_window.y.max = map.entities.size();
          }
        }
        break;
      }
    }
    auto& map = alpha.maps[curr_map_index];

    // Get camera position.
    camera_pos = get_camera_position(
      victor_pos,
      map.position,
      map.size
    );

    // Clear new entities list.
    new_entities.clear();

    // Check for entities entering the window from the left.
    for (ssize_t i = entity_window.x.min; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto index = map.sorted_entities.x.max[i];
      auto& entity = map.entities[index];
      auto right = entity.position.x + entity.tileset.tile_size.x;
      auto top = entity.position.y - entity.tileset.tile_size.y;
      auto bottom = entity.position.y;
      if (right >= camera_pos.x) {
        if (i > 0) {
          entity_window.x.min--;
        }
        if (bottom >= camera_pos.y
            && top <= camera_pos.y + 240
            && map_entities[index] == nullptr) {
          map_entities[index].reset(
            ultra::Entity::Factory<example::SingleSpriteEntity>::from_map(
              entity,
              map_tileset_handle,
              alpha.get_boundaries()
            )
          );
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the left.
    for (size_t i = entity_window.x.min; i < map.entities.size(); i++) {
      auto& entity = map.entities[map.sorted_entities.x.max[i]];
      auto right = entity.position.x + entity.tileset.tile_size.x;
      if (right < camera_pos.x) {
        entity_window.x.min++;
      } else {
        break;
      }
    }

    // Check for entities entering the window from the right.
    for (size_t i = entity_window.x.max; i < map.entities.size(); i++) {
      auto index = map.sorted_entities.x.min[i];
      auto& entity = map.entities[index];
      auto top = entity.position.y - entity.tileset.tile_size.y;
      auto bottom = entity.position.y;
      auto left = entity.position.x;
      if (left <= camera_pos.x + 256) {
        entity_window.x.max++;
        if (bottom >= camera_pos.y
            && top <= camera_pos.y + 240
            && map_entities[index] == nullptr) {
          map_entities[index].reset(
            ultra::Entity::Factory<example::SingleSpriteEntity>::from_map(
              entity,
              map_tileset_handle,
              alpha.get_boundaries()
            )
          );
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the right.
    for (ssize_t i = entity_window.x.max; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto& entity = map.entities[map.sorted_entities.x.min[i]];
      auto left = entity.position.x;
      if (left > camera_pos.x + 256) {
        if (i > 0) {
          entity_window.x.max--;
        }
      } else {
        break;
      }
    }

    // Check for entities entering the window from the bottom.
    for (size_t i = entity_window.y.max; i < map.entities.size(); i++) {
      auto index = map.sorted_entities.y.min[i];
      auto& entity = map.entities[index];
      auto top = entity.position.y - entity.tileset.tile_size.y;
      auto right = entity.position.x + entity.tileset.tile_size.x;
      auto left = entity.position.x;
      if (top <= camera_pos.y + 240) {
        entity_window.y.max++;
        if (right >= camera_pos.x
            && left <= camera_pos.x + 256
            && map_entities[index] == nullptr) {
          map_entities[index].reset(
            ultra::Entity::Factory<example::SingleSpriteEntity>::from_map(
              entity,
              map_tileset_handle,
              alpha.get_boundaries()
            )
          );
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the bottom.
    for (ssize_t i = entity_window.y.max; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto& entity = map.entities[map.sorted_entities.y.min[i]];
      auto top = entity.position.y - entity.tileset.tile_size.y;
      if (top > camera_pos.y + 240) {
        if (i > 0) {
          entity_window.y.max--;
        }
      } else {
        break;
      }
    }

    // Check for entities entering the window from the top.
    for (ssize_t i = entity_window.y.min; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto index = map.sorted_entities.y.max[i];
      auto& entity = map.entities[index];
      auto bottom = entity.position.y;
      auto right = entity.position.x + entity.tileset.tile_size.x;
      auto left = entity.position.x;
      if (bottom >= camera_pos.y) {
        if (i > 0) {
          entity_window.y.min--;
        }
        if (right >= camera_pos.x
            && left <= camera_pos.x + 256
            && map_entities[index] == nullptr) {
          map_entities[index].reset(
            ultra::Entity::Factory<example::SingleSpriteEntity>::from_map(
              entity,
              map_tileset_handle,
              alpha.get_boundaries()
            )
          );
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the top.
    for (size_t i = entity_window.y.min; i < map.entities.size(); i++) {
      auto& entity = map.entities[map.sorted_entities.y.max[i]];
      auto bottom = entity.position.y;
      if (bottom < camera_pos.y) {
        entity_window.y.min++;
      } else {
        break;
      }
    }

    // Load new entities.
    if (new_entities.size()) {
      // Create boundaries for newly loaded platforms.
      for (const auto entity : new_entities) {
        size_t count = entity->get_collision_boxes_count(
          ultra::hash("boundary"_h)
        );
        ultra::Tileset::Tile::CollisionBox<uint16_t> boxes[count];
        entity->get_collision_boxes<uint16_t>(
          boxes,
          ultra::hash("boundary"_h)
        );
        for (size_t i = 0; i < count; i++) {
          if (boxes[i].name == ultra::hash("platform"_h)) {
            platforms.push_back({
              entity,
              entity->get_position(),
              boxes[i],
              boundaries.cend(),
            });
          }
        }
      }
    }

    // Poll for events.
    bool quit = false;
    bool advance = false;
    bool printed_debug_text = false;
    do {
      SDL_Event sdl_event;
      while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          if (!sdl_event.key.repeat) {
            switch (sdl_event.key.keysym.scancode) {
            case SDL_SCANCODE_W:
              input.up = true;
              break;
            case SDL_SCANCODE_S:
              input.down = true;
              break;
            case SDL_SCANCODE_A:
              input.left = true;
              break;
            case SDL_SCANCODE_D:
              input.right = true;
              break;
            case SDL_SCANCODE_SPACE:
              input.jump = true;
              break;
            case SDL_SCANCODE_Q:
              quit = true;
              break;
            }
            break;
          }
          break;
        case SDL_KEYUP:
          if (!sdl_event.key.repeat) {
            switch (sdl_event.key.keysym.scancode) {
            case SDL_SCANCODE_W:
              input.up = false;
              break;
            case SDL_SCANCODE_S:
              input.down = false;
              break;
            case SDL_SCANCODE_A:
              input.left = false;
              break;
            case SDL_SCANCODE_D:
              input.right = false;
              break;
            case SDL_SCANCODE_SPACE:
              input.jump = false;
              break;
            // Debugging keys.
            case SDL_SCANCODE_N:
              advance = true;
              break;
            case SDL_SCANCODE_B:
              render_collision_boxes = !render_collision_boxes;
              break;
            case SDL_SCANCODE_F:
              frame_advancing = !frame_advancing;
              if (frame_advancing) {
                advance = true;
              }
              break;
            }
          }
          break;
        case SDL_JOYHATMOTION:
          switch (sdl_event.jhat.value) {
          case SDL_HAT_CENTERED:
            input.up = false;
            input.down = false;
            input.left = false;
            input.right = false;
            break;
          case SDL_HAT_UP:
            input.up = true;
            input.down = false;
            input.left = false;
            input.right = false;
            break;
          case SDL_HAT_DOWN:
            input.up = false;
            input.down = true;
            input.left = false;
            input.right = false;
            break;
          case SDL_HAT_LEFT:
            input.up = false;
            input.down = false;
            input.left = true;
            input.right = false;
            break;
          case SDL_HAT_RIGHT:
            input.up = false;
            input.down = false;
            input.left = false;
            input.right = true;
            break;
          case SDL_HAT_LEFTUP:
            input.up = true;
            input.down = false;
            input.left = true;
            input.right = false;
            break;
          case SDL_HAT_RIGHTUP:
            input.up = true;
            input.down = false;
            input.left = false;
            input.right = true;
            break;
          case SDL_HAT_LEFTDOWN:
            input.up = false;
            input.down = true;
            input.left = true;
            input.right = false;
            break;
          case SDL_HAT_RIGHTDOWN:
            input.up = false;
            input.down = true;
            input.left = false;
            input.right = true;
            break;
          }
          break;
        case SDL_JOYBUTTONDOWN:
          switch (sdl_event.jbutton.button) {
          case ButtonIndex::Start:
            quit = true;
            break;
          case ButtonIndex::A:
            input.jump = true;
            break;
          }
          break;
        case SDL_JOYBUTTONUP:
          switch (sdl_event.jbutton.button) {
          case ButtonIndex::A:
            input.jump = false;
            break;
          }
          break;
        }
      }
      if (frame_advancing) {
        bgfx::dbgTextClear(0);
        dbg_line = txt_top;
        if (!advance) {
          print_debug_text(victor_pos, map, state);
          printed_debug_text = true;
          render(
            map,
            &loaded_entities[0],
            loaded_entities.size(),
            false
          );
        }
      }
    } while (frame_advancing && !advance && !quit);

    if (quit) {
      break;
    }

    // Update entities.
    for (auto entity : loaded_entities) {
      boundaries.clear();
      std::copy(
        alpha.get_boundaries().cbegin(),
        alpha.get_boundaries().cend(),
        std::back_inserter(boundaries)
      );
      entity->update({
        .boundaries = boundaries,
        .entities = &loaded_entities[0],
        .entities_count = loaded_entities.size(),
      });
    }

    // Apply platform forces.
    for (auto& platform : platforms) {
      auto dst = platform.entity->get_position() - platform.position;
      auto box = platform.entity->sprite.tileset.adjust_collision_box(
        platform.collision_box,
        platform.position,
        platform.entity->get_attributes()
      );
      ultra::geometry::LineSegment<float> boundary(
        box.position,
        box.position + platform.collision_box.size.as_x()
      );
      if (state.grounded && state.floor == boundary) {
        victor->set_position(victor->get_position() + dst);
      }
      platform.position = platform.entity->get_position();
    }

    // Collect map and entity boundaries.
    reset_boundaries(boundaries, alpha, platforms);
    
    // Update animations.
    for (auto entity : loaded_entities) {
      entity->update_animation(
        ultra::hash("collision"_h),
        boundaries
      );
    }

    // Update forces.
    state.walking = false;
    if (input.right && !input.left) {
      if (state.grounded) {
        force = ultra::geometry::Vector<float>::from_slope(
          state.floor.to_line().slope(),
          1.f
        );
        state.walking = true;
      } else {
        force.x = 1.f;
      }
    } else if (input.left && !input.right) {
      if (state.grounded) {
        force = ultra::geometry::Vector<float>::from_slope(
          state.floor.to_line().slope(),
          -1.f
        );
        state.walking = true;
      } else {
        force.x = -1.f;
      }
    } else if (state.grounded) {
      force = {0, 0};
    } else if (force.x) {
      force.x *= jump_momentum_loss_factor;
    }
    if (input.jump) {
      if ((state.grounded && state.jump_counter == 0 && !state.jump_reset)
          || state.jump_counter) {
        if (state.jump_counter == 0) {
          force = {0, 0};
          state.walking = false;
        }
        state.jump_counter++;
        state.jump_reset = true;
      }
      if (state.jump_counter && state.jump_counter <= jump_accel_frames) {
        force.y -= jump_accel / state.jump_counter;
      }
    } else {
      state.jump_counter = 0;
      state.jump_reset = true;
    }
    if (force.y + gravity_accel <= terminal_velocity) {
      force.y += gravity_accel;
    } else if (force.y < terminal_velocity) {
      force.y = terminal_velocity;
    }

    // Apply forces.
    state.grounded = false;
    ground_cross = std::numeric_limits<float>::infinity();
    auto original_force = force;
    ultra::geometry::Vector<float> abs_force = {
      std::abs(force.x),
      std::abs(force.y),
    };
    ultra::geometry::Vector<float> applied_force;
    do {
      collision = victor->get_boundary_collision(
        force,
        ultra::hash("collision"_h),
        boundaries
      );
      if (collision.first) {
        auto boundary = *collision.second.boundary;
        // Get cross product of boundary and force.
        auto cross = boundary.to_vector().unit().cross(force.unit());
        // Detect ground boundary.
        if (collision.second.edge == ultra::World::Collision::Edge::Bottom
            && state.jump_counter != 1) {
          // Skip one-way boundaries on an upward vector.
          bool skip = false;
          auto flags = collision.second.boundary->flags;
          if (cross < 0 && flags & ultra::World::Boundary::Flags::OneWay) {
            skip = true;
          } else {
            // Skip platforms on an upward vector.
            if (cross < 0) {
              for (const auto& platform : platforms) {
                if (platform.boundary == collision.second.boundary) {
                  skip = true;
                }
              }
            }
          }
          if (!skip) {
            state.grounded = true;
            if (std::abs(cross) < std::abs(ground_cross)) {
              ground_cross = cross;
              state.floor = *collision.second.boundary;
            }
          }
        }
        // Remove boundary from further collision detection in this loop.
        auto slope = boundary.to_line().slope();
        boundaries.erase(collision.second.boundary);
        // Adjust position.
        auto dst = collision.second.distance;
        if (std::abs(applied_force.x) + std::abs(dst.x) > abs_force.x) {
          dst.x = original_force.x - applied_force.x;
        }
        if (std::abs(applied_force.y) + std::abs(dst.y) > abs_force.y) {
          dst.y = original_force.y - applied_force.y;
        }
        victor->set_position(victor->get_position() + dst);
        applied_force += dst;
        // If player is standing still on ground, zero out the force vector.
        if (state.grounded && !state.walking && state.jump_counter != 1) {
          if (cross > 0) {
            force = {0, 0};
          }
          continue;
        }
        // Vertical boundaries always result in zero force on the x axis.
        if (cross > 0 && slope == std::numeric_limits<float>::infinity()) {
          if (state.grounded) {
            // On the ground, zero out the force to prevent "jumping" against
            // walls.
            force = {0, 0};
          } else {
            force = force.as_y();
          }
          continue;
        }
        // Horizontal boundaries always result in zero force on the y axis.
        if (cross > 0 && slope == 0) {
          force = force.as_x();
          continue;
        }
        // Perpendicular lines results in zero force.
        if (cross == 1) {
          force = {0, 0};
          continue;
        }
        // If force is not sticking to a floor and does not interact with
        // boundary, use same force.
        if (cross < 0 && !(boundary.p.x < boundary.q.x && state.walking)) {
          continue;
        }
        // If force is parallel, use same force.
        if (cross == 0) {
          continue;
        }
        // If the force is a ceiling, zero out the vertical dimension of the
        // force.
        if (boundary.p.x > boundary.q.x) {
          force.y = 0;
        }
        // Any other intersection results in a new force parallel to the
        // boundary.
        auto direction = force.length();
        if (force.x < 0 || (force.x == 0 && cross < 0)) {
          direction = -direction;
        }
        force = ultra::geometry::Vector<float>::from_slope(slope, direction);
        applied_force = {0, 0};
        abs_force = {
          std::abs(force.x),
          std::abs(force.y),
        };
        original_force = force;
      } else {
        auto dst = force;
        if (std::abs(applied_force.x) + std::abs(force.x) > abs_force.x) {
          dst.x = original_force.x - applied_force.x;
        }
        if (std::abs(applied_force.y) + std::abs(force.y) > abs_force.y) {
          dst.y = original_force.y - applied_force.y;
        }
        victor->set_position(victor->get_position() + dst);
        applied_force += dst;
      }
    } while (boundaries.size() && collision.first);

    // Update air state.
    if (state.grounded && state.jump_reset && !input.jump) {
      state.jump_reset = false;
    }

    reset_boundaries(boundaries, alpha, platforms);

    // Set animation.
    if (input.left && !input.right) {
      victor->sprite.attributes.flip_x = true;
    } else if (!input.left && input.right) {
      victor->sprite.attributes.flip_x = false;
    }
    if (state.grounded) {
      if (input.left ^ input.right) {
        victor->animate(
          ultra::hash("walk"_h),
          ultra::AnimatedSprite::Controls(),
          ultra::hash("collision"_h),
          boundaries
        );
      } else {
        victor->animate(
          ultra::hash("rest"_h),
          ultra::AnimatedSprite::Controls(),
          ultra::hash("collision"_h),
          boundaries
        );
      }
    } else if (input.jump
               && state.jump_counter <= jump_rise_frames
               && force.y < 0) {
      if (state.jump_counter) {
        victor->animate(
          ultra::hash("jump rise"_h),
          ultra::AnimatedSprite::Controls(),
          ultra::hash("collision"_h),
          boundaries
        );
      }
    } else {
      victor->animate(
        ultra::hash("jump fall"_h),
        ultra::AnimatedSprite::Controls::Builder().build(),
        ultra::hash("collision"_h),
        boundaries
      );
    }

    // Get new player position.
    victor_pos = victor->get_position()
      + victor->sprite.tileset.tile_size.as_x().as<int>() / 2
      - victor->sprite.tileset.tile_size.as_y().as<int>() / 2;

    // Set new camera position.
    camera_pos = get_camera_position(victor_pos, map.position, map.size);

    // Render frame.
    if (!printed_debug_text) {
      print_debug_text(victor_pos, map, state);
    }
    render(
      map,
      &loaded_entities[0],
      loaded_entities.size(),
      true
    );
  }

  // Free resources.
  const ultra::renderer::SpriteHandle* handles[loaded_entities.size()];
  for (size_t j = 0; j < loaded_entities.size(); j++) {
    handles[j] = loaded_entities[j]->get_sprites();
  }
  ultra::renderer::unload_sprites(handles, loaded_entities.size());
  ultra::renderer::unload_world();
  ultra::renderer::unload_tilesets(&victor_tileset_handle, 1);
  ultra::quit();
  bgfx::destroy(rendered_vertex_buffer);
  bgfx::destroy(frame_buffer);
  bgfx::destroy(tilesets_texture);
  bgfx::destroy(s_tex);
  bgfx::destroy(tile_program);
  bgfx::destroy(rt_program);
  bgfx::destroy(box_program);
  bgfx::shutdown();
  window.reset(nullptr);
  SDL_StopTextInput();
  SDL_Quit();
  return 0;
}
