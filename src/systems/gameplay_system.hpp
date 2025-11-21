#pragma once

#include <entt/entt.hpp>
#include "../components/components.hpp"

class SpatialGrid;

class GameplaySystem {
public:
	GameplaySystem(SpatialGrid& spatial_grid) : _spatial_grid(spatial_grid) {}

	// Update all gameplay systems
	void update(entt::registry& registry, float dt);

private:
	// Individual system updates
	void update_movement(entt::registry& registry, float dt);
	void update_targeting(entt::registry& registry, float dt);
	void update_melee_combat(entt::registry& registry, float dt);
	void update_ranged_combat(entt::registry& registry, float dt);
	void update_healer(entt::registry& registry, float dt);
	void update_projectiles(entt::registry& registry, float dt);
	void update_death(entt::registry& registry, float dt);

	SpatialGrid& _spatial_grid;
	float _targeting_timer = 0.0f;
	const float _targeting_interval = 1.0f; // Run targeting every second
};

