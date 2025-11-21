#include "input_system.hpp"
#include "../components/components.hpp"
#include <iostream>

void InputSystem::process_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        if (event.button.button == SDL_BUTTON_LEFT) _left_mouse_down = true;
        if (event.button.button == SDL_BUTTON_RIGHT) _right_mouse_down = true;
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        if (event.button.button == SDL_BUTTON_LEFT) _left_mouse_down = false;
        if (event.button.button == SDL_BUTTON_RIGHT) _right_mouse_down = false;
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        _last_mouse_x = _mouse_x;
        _last_mouse_y = _mouse_y;
        _mouse_x = event.motion.x;
        _mouse_y = event.motion.y;
    } else if (event.type == SDL_EVENT_KEY_DOWN) {
        if (event.key.key == SDLK_SPACE) _space_down = true;
    } else if (event.type == SDL_EVENT_KEY_UP) {
        if (event.key.key == SDLK_SPACE) _space_down = false;
    } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        // Handle Zoom...
    }
}

void InputSystem::update(entt::registry& registry) {
    // Pan Camera (Space + Mouse Move)
    // We need to find the camera entity
    auto view = registry.view<Camera, MainCamera>();
    for (auto entity : view) {
        auto& camera = view.get<Camera>(entity);

        // Pan logic
        if (_space_down) {
            float dx = _mouse_x - _last_mouse_x;
            float dy = _mouse_y - _last_mouse_y;
            camera.offset.x -= dx / camera.zoom;
            camera.offset.y += dy / camera.zoom;
        }
    }
    
    _last_mouse_x = _mouse_x;
    _last_mouse_y = _mouse_y;
}
