#include "gameplay_system.hpp"
#include "../world/spatial_grid.hpp"
#include "../utils/profiler.hpp"
#include <iostream>

void GameplaySystem::update(entt::registry& registry, float dt) {
	ZoneScopedNC("GameplaySystem::update", TRACY_COLOR_UPDATE);
	if (dt == 0.0f)
		return;
	
	update_movement(registry, dt);
	update_follow(registry, dt);
	update_targeting(registry, dt);
	update_melee_combat(registry, dt);
	update_ranged_combat(registry, dt);
	update_healer(registry, dt);
	update_projectiles(registry, dt);
	update_death(registry, dt);
}

void GameplaySystem::update_movement(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_movement");
	
	// Process entities with SpatialNode (need grid update)
	auto view_with_spatial = registry.view<Movement, Position, SpatialNode>(entt::exclude<StateAttackingTag>);
	for (auto entity : view_with_spatial) {
		auto& movement = view_with_spatial.get<Movement>(entity);
		auto& pos = view_with_spatial.get<Position>(entity);
		
		Vec2 old_pos = pos.value;
		if (processMovement(movement, pos, dt)) {
			_spatial_grid.Update(entity, old_pos, pos.value);
		}
	}
	
	// Process entities without SpatialNode (e.g. projectiles)
	auto view_without_spatial = registry.view<Movement, Position>(entt::exclude<StateAttackingTag, SpatialNode>);
	for (auto entity : view_without_spatial) {
		auto& movement = view_without_spatial.get<Movement>(entity);
		auto& pos = view_without_spatial.get<Position>(entity);
		
		processMovement(movement, pos, dt);
	}
}

bool GameplaySystem::processMovement(Movement& movement, Position& pos, float dt) {
	if (movement.velocity.isZero()) {
		return false; // Not moving
	}
	
	// Store old position for overshoot check
	Vec2 old_pos = pos.value;
			
	// Update position
	pos.value += movement.velocity * dt;
	
	// If reached target or overshot, stop at target
	float dist = Vec2::distance(pos.value, movement.target);
	if (dist < 0.5f || Vec2::dot(movement.target - old_pos, movement.velocity) < 0.0f) {
		// Reached target, stop moving
		movement.velocity = Vec2{0.0f, 0.0f};
		movement.target = pos.value;
	}
	
	return true; // Entity was moved
}

void GameplaySystem::update_follow(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_follow");
	
	auto view = registry.view<Follow, Position, Faction, SpatialNode>();
	
	for (auto entity : view) {
		auto& follow = view.get<Follow>(entity);
		auto& pos = view.get<Position>(entity);
		const auto& faction = view.get<Faction>(entity);
		
		// Update target selection timer
		follow.targetTimer += dt;
		
		// Check if we need to select a new target
		bool needNewTarget = false;
		if (follow.target == entt::null || !registry.valid(follow.target)) {
			needNewTarget = true;
		} else {
			// Check if target is still alive
			if (registry.all_of<Health>(follow.target)) {
				const auto& targetHealth = registry.get<Health>(follow.target);
				if (targetHealth.current <= 0) {
					needNewTarget = true;
				}
			} else {
				needNewTarget = true;
			}
		}
		
		// Select new target if needed and cooldown has passed
		if (needNewTarget && follow.targetTimer >= follow.targetCooldown) {
			follow.targetTimer = 0.0f;
			
			// Find nearest ally within search radius
			entt::entity bestTarget = entt::null;
			float bestDist = follow.searchRadius;
			
			_spatial_grid.QueryRadius(pos.value, follow.searchRadius, [&](entt::entity ally) {
				// Skip self
				if (ally == entity) return;
				
				// Check if ally is valid and alive
				if (registry.valid(ally) && registry.all_of<Health, Position>(ally)) {
					const auto& allyHealth = registry.get<Health>(ally);
					if (allyHealth.current > 0) {
						const auto& allyPos = registry.get<Position>(ally);
						float dist = Vec2::distance(pos.value, allyPos.value);
						if (dist < bestDist) {
							bestDist = dist;
							bestTarget = ally;
						}
					}
				}
			}, faction.id, true); // true = same faction (allies)
			
			follow.target = bestTarget;
		}
		
		// Move towards target if we have one
		if (follow.target != entt::null && registry.valid(follow.target) && 
		    registry.all_of<Position>(follow.target)) {
			const auto& targetPos = registry.get<Position>(follow.target);
			float dist = Vec2::distance(pos.value, targetPos.value);
			
			// Only move if we're outside the follow range
			if (dist > follow.followRange) {
				Vec2 oldPos = pos.value;
				
				// Calculate direction and move
				Vec2 direction = Vec2::direction_to(pos.value, targetPos.value);
				Vec2 velocity = direction * follow.speed;
				pos.value += velocity * dt;
				
				// Don't overshoot - stop at follow range distance
				float newDist = Vec2::distance(pos.value, targetPos.value);
				if (newDist < follow.followRange) {
					// Calculate position at follow range from target
					Vec2 stopDirection = Vec2::direction_to(targetPos.value, pos.value);
					pos.value = targetPos.value + stopDirection * follow.followRange;
				}
				
				// Update spatial grid
				_spatial_grid.Update(entity, oldPos, pos.value);
			}
		}
	}
}

void GameplaySystem::update_targeting(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_targeting");
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
			target_comp.target = entt::null;
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

		// Sync stage attacking tag
		if (target_comp.target != entt::null) {
			if (!registry.all_of<StateAttackingTag>(entity)) {
				registry.emplace<StateAttackingTag>(entity);
			}
		} else {
			if (registry.all_of<StateAttackingTag>(entity)) {
				registry.remove<StateAttackingTag>(entity);
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
		
		// Sync stage attacking tag
		if (target_comp.target != entt::null) {
			if (!registry.all_of<StateAttackingTag>(entity)) {
				registry.emplace<StateAttackingTag>(entity);
			}
		}
		else {
			if (registry.all_of<StateAttackingTag>(entity)) {
				registry.remove<StateAttackingTag>(entity);
			}
		}
	}
}

void GameplaySystem::update_melee_combat(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_melee_combat");
	auto view = registry.view<DirectDamage, AttackTarget, StateAttackingTag, Position, Faction>();
	
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
						target_health.Damage(damage_comp.damage);

						// Reset timer
						damage_comp.timer = 0.0f;
					}
				}
			}
		}
	}
}

void GameplaySystem::update_ranged_combat(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_ranged_combat");
	auto view = registry.view<ProjectileEmitter, AttackTarget, StateAttackingTag, Position, Faction>();
	
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
							Vec2::direction_to(pos.value, target_pos.value) * emitter.projectile_speed, // velocity 
							target_pos.value,           // target
							emitter.projectile_speed    // speed
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
	ZoneScopedN("GameplaySystem::update_healer");
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
			_spatial_grid.QueryRadius(pos.value, healer.range, [&](entt::entity ally) {
				if (registry.valid(ally) && registry.all_of<Health>(ally)) {
					auto& health = registry.get<Health>(ally);
					// Only heal if not at full health
					if (!health.IsFullHealth()) {
						health.Heal(healer.heal_amount);
					}
				}
			}, faction.id, true);

			// Reset timer
			healer.timer = 0.0f;
		}
	}
}

void GameplaySystem::update_projectiles(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_projectiles");
	auto view = registry.view<Projectile, Position, Movement>();
	
	std::vector<entt::entity> to_destroy;

	for (auto entity : view) {
		auto& projectile = view.get<Projectile>(entity);
		const auto& pos = view.get<Position>(entity);
		const auto& movement = view.get<Movement>(entity);

		// Check if reached target (movement system handles actual movement)
		// Movement system stops movement when dist < 0.5f, so we check if state is NotMoving
		if (movement.velocity.isZero()) {
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
						health.Damage(projectile.damage);
					}
				}
			}

			// Mark for destruction
			to_destroy.push_back(entity);
		}
	}

	// Destroy projectiles that hit
	registry.destroy(to_destroy.begin(), to_destroy.end());
}

void GameplaySystem::update_death(entt::registry& registry, float dt) {
	ZoneScopedN("GameplaySystem::update_death");
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

