#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/debug.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <SDL.h>
#include <SDL_syswm.h>
#include <thread>
#include <ultra240/ultra.h>
#include <wayland-egl.h>

namespace shader {
#include "shader/vs_pass.c"
#include "shader/fs_quad.c"
#include "shader/vs_box.c"
#include "shader/fs_line.c"
}

static bgfx::VertexLayout vertex_layout;
static bgfx::TextureHandle render_texture;
static bgfx::ProgramHandle draw_quad_program;
static bgfx::ProgramHandle draw_box_program;
static bgfx::VertexBufferHandle quad_vertex_buffer;
static bgfx::IndexBufferHandle quad_index_buffer;
static bgfx::DynamicVertexBufferHandle box_vertex_buffer;
static bgfx::UniformHandle v_ratio;
static bgfx::UniformHandle s_tex;
static bgfx::UniformHandle v_cam;
static bgfx::UniformHandle v_map;
static ultra::geometry::Vector<float> camera_pos;
static ultra::geometry::Vector<int16_t> map_pos;
static std::array<float, 4> ratio;
static bool render_collision_boxes = false;

struct PlayerState {
  bool walking;
  bool grounded;
  bool jump_reset;
  int jump_counter;
  ultra::geometry::LineSegment<float> floor;
};

static int dbg_line;
static int txt_top;
static int txt_left;
#define DEBUG(...) bgfx::dbgTextPrintf(txt_left, dbg_line++, 0x0e, __VA_ARGS__);

void print_debug_text(
  const ultra::geometry::Vector<float>& player_pos,
  const ultra::World::Map& map,
  const PlayerState& state
) {
  DEBUG("Move: arrow keys or directional pad");
  DEBUG("Jump: space bar or face button");
  DEBUG("Pause: <f>");
  DEBUG("Frame advance: <a> (while paused)");
  DEBUG("Show collision boxes: <b>");
  DEBUG("Quit: <q>");
  DEBUG("Player position: %s", player_pos.to_string().c_str());
  DEBUG("Camera position: %s", camera_pos.to_string().c_str());
  DEBUG("Map position: %s", map.position.to_string().c_str());
  if (state.grounded) {
    DEBUG("Ground: %s", state.floor.as<int>().to_string().c_str());
  } else {
    DEBUG("Ground: (nil)");
  }
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
  const ultra::Entity* entity;
  ultra::geometry::Vector<float> position;
  ultra::Tileset::Tile::CollisionBox collision_box;
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
    auto pos = platform.entity->get_collision_box_position(
      platform.collision_box
    );
    platform.boundary = boundaries.insert(
      boundaries.end(),
      ultra::World::Boundary(
        ultra::World::Boundary::Flags::OneWay,
        pos,
        pos + platform.collision_box.size.as_x()
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

void render(
  const ultra::World::Map& map,
  const ultra::Entity& victor,
  const ultra::geometry::Vector<float> victor_pos,
  const std::vector<ultra::Entity*>& entities,
  bool advance_time = true
) {
  // Render frame.
  ultra::renderer::render(advance_time);

  // Draw texture as quad.
  bgfx::setState(
    BGFX_STATE_PT_TRISTRIP
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_WRITE_Z
  );
  bgfx::setIndexBuffer(quad_index_buffer);
  bgfx::setVertexBuffer(0, quad_vertex_buffer);
  bgfx::setUniform(v_ratio, &ratio[0]);
  bgfx::setTexture(
    0,
    s_tex,
    render_texture,
    BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT
  );
  bgfx::submit(0, draw_quad_program);

  if (render_collision_boxes) {
    static std::vector<float> vertices;
    vertices.clear();
    vertices.reserve(8 * (1 + entities.size()));
    float camera_vals[] = {camera_pos.x, camera_pos.y, 0, 0};
    float map_vals[] = {
      map.position.as<float>().x,
      map.position.as<float>().y,
      0, 0,
    };
    const auto& boxes = victor.get_collision_boxes(ultra::hash("collision"_h));
    for (const auto& pair : boxes) {
      auto box = pair.second;
      auto pos = victor.get_collision_box_position(box);

      vertices.push_back(pos.x);
      vertices.push_back(pos.y);
      vertices.push_back(pos.x + box.size.x);
      vertices.push_back(pos.y);

      vertices.push_back(pos.x + box.size.x);
      vertices.push_back(pos.y);
      vertices.push_back(pos.x + box.size.x);
      vertices.push_back(pos.y + box.size.y);

      vertices.push_back(pos.x + box.size.x);
      vertices.push_back(pos.y + box.size.y);
      vertices.push_back(pos.x);
      vertices.push_back(pos.y + box.size.y);

      vertices.push_back(pos.x);
      vertices.push_back(pos.y + box.size.y);
      vertices.push_back(pos.x);
      vertices.push_back(pos.y);
    }
    for (auto& entity : entities) {
      if (entity->has_collision_boxes(ultra::hash("collision"_h))) {
        const auto& boxes = entity->get_collision_boxes(
          ultra::hash("collision"_h)
        );
        for (const auto& pair : boxes) {
          auto box = pair.second;
          auto pos = entity->get_collision_box_position(box);

          vertices.push_back(pos.x);
          vertices.push_back(pos.y);
          vertices.push_back(pos.x + box.size.x);
          vertices.push_back(pos.y);

          vertices.push_back(pos.x + box.size.x);
          vertices.push_back(pos.y);
          vertices.push_back(pos.x + box.size.x);
          vertices.push_back(pos.y + box.size.y);

          vertices.push_back(pos.x + box.size.x);
          vertices.push_back(pos.y + box.size.y);
          vertices.push_back(pos.x);
          vertices.push_back(pos.y + box.size.y);

          vertices.push_back(pos.x);
          vertices.push_back(pos.y + box.size.y);
          vertices.push_back(pos.x);
          vertices.push_back(pos.y);
        }
      }
    }
    bgfx::update(
      box_vertex_buffer,
      0,
      bgfx::makeRef(&vertices[0], sizeof(float) * vertices.size())
    );
    auto vertices_count = vertices.size() / 2;
    bgfx::setState(
      BGFX_STATE_PT_LINES
      | BGFX_STATE_WRITE_RGB
      | BGFX_STATE_WRITE_A
      | BGFX_STATE_WRITE_Z
    );
    bgfx::setUniform(v_cam, camera_vals);
    bgfx::setUniform(v_map, map_vals);
    bgfx::setUniform(v_ratio, &ratio[0]);
    bgfx::setVertexBuffer(0, box_vertex_buffer);
    bgfx::submit(1, draw_box_program);
  }

  // Draw to screen.
  bgfx::frame();
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
  float aspect = 16. / 15.;
  struct {
    float x, y;
  } draw_size;
  if (bounds.w > bounds.h) {
    draw_size.x = bounds.h * aspect;
    draw_size.y = bounds.h;
  } else {
    draw_size.x = bounds.w;
    draw_size.y = bounds.w / aspect;
  }
  txt_left = (bounds.w - draw_size.x) / 16 + 3;
  txt_top = (bounds.h - draw_size.y) / 32 + 1;

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

  // Create quad drawing program.
  draw_quad_program = bgfx::createProgram(
    bgfx::createShader(bgfx::makeRef(shader::vs_pass, sizeof(shader::vs_pass))),
    bgfx::createShader(bgfx::makeRef(shader::fs_quad, sizeof(shader::fs_quad))),
    true
  );
  v_ratio = bgfx::createUniform("v_ratio", bgfx::UniformType::Vec4);
  s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);

  // Create collision box drawing program.
  draw_box_program = bgfx::createProgram(
    bgfx::createShader(bgfx::makeRef(shader::vs_box, sizeof(shader::vs_box))),
    bgfx::createShader(bgfx::makeRef(shader::fs_line, sizeof(shader::fs_line))),
    true
  );
  v_cam = bgfx::createUniform("v_cam", bgfx::UniformType::Vec4);
  v_map = bgfx::createUniform("v_map", bgfx::UniformType::Vec4);

  // Create render texture.
  render_texture = bgfx::createTexture2D(
    256,
    240,
    false,
    1,
    bgfx::TextureFormat::RGBA8
  );
  bgfx::frame();

  // Define screen geometry.
  float x = draw_size.x / bounds.w;
  float y = draw_size.y / bounds.h;
  float vertices[] = {
    -x, -y,
    x, -y,
    -x,  y,
    x,  y,
  };
  uint16_t indices[] = {0, 1, 2, 3};
  ratio[0] = x;
  ratio[1] = y;

  // Define the common vertex layout.
  vertex_layout
    .begin()
    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
    .end();

  // Define the rendering quad vertex buffers.
  quad_vertex_buffer = bgfx::createVertexBuffer(
    bgfx::makeRef(vertices, sizeof(vertices)),
    vertex_layout
  );
  quad_index_buffer = bgfx::createIndexBuffer(
    bgfx::makeRef(indices, sizeof(indices))
  );

  // Define the collision box vertex buffer.
  box_vertex_buffer = bgfx::createDynamicVertexBuffer(
    0u,
    vertex_layout,
    BGFX_BUFFER_ALLOW_RESIZE
  );

  // Init ULTRA240.
  ultra::init("ultra240-example");
  bgfx::overrideInternal(render_texture, ultra::renderer::get_render_texture());

  // Load world.
  ultra::World alpha("alpha");
  ultra::renderer::load_world(alpha);
  ultra::World::Boundaries boundaries(
    ultra::VectorAllocator<ultra::World::Boundary>(1024)
  );

  // Declare stuff needed for loaded map entities.
  std::vector<const ultra::renderer::TilesetHandle*> map_tileset_handles;
  std::vector<std::shared_ptr<ultra::Entity>> map_entities;
  std::vector<ultra::Entity*> loaded_entities;
  std::vector<const ultra::renderer::EntityHandle*> loaded_entity_handles;
  std::vector<const ultra::Entity*> new_entities;
  std::list<Platform> platforms;
  ssize_t curr_map_index = -1;
  struct {
    struct {
      size_t min, max;
    } x, y;
  } entity_window;

  // Allocate enough storage for entities so that reallocations are avoided.
  loaded_entities.reserve(512);
  loaded_entity_handles.reserve(512);
  new_entities.reserve(512);

  // Load player tileset.
  ultra::Tileset victor_tileset("victor");
  auto victor_tileset_handle = ultra::renderer::load_tilesets(
    {&victor_tileset}
  );

  // Create player entity.
  std::shared_ptr<ultra::Entity> victor(
    ultra::Entity::from_tileset(
      alpha.get_boundaries(),
      victor_tileset,
      {32, 512}
    )
  );

  // Load player entity.
  auto victor_entity_handle = ultra::renderer::load_entities(
    {victor.get()},
    {victor_tileset_handle}
  );

  // Clear the view.
  bgfx::setViewRect(0, 0, 0, bounds.w, bounds.h);
  bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x000000ff);
  bgfx::setViewRect(1, 0, 0, bounds.w, bounds.h);
  bgfx::setViewClear(1, BGFX_CLEAR_DEPTH, 0x00000000);

  // Declare input struct.
  struct {
    bool up = false,
      down = false,
      left = false,
      right = false,
      jump = false;
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
  std::pair<bool, ultra::Entity::BoundaryCollision> collision;

  // Event loop.
  bool frame_advancing = false;
  while (true) {

    // Clear debug text buffer.
    bgfx::dbgTextClear(0);
    dbg_line = txt_top;

    // Determine map based on player's world position.
    auto victor_pos = victor->position
      + victor->tileset.tile_size.as_x().as<int>() / 2
      - victor->tileset.tile_size.as_y().as<int>() / 2;
    for (int i = 0; i < alpha.maps.size(); i++) {
      auto& map = alpha.maps[i];
      if (victor_pos.x >= 16 * map.position.x
          && victor_pos.y >= 16 * map.position.y
          && victor_pos.x < 16 * (map.position.x + map.size.x)
          && victor_pos.y < 16 * (map.position.y + map.size.y)) {
        if (curr_map_index != i) {
          curr_map_index = i;
          map_tileset_handles.clear();
          map_tileset_handles.push_back(
            ultra::renderer::load_map(curr_map_index)
          );
          platforms.clear();
          map_entities.clear();
          map_entities.resize(map.entities.size());
          ultra::renderer::unload_entities(loaded_entity_handles);
          loaded_entities.clear();
          loaded_entity_handles.clear();
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
    for (int i = entity_window.x.min; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto index = map.sorted_entities.x.max[i];
      auto& entity = map.entities[index];
      auto right = entity.position.x + entity.tileset->tile_size.x;
      auto top = entity.position.y - entity.tileset->tile_size.y;
      auto bottom = entity.position.y;
      if (right >= camera_pos.x) {
        if (i > 0) {
          entity_window.x.min--;
        }
        if (bottom >= camera_pos.y
            && top <= camera_pos.y + 240
            && map_entities[index] == nullptr) {
          map_entities[index].reset(ultra::Entity::from_map(
            alpha.get_boundaries(),
            entity
          ));
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the left.
    for (int i = entity_window.x.min; i < map.entities.size(); i++) {
      auto& entity = map.entities[map.sorted_entities.x.max[i]];
      auto right = entity.position.x + entity.tileset->tile_size.x;
      if (right < camera_pos.x) {
        entity_window.x.min++;
      } else {
        break;
      }
    }

    // Check for entities entering the window from the right.
    for (int i = entity_window.x.max; i < map.entities.size(); i++) {
      auto index = map.sorted_entities.x.min[i];
      auto& entity = map.entities[index];
      auto top = entity.position.y - entity.tileset->tile_size.y;;
      auto bottom = entity.position.y;
      auto left = entity.position.x;
      if (left <= camera_pos.x + 256) {
        entity_window.x.max++;
        if (bottom >= camera_pos.y
            && top <= camera_pos.y + 240
            && map_entities[index] == nullptr) {
          map_entities[index].reset(ultra::Entity::from_map(
            alpha.get_boundaries(),
            entity
          ));
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the right.
    for (int i = entity_window.x.max; i >= 0; i--) {
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
    for (int i = entity_window.y.max; i < map.entities.size(); i++) {
      auto index = map.sorted_entities.y.min[i];
      auto& entity = map.entities[index];
      auto top = entity.position.y - entity.tileset->tile_size.y;
      auto right = entity.position.x + entity.tileset->tile_size.x;
      auto left = entity.position.x;
      if (top <= camera_pos.y + 240) {
        entity_window.y.max++;
        if (right >= camera_pos.x
            && left <= camera_pos.x + 256
            && map_entities[index] == nullptr) {
          map_entities[index].reset(ultra::Entity::from_map(
            alpha.get_boundaries(),
            entity
          ));
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the bottom.
    for (int i = entity_window.y.max; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto& entity = map.entities[map.sorted_entities.y.min[i]];
      auto top = entity.position.y - entity.tileset->tile_size.y;
      if (top > camera_pos.y + 240) {
        if (i > 0) {
          entity_window.y.max--;
        }
      } else {
        break;
      }
    }

    // Check for entities entering the window from the top.
    for (int i = entity_window.y.min; i >= 0; i--) {
      if (i == map.entities.size()) {
        continue;
      }
      auto index = map.sorted_entities.y.max[i];
      auto& entity = map.entities[index];
      auto bottom = entity.position.y;
      auto right = entity.position.x + entity.tileset->tile_size.x;
      auto left = entity.position.x;
      if (bottom >= camera_pos.y) {
        if (i > 0) {
          entity_window.y.min--;
        }
        if (right >= camera_pos.x
            && left <= camera_pos.x + 256
            && map_entities[index] == nullptr) {
          map_entities[index].reset(ultra::Entity::from_map(
            alpha.get_boundaries(),
            entity
          ));
          auto entity = map_entities[index].get();
          loaded_entities.push_back(entity);
          new_entities.push_back(entity);
        }
      } else {
        break;
      }
    }

    // Check for entities leaving the window from the top.
    for (int i = entity_window.y.min; i < map.entities.size(); i++) {
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
      loaded_entity_handles.push_back(
        ultra::renderer::load_entities(new_entities, map_tileset_handles)
      );
      // Create boundaries for newly loaded platforms.
      for (const auto& entity : new_entities) {
        if (entity->has_collision_boxes(ultra::hash("boundary"_h))) {
          for (const auto& pair : entity->get_collision_boxes(
                 ultra::hash("boundary"_h)
               )) {
            if (pair.first == ultra::hash("platform"_h)) {
              platforms.push_back({
                entity,
                entity->position,
                pair.second,
                boundaries.cend(),
              });
            }
          }
        }
      }
    }

    // Poll for events.
    bool quit = false;
    bool advance = false;
    do {
      SDL_Event sdl_event;
      while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
        case SDL_QUIT:
          quit = true;
          break;
        case SDL_KEYDOWN:
          if (!sdl_event.key.repeat) {
            switch (sdl_event.key.keysym.sym) {
            case SDLK_q:
              quit = true;
              break;
            case SDLK_UP:
              input.up = true;
              break;
            case SDLK_DOWN:
              input.down = true;
              break;
            case SDLK_LEFT:
              input.left = true;
              break;
            case SDLK_RIGHT:
              input.right = true;
              break;
            case SDLK_SPACE:
              input.jump = true;
              break;
            }
            break;
          }
          break;
        case SDL_KEYUP:
          if (!sdl_event.key.repeat) {
            switch (sdl_event.key.keysym.sym) {
            case SDLK_UP:
              input.up = false;
              break;
            case SDLK_DOWN:
              input.down = false;
              break;
            case SDLK_LEFT:
              input.left = false;
              break;
            case SDLK_RIGHT:
              input.right = false;
              break;
            case SDLK_SPACE:
              input.jump = false;
              break;
            // Debugging keys.
            case SDLK_a:
              advance = true;
              break;
            case SDLK_b:
              render_collision_boxes = !render_collision_boxes;
              break;
            case SDLK_f:
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
          render(map, *victor, victor_pos, loaded_entities, false);
        }
      }
    } while (frame_advancing && !advance && !quit);

    if (quit) {
      break;
    }

    // Update map entities.
    for (auto entity : loaded_entities) {
      boundaries.clear();
      std::copy(
        alpha.get_boundaries().cbegin(),
        alpha.get_boundaries().cend(),
        std::back_inserter(boundaries)
      );
      entity->update(boundaries, victor.get(), loaded_entities);
    }

    // Collect map and entity boundaries.
    reset_boundaries(boundaries, alpha, platforms);
    
    // Update animations.
    victor->update_animation(ultra::hash("collision"_h), boundaries);
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

    // Apply platform forces.
    for (auto& platform : platforms) {
      auto dst = platform.entity->position - platform.position;
      auto pos = platform.entity->get_collision_box_position(
        platform.position,
        platform.collision_box
      );
      ultra::geometry::LineSegment<float> boundary(
        pos,
        pos + platform.collision_box.size.as_x()
      );
      if (state.grounded && state.floor == boundary) {
        victor->position += dst;
      }
      platform.position = platform.entity->position;
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
        victor->get_collision_boxes(ultra::hash("collision"_h)),
        boundaries
      );
      if (collision.first) {
        auto boundary = *collision.second.boundary;
        // Get cross product of boundary and force.
        auto cross = boundary.to_vector().unit().cross(force.unit());
        // Detect ground boundary.
        if (collision.second.edge == ultra::Entity::Collision::Edge::Bottom
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
        victor->position += dst;
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
        victor->position += dst;
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
      victor->attributes.flip_x = true;
    } else if (!input.left && input.right) {
      victor->attributes.flip_x = false;
    }
    if (state.grounded) {
      if (input.left ^ input.right) {
        victor->animate(
          ultra::hash("collision"_h),
          boundaries,
          {ultra::hash("walk"_h)}
        );
      } else {
        victor->animate(
          ultra::hash("collision"_h),
          boundaries,
          {ultra::hash("rest"_h)}
        );
      }
    } else if (input.jump
               && state.jump_counter <= jump_rise_frames
               && force.y < 0) {
      if (state.jump_counter) {
        victor->animate(
          ultra::hash("collision"_h),
          boundaries,
          {ultra::hash("jump rise"_h)}
        );
      }
    } else {
      victor->animate(
        ultra::hash("collision"_h),
        boundaries,
        ultra::Entity::AnimationControls::Builder()
        .name(ultra::hash("jump fall"_h))
        .build()
      );
    }

    // Get new player position.
    victor_pos = victor->position
      + victor->tileset.tile_size.as_x().as<int>() / 2
      - victor->tileset.tile_size.as_y().as<int>() / 2;

    // Set new camera position.
    camera_pos = get_camera_position(victor_pos, map.position, map.size);
    ultra::renderer::set_camera_position(camera_pos);

    // Render frame.
    print_debug_text(victor_pos, map, state);
    render(map, *victor, victor_pos, loaded_entities);
  }

  // Free resources.
  ultra::renderer::unload_world();
  ultra::quit();
  bgfx::destroy(box_vertex_buffer);
  bgfx::destroy(quad_index_buffer);
  bgfx::destroy(quad_vertex_buffer);
  bgfx::destroy(render_texture);
  bgfx::destroy(v_ratio);
  bgfx::destroy(s_tex);
  bgfx::destroy(v_map);
  bgfx::destroy(v_cam);
  bgfx::destroy(draw_quad_program);
  bgfx::destroy(draw_box_program);
  bgfx::shutdown();
  window.reset(nullptr);
  SDL_StopTextInput();
  SDL_Quit();
  return 0;
}
