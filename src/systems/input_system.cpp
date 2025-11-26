#include "input_system.hpp"
#include "../components/components.hpp"
#include "../utils/profiler.hpp"
#include "../world/world.hpp"
#include <iostream>
#include <cmath>
#include <limits>
#include <vector>

void InputSystem::process_event(const SDL_Event& event) {
	ZoneScopedN("InputSystem::process_event");
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
		if (event.key.key == SDLK_M) _m_down = true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.key == SDLK_SPACE) _space_down = false;
		if (event.key.key == SDLK_S) _s_down = false;
		if (event.key.key == SDLK_D) _d_down = false;
		if (event.key.key == SDLK_M) _m_down = false;
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        _scroll_delta += event.wheel.y;
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

void InputSystem::issue_move_command(entt::registry& registry, const Vec2& click_world_pos) {
	// Get all selected units
	auto selected_view = registry.view<Selected, Position, Movement>();
	
	// Calculate bounding box of selected units
	float min_x = std::numeric_limits<float>::max();
	float max_x = std::numeric_limits<float>::lowest();
	float min_y = std::numeric_limits<float>::max();
	float max_y = std::numeric_limits<float>::lowest();
	
	std::vector<entt::entity> selected_units;
	for (auto entity : selected_view) {
		const auto& pos = selected_view.get<Position>(entity);
		selected_units.push_back(entity);
		
		min_x = std::min(min_x, pos.value.x);
		max_x = std::max(max_x, pos.value.x);
		min_y = std::min(min_y, pos.value.y);
		max_y = std::max(max_y, pos.value.y);
	}
	
	// Early return if no units selected
	if (selected_units.empty()) {
		return;
	}
	
	// Calculate center of bounding box
	Vec2 bounding_box_center = {
		(min_x + max_x) * 0.5f,
		(min_y + max_y) * 0.5f
	};
	
	// For each selected unit, calculate target position preserving formation
	for (auto entity : selected_units) {
		const auto& pos = selected_view.get<Position>(entity);
		auto& movement = selected_view.get<Movement>(entity);
		
		// Calculate offset from bounding box center
		Vec2 offset = pos.value - bounding_box_center;
		
		// Set target position = click position + offset
		movement.MoveTo(pos.value, click_world_pos + offset);
	}
}

void InputSystem::update(World& world, float dt) {
	ZoneScopedN("InputSystem::update");
	entt::registry& registry = world.GetRegistry();
	SpatialGrid& spatial_grid = world.GetSpatialGrid();
	
    // Get Camera
    auto cam_view = registry.view<Camera, MainCamera>();
	Camera camera = {Vec2{0.0f, 0.0f}, 1.0f};
	
	for (auto entity : cam_view) {
        camera = cam_view.get<Camera>(entity);

        // Pan logic (Space + Mouse Move)
        if (_right_mouse_down && !_is_dragging) {
            float dx = _mouse_x - _last_mouse_x;
            float dy = _mouse_y - _last_mouse_y;
            camera.offset -= Vec2{dx / camera.zoom, -dy / camera.zoom};
			cam_view.get<Camera>(entity) = camera;
        }
		
		// Zoom logic (Mouse Scroll) - zoom centered at mouse position
		if (_scroll_delta != 0.0f) {
			// Get world position under mouse BEFORE zoom change
			Vec2 world_pos_before = screen_to_world(_mouse_x, _mouse_y, camera, _screen_width, _screen_height);
			
			// Apply zoom (multiply by factor per scroll step)
			float zoom_factor = 1.1f;
			if (_scroll_delta > 0) {
				camera.zoom *= std::pow(zoom_factor, _scroll_delta);
			} else {
				camera.zoom *= std::pow(1.0f / zoom_factor, -_scroll_delta);
			}
			
			// Clamp zoom to reasonable range
			camera.zoom = std::max(0.1f, std::min(10.0f, camera.zoom));
			
			// Get world position under mouse AFTER zoom change (with old offset)
			Vec2 world_pos_after = screen_to_world(_mouse_x, _mouse_y, camera, _screen_width, _screen_height);
			
			// Adjust offset so the original world position stays under the mouse
			camera.offset += world_pos_before - world_pos_after;
			
			cam_view.get<Camera>(entity) = camera;
			_scroll_delta = 0.0f;
		}
		break;
    }
	
	// Update selection rect in world space
	if (_is_dragging) {
		_selection_start = screen_to_world(_drag_start_screen.x, _drag_start_screen.y, camera, _screen_width, _screen_height);
		_selection_end = screen_to_world(_mouse_x, _mouse_y, camera, _screen_width, _screen_height);
	}
	
	// Handle mouse up - finalize selection/spawn/delete/move
	static bool was_dragging = false;
	static bool was_left_mouse_down = false;
	
	if (was_dragging && !_is_dragging) {
		// Mouse was just released
		
		// Check for M + Click (move command) - only if it was a click, not a drag
		if (_m_down) {
			float drag_distance = Vec2::distance(_drag_start_screen, Vec2{_mouse_x, _mouse_y});
			// If drag distance is small, treat it as a click
			if (drag_distance < 5.0f) {
				Vec2 click_world_pos = screen_to_world(_mouse_x, _mouse_y, camera, _screen_width, _screen_height);
				issue_move_command(registry, click_world_pos);
				was_dragging = _is_dragging;
				was_left_mouse_down = _left_mouse_down;
				_last_mouse_x = _mouse_x;
				_last_mouse_y = _mouse_y;
				return;
			}
		}
		
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
						world.SpawnUnit(_spawn_type, _spawn_faction, spawn_pos);
						spawned++;
					}
				}
			}
		} else if (_d_down) {
			// Delete units in rect
			spatial_grid.QueryRect(rect_min, rect_max, [&](entt::entity entity) {
				if (registry.valid(entity)) {
					// Remove from spatial grid before destroying
					if (registry.all_of<SpatialNode>(entity)) {
						spatial_grid.Remove(entity);
					}
					registry.destroy(entity);
				}
			});
		} else {
			// Normal selection
			// First, clear existing selections
			auto selected_view = registry.view<Selected>();
			for (auto entity : selected_view) {
				registry.remove<Selected>(entity);
			}
			
			// Add selection to entities in rect
			spatial_grid.QueryRect(rect_min, rect_max, [&](entt::entity entity) {
				if (registry.valid(entity) && registry.all_of<Unit>(entity)) {
					registry.emplace_or_replace<Selected>(entity);
				}
			});
		}
	}
	was_dragging = _is_dragging;
	was_left_mouse_down = _left_mouse_down;
    
    _last_mouse_x = _mouse_x;
    _last_mouse_y = _mouse_y;
}
