#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <entt/entt.hpp>
#include "../components/components.hpp"

class InputSystem {
public:
    void update(entt::registry& registry);
    void process_event(const SDL_Event& event);

private:
    bool _left_mouse_down = false;
    bool _right_mouse_down = false;
    bool _space_down = false;
    float _mouse_x = 0.0f;
    float _mouse_y = 0.0f;
    float _last_mouse_x = 0.0f;
    float _last_mouse_y = 0.0f;
};
