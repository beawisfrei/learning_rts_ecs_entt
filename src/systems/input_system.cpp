#include "input_system.hpp"
#include "../components/components.hpp"
#include "../utils/spatial_grid.hpp"
#include "../utils/unit_factory.hpp"
#include "../utils/math_utils.hpp"
#include <iostream>
#include <cmath>

void InputSystem::process_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) {
			_left_mouse_down = true;
			_is_dragging = true;
			_drag_start_screen = Vec2{_mouse_x, _mouse_y};
		}
        if (event.button.button == SDL_BUTTON_RIGHT) _right_mouse_down = true;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event.button.button == SDL_BUTTON_LEFT) {
			_left_mouse_down = false;
			_is_dragging = false;
		}
        if (event.button.button == SDL_BUTTON_RIGHT) _right_mouse_down = false;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        _last_mouse_x = _mouse_x;
        _last_mouse_y = _mouse_y;
        _mouse_x = event.motion.x;
        _mouse_y = event.motion.y;
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_SPACE) _space_down = true;
		if (event.key.key == SDLK_S) _s_down = true;
		if (event.key.key == SDLK_D) _d_down = true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.key == SDLK_SPACE) _space_down = false;
		if (event.key.key == SDLK_S) _s_down = false;
		if (event.key.key == SDLK_D) _d_down = false;
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        // Handle Zoom...
    }
}

Vec2 InputSystem::screen_to_world(float screen_x, float screen_y, const Camera& camera, int screen_width, int screen_height) {
	// Convert screen coordinates to normalized device coordinates
	float ndc_x = (screen_x / screen_width) * 2.0f - 1.0f;
	float ndc_y = -((screen_y / screen_height) * 2.0f - 1.0f); // Y is flipped in screen space
	
	// Convert NDC to world coordinates (reverse of vertex shader transform)
	// From shader: vec2 ndc = worldPos / vec2(640.0, 360.0);
	// worldPos = (scaledPos + uObjPos - uOffset) * uZoom
	// So: world = ndc * vec2(640, 360) / zoom + offset
	
	float world_x = (ndc_x * 640.0f) / camera.zoom + camera.offset.x;
	float world_y = (ndc_y * 360.0f) / camera.zoom + camera.offset.y;
	
	return Vec2{world_x, world_y};
}

void InputSystem::update(entt::registry& registry, SpatialGrid& spatial_grid, UnitFactory& unit_factory, float dt) {
    // Get Camera
    auto cam_view = registry.view<Camera, MainCamera>();
	Camera camera = {Vec2{0.0f, 0.0f}, 1.0f};
	int screen_width = 1280;
	int screen_height = 720;
	
	for (auto entity : cam_view) {
        camera = cam_view.get<Camera>(entity);

        // Pan logic (Space + Mouse Move)
        if (_space_down && !_is_dragging) {
            float dx = _mouse_x - _last_mouse_x;
            float dy = _mouse_y - _last_mouse_y;
            camera.offset.x -= dx / camera.zoom;
            camera.offset.y += dy / camera.zoom;
			cam_view.get<Camera>(entity) = camera;
        }
		break;
    }
	
	// Update selection rect in world space
	if (_is_dragging) {
		_selection_start = screen_to_world(_drag_start_screen.x, _drag_start_screen.y, camera, screen_width, screen_height);
		_selection_end = screen_to_world(_mouse_x, _mouse_y, camera, screen_width, screen_height);
	}
	
	// Handle mouse up - finalize selection/spawn/delete
	static bool was_dragging = false;
	if (was_dragging && !_is_dragging) {
		// Mouse was just released
		Vec2 rect_min = {
			std::min(_selection_start.x, _selection_end.x),
			std::min(_selection_start.y, _selection_end.y)
		};
		Vec2 rect_max = {
			std::max(_selection_start.x, _selection_end.x),
			std::max(_selection_start.y, _selection_end.y)
		};
		
		// Check modifiers
		if (_s_down) {
			// Spawn units in grid formation
			float rect_width = rect_max.x - rect_min.x;
			float rect_height = rect_max.y - rect_min.y;
			
			if (rect_width > 0.1f && rect_height > 0.1f) {
				int grid_size = static_cast<int>(std::sqrt(_spawn_count)) + 1;
				float spacing_x = rect_width / grid_size;
				float spacing_y = rect_height / grid_size;
				
				int spawned = 0;
				for (int y = 0; y <= grid_size && spawned < _spawn_count; ++y) {
					for (int x = 0; x <= grid_size && spawned < _spawn_count; ++x) {
						Vec2 spawn_pos = {
							rect_min.x + x * spacing_x,
							rect_min.y + y * spacing_y
						};
						unit_factory.spawn_unit(registry, _spawn_type, _spawn_faction, spawn_pos);
						spawned++;
					}
				}
			}
		} else if (_d_down) {
			// Delete units in rect
			auto entities = spatial_grid.query_rect(rect_min, rect_max);
			for (auto entity : entities) {
				if (registry.valid(entity)) {
					registry.destroy(entity);
				}
			}
		} else {
			// Normal selection
			// First, clear existing selections
			auto selected_view = registry.view<Selected>();
			for (auto entity : selected_view) {
				registry.remove<Selected>(entity);
			}
			
			// Add selection to entities in rect
			auto entities = spatial_grid.query_rect(rect_min, rect_max);
			for (auto entity : entities) {
				if (registry.valid(entity) && registry.all_of<Unit>(entity)) {
					registry.emplace_or_replace<Selected>(entity);
				}
			}
		}
	}
	was_dragging = _is_dragging;
    
    _last_mouse_x = _mouse_x;
    _last_mouse_y = _mouse_y;
}
