#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <entt/entt.hpp>
#include "../components/components.hpp"

class World;

class InputSystem {
public:
    void update(World& world, float dt);
    void process_event(const SDL_Event& event);
	
	// Get selection rect for rendering
	bool is_selecting() const { return _is_dragging; }
	Vec2 get_selection_start() const { return _selection_start; }
	Vec2 get_selection_end() const { return _selection_end; }
	
	// Set spawn parameters from UI
	void set_spawn_params(UnitType type, int faction, int count) {
		_spawn_type = type;
		_spawn_faction = faction;
		_spawn_count = count;
	}
	
	// Set screen dimensions from config
	void set_screen_dimensions(int width, int height) {
		_screen_width = width;
		_screen_height = height;
	}

private:
	Vec2 screen_to_world(float screen_x, float screen_y, const Camera& camera, int screen_width, int screen_height);
	void issue_move_command(entt::registry& registry, const Vec2& click_world_pos);
	
    bool _left_mouse_down = false;
    bool _right_mouse_down = false;
    bool _space_down = false;
	bool _s_down = false;
	bool _d_down = false;
	bool _m_down = false;
	
    float _mouse_x = 0.0f;
    float _mouse_y = 0.0f;
    float _last_mouse_x = 0.0f;
    float _last_mouse_y = 0.0f;
	
	// Selection tracking
	bool _is_dragging = false;
	Vec2 _selection_start = {0.0f, 0.0f};
	Vec2 _selection_end = {0.0f, 0.0f};
	Vec2 _drag_start_screen = {0.0f, 0.0f};
	
	// Spawn parameters
	UnitType _spawn_type = UnitType::Footman;
	int _spawn_faction = 0;
	int _spawn_count = 10;
	
	// Screen dimensions
	int _screen_width = 1280;
	int _screen_height = 720;
};
