#include "gameplay_system.hpp"
#include "../world/spatial_grid.hpp"
#include <iostream>

void GameplaySystem::update(entt::registry& registry, float dt) {
	update_movement(registry, dt);
	update_targeting(registry, dt);
	update_melee_combat(registry, dt);
	update_ranged_combat(registry, dt);
	update_healer(registry, dt);
	update_projectiles(registry, dt);
	update_death(registry, dt);
}

void GameplaySystem::update_movement(entt::registry& registry, float dt) {
	auto view = registry.view<Movement, Position>();
	
	for (auto entity : view) {
		auto& movement = view.get<Movement>(entity);
		auto& pos = view.get<Position>(entity);
		
		// Store old position for grid update
		Vec2 old_pos = pos.value;
		
		// Only move if state is Moving
		if (movement.state != MovementState::Moving) {
			continue;
		}
		
		// Calculate direction to target
		Vec2 dir = Vec2::direction_to(pos.value, movement.target);
		
		// Update velocity
		movement.velocity = dir * movement.speed;
		
		// Update position
		pos.value += movement.velocity * dt;
		
		// Update spatial grid if entity has SpatialNode
		if (registry.all_of<SpatialNode>(entity)) {
			_spatial_grid.Update(entity, old_pos, pos.value);
		}
		
		// Check if reached target
		float dist = Vec2::distance(pos.value, movement.target);
		if (dist < 0.5f) {
			// Reached target, stop moving
			movement.state = MovementState::NotMoving;
			movement.velocity = Vec2{0.0f, 0.0f};
		}
	}
}

void GameplaySystem::update_targeting(entt::registry& registry, float dt) {
	_targeting_timer += dt;
	
	// Only run targeting periodically
	if (_targeting_timer < _targeting_interval) {
		return;
	}
	_targeting_timer = 0.0f;

	// Update targets for units with DirectDamage (melee units)
	auto attack_view = registry.view<AttackTarget, Position, Faction, DirectDamage>();
	for (auto entity : attack_view) {
		auto& target_comp = attack_view.get<AttackTarget>(entity);
		const auto& pos = attack_view.get<Position>(entity);
		const auto& faction = attack_view.get<Faction>(entity);
		const auto& damage = attack_view.get<DirectDamage>(entity);

		// Check if current target is still valid
		bool need_new_target = false;
		if (target_comp.target == entt::null || !registry.valid(target_comp.target)) {
			need_new_target = true;
		} else {
			// Check if target is alive and in range
			if (registry.all_of<Health, Position>(target_comp.target)) {
				const auto& target_health = registry.get<Health>(target_comp.target);
				const auto& target_pos = registry.get<Position>(target_comp.target);
				
				if (target_health.current <= 0) {
					need_new_target = true;
				} else {
					float dist = Vec2::distance(pos.value, target_pos.value);
					if (dist > damage.range) {
						need_new_target = true;
					}
				}
			} else {
				need_new_target = true;
			}
		}

		// Find new target if needed
		if (need_new_target) {
			entt::entity new_target = _spatial_grid.FindNearest(pos.value, damage.range, faction.id, false);
			target_comp.target = new_target;
		}
		
		// Update movement state based on target
		if (registry.all_of<Movement>(entity)) {
			auto& movement = registry.get<Movement>(entity);
			if (target_comp.target != entt::null && registry.valid(target_comp.target)) {
				// Has valid target, pause movement
				if (movement.state == MovementState::Moving) {
					movement.state = MovementState::Paused;
				}
			} else {
				// No valid target, resume movement if paused
				if (movement.state == MovementState::Paused) {
					movement.state = MovementState::Moving;
				}
			}
		}
	}

	// Update targets for ranged units (ProjectileEmitter)
	auto ranged_view = registry.view<AttackTarget, Position, Faction, ProjectileEmitter>();
	for (auto entity : ranged_view) {
		auto& target_comp = ranged_view.get<AttackTarget>(entity);
		const auto& pos = ranged_view.get<Position>(entity);
		const auto& faction = ranged_view.get<Faction>(entity);
		const auto& emitter = ranged_view.get<ProjectileEmitter>(entity);

		// Check if current target is still valid
		bool need_new_target = false;
		if (target_comp.target == entt::null || !registry.valid(target_comp.target)) {
			need_new_target = true;
		} else {
			// Check if target is alive and in range
			if (registry.all_of<Health, Position>(target_comp.target)) {
				const auto& target_health = registry.get<Health>(target_comp.target);
				const auto& target_pos = registry.get<Position>(target_comp.target);
				
				if (target_health.current <= 0) {
					need_new_target = true;
				} else {
					float dist = Vec2::distance(pos.value, target_pos.value);
					if (dist > emitter.range) {
						need_new_target = true;
					}
				}
			} else {
				need_new_target = true;
			}
		}

		// Find new target if needed
		if (need_new_target) {
			entt::entity new_target = _spatial_grid.FindNearest(pos.value, emitter.range, faction.id, false);
			target_comp.target = new_target;
		}
		
		// Update movement state based on target
		if (registry.all_of<Movement>(entity)) {
			auto& movement = registry.get<Movement>(entity);
			if (target_comp.target != entt::null && registry.valid(target_comp.target)) {
				// Has valid target, pause movement
				if (movement.state == MovementState::Moving) {
					movement.state = MovementState::Paused;
				}
			} else {
				// No valid target, resume movement if paused
				if (movement.state == MovementState::Paused) {
					movement.state = MovementState::Moving;
				}
			}
		}
	}
	
	// Update movement for entities with AttackTarget but without DirectDamage or ProjectileEmitter
	// This handles any other unit types that might have attack targets but different combat components
	auto movement_view = registry.view<Movement, AttackTarget>(entt::exclude<DirectDamage, ProjectileEmitter>);
	for (auto entity : movement_view) {
		auto& movement = movement_view.get<Movement>(entity);
		const auto& target_comp = movement_view.get<AttackTarget>(entity);
		
		// Update movement state based on target
		if (target_comp.target != entt::null && registry.valid(target_comp.target)) {
			// Has valid target, pause movement
			if (movement.state == MovementState::Moving) {
				movement.state = MovementState::Paused;
			}
		} else {
			// No valid target, resume movement if paused
			if (movement.state == MovementState::Paused) {
				movement.state = MovementState::Moving;
			}
		}
	}
}

void GameplaySystem::update_melee_combat(entt::registry& registry, float dt) {
	auto view = registry.view<DirectDamage, AttackTarget, Position, Faction>();
	
	for (auto entity : view) {
		auto& damage_comp = view.get<DirectDamage>(entity);
		const auto& target_comp = view.get<AttackTarget>(entity);
		const auto& pos = view.get<Position>(entity);

		// Update cooldown timer
		damage_comp.timer += dt;

		// Check if can attack
		if (damage_comp.timer >= damage_comp.cooldown) {
			// Check if has valid target
			if (target_comp.target != entt::null && registry.valid(target_comp.target)) {
				if (registry.all_of<Health, Position>(target_comp.target)) {
					const auto& target_pos = registry.get<Position>(target_comp.target);
					float dist = Vec2::distance(pos.value, target_pos.value);

					// Check if in range
					if (dist <= damage_comp.range) {
						// Deal damage
						auto& target_health = registry.get<Health>(target_comp.target);
						float actual_damage = damage_comp.damage - target_health.shield;
						if (actual_damage > 0) {
							target_health.current -= actual_damage;
						}

						// Reset timer
						damage_comp.timer = 0.0f;
					}
				}
			}
		}
	}
}

void GameplaySystem::update_ranged_combat(entt::registry& registry, float dt) {
	auto view = registry.view<ProjectileEmitter, AttackTarget, Position, Faction>();
	
	for (auto entity : view) {
		auto& emitter = view.get<ProjectileEmitter>(entity);
		const auto& target_comp = view.get<AttackTarget>(entity);
		const auto& pos = view.get<Position>(entity);
		const auto& faction = view.get<Faction>(entity);

		// Update cooldown timer
		emitter.timer += dt;

		// Check if can fire
		if (emitter.timer >= emitter.cooldown) {
			// Check if has valid target
			if (target_comp.target != entt::null && registry.valid(target_comp.target)) {
				if (registry.all_of<Position>(target_comp.target)) {
					const auto& target_pos = registry.get<Position>(target_comp.target);
					float dist = Vec2::distance(pos.value, target_pos.value);

					// Check if in range
					if (dist <= emitter.range) {
						// Spawn projectile
						auto projectile = registry.create();
						registry.emplace<Position>(projectile, pos.value);
						
						bool is_aoe = (emitter.projectile_type == 1);
						registry.emplace<Projectile>(projectile, 
							emitter.damage, 
							faction.id, 
							is_aoe, 
							emitter.aoe_radius
						);

						// Add Movement component for projectile
						registry.emplace<Movement>(projectile, 
							Vec2{0.0f, 0.0f},           // velocity (will be calculated by movement system)
							target_pos.value,            // target
							emitter.projectile_speed,    // speed
							MovementState::Moving        // state = Moving
						);

						// For rendering - create a simple visual
						// We'll use a small unit-like sprite
						registry.emplace<Unit>(projectile, UnitType::Footman, faction.id); // Placeholder type

						// Reset timer
						emitter.timer = 0.0f;
					}
				}
			}
		}
	}
}

void GameplaySystem::update_healer(entt::registry& registry, float dt) {
	auto view = registry.view<Healer, Position, Faction>();
	
	for (auto entity : view) {
		auto& healer = view.get<Healer>(entity);
		const auto& pos = view.get<Position>(entity);
		const auto& faction = view.get<Faction>(entity);

		// Update cooldown timer
		healer.timer += dt;

		// Check if can heal
		if (healer.timer >= healer.cooldown) {
			// Find all allies in range
			bool found_allies = false;
			_spatial_grid.QueryRadius(pos.value, healer.range, [&](entt::entity ally) {
				if (registry.valid(ally) && registry.all_of<Health>(ally)) {
					auto& health = registry.get<Health>(ally);
					// Only heal if not at full health
					if (health.current < health.max) {
						health.current += healer.heal_amount;
						if (health.current > health.max) {
							health.current = health.max;
						}
						found_allies = true;
					}
				}
			}, faction.id, true);

			// Reset timer if we found allies to heal
			if (found_allies) {
				healer.timer = 0.0f;
			}
		}
	}
}

void GameplaySystem::update_projectiles(entt::registry& registry, float dt) {
	auto view = registry.view<Projectile, Position, Movement>();
	
	std::vector<entt::entity> to_destroy;

	for (auto entity : view) {
		auto& projectile = view.get<Projectile>(entity);
		const auto& pos = view.get<Position>(entity);
		const auto& movement = view.get<Movement>(entity);

		// Check if reached target (movement system handles actual movement)
		// Movement system stops movement when dist < 0.5f, so we check if state is NotMoving
		if (movement.state == MovementState::NotMoving) {
			// Projectile hit
			if (projectile.is_aoe) {
				// AOE damage
				_spatial_grid.QueryRadius(pos.value, projectile.aoe_radius, [&](entt::entity enemy) {
					if (registry.valid(enemy) && registry.all_of<Health>(enemy)) {
						auto& health = registry.get<Health>(enemy);
						float actual_damage = projectile.damage - health.shield;
						if (actual_damage > 0) {
							health.current -= actual_damage;
						}
					}
				}, projectile.faction, false);
			} else {
				// Single target damage - find nearest enemy at impact point
				entt::entity target = _spatial_grid.FindNearest(pos.value, 1.0f, projectile.faction, false);
				if (target != entt::null && registry.valid(target)) {
					if (registry.all_of<Health>(target)) {
						auto& health = registry.get<Health>(target);
						float actual_damage = projectile.damage - health.shield;
						if (actual_damage > 0) {
							health.current -= actual_damage;
						}
					}
				}
			}

			// Mark for destruction
			to_destroy.push_back(entity);
		}
	}

	// Destroy projectiles that hit
	for (auto entity : to_destroy) {
		if (registry.valid(entity)) {
			registry.destroy(entity);
		}
	}
}

void GameplaySystem::update_death(entt::registry& registry, float dt) {
	auto view = registry.view<Health>();
	
	std::vector<entt::entity> to_destroy;

	for (auto entity : view) {
		const auto& health = view.get<Health>(entity);
		
		if (health.current <= 0) {
			// Remove from spatial grid before destroying
			if (registry.all_of<SpatialNode>(entity)) {
				_spatial_grid.Remove(entity);
			}
			to_destroy.push_back(entity);
		}
	}

	registry.destroy(to_destroy.begin(), to_destroy.end());
}

