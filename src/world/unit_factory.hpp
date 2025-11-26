#pragma once

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include "../components/components.hpp"

class UnitFactory {
public:
	UnitFactory(const nlohmann::json& config) : _config(config) {}

	// Spawn a unit of given type at position
	entt::entity spawn_unit(entt::registry& registry, UnitType type, int faction, const Vec2& position) {
		auto entity = registry.create();

		// Add common components
		registry.emplace<Position>(entity, position);
		registry.emplace<Unit>(entity, type, faction);
		registry.emplace<Faction>(entity, faction);

		// Get unit config data
		int type_idx = static_cast<int>(type);
		if (type_idx < 0 || type_idx >= _config["units"].size()) {
			return entity; // Return basic entity if config missing
		}

		const auto& unit_config = _config["units"][type_idx];
		
		// Add Movement component with speed from config
		float speed = unit_config.value("speed", 10.0f);
		registry.emplace<Movement>(entity, Vec2{0.0f, 0.0f}, position, speed);

		// Add Health component (all units have health)
		float max_hp = unit_config.value("hp", 100.0f);
		float shield = unit_config.value("shield", 0.0f);
		registry.emplace<Health>(entity, max_hp, max_hp, shield);

		// Add type-specific components
		switch (type) {
			case UnitType::Footman: {
				// Melee unit - DirectDamage
				float damage = unit_config.value("damage", 10.0f);
				float range = unit_config.value("range", 1.5f);
				float cooldown = unit_config.value("attack_cooldown", 1.0f);
				registry.emplace<DirectDamage>(entity, damage, range, cooldown, 0.0f);
				registry.emplace<AttackTarget>(entity, entt::null);
				break;
			}
			case UnitType::Archer: {
				// Ranged unit - ProjectileEmitter (single target)
				float damage = unit_config.value("damage", 8.0f);
				float range = unit_config.value("range", 10.0f);
				float cooldown = unit_config.value("attack_cooldown", 2.0f);
				float projectile_speed = unit_config.value("projectile_speed", 15.0f);
				registry.emplace<ProjectileEmitter>(entity, damage, range, cooldown, 0.0f, projectile_speed, 0, 0.0f);
				registry.emplace<AttackTarget>(entity, entt::null);
				break;
			}
			case UnitType::Ballista: {
				// Ranged unit - ProjectileEmitter (AOE)
				float damage = unit_config.value("damage", 50.0f);
				float range = unit_config.value("range", 15.0f);
				float cooldown = unit_config.value("attack_cooldown", 5.0f);
				float aoe_radius = unit_config.value("damage_radius", 3.0f);
				float projectile_speed = unit_config.value("projectile_speed", 15.0f);
				registry.emplace<ProjectileEmitter>(entity, damage, range, cooldown, 0.0f, projectile_speed, 1, aoe_radius);
				registry.emplace<AttackTarget>(entity, entt::null);
				break;
			}
			case UnitType::Healer: {
				// Support unit - Healer
				float heal_amount = unit_config.value("heal_amount", 10.0f);
				float heal_range = unit_config.value("heal_range", 5.0f);
				float cooldown = unit_config.value("heal_cooldown", 2.0f);
				registry.emplace<Healer>(entity, heal_amount, heal_range, cooldown, 0.0f);
				
				// Follow component instead of Movement - healer follows allies
				float follow_range = unit_config.value("follow_range", 2.0f);
				float follow_search_radius = unit_config.value("follow_search_radius", 10.0f);
				float follow_target_cooldown = unit_config.value("follow_target_cooldown", 2.0f);
				registry.emplace<Follow>(entity, entt::null, speed, follow_range, follow_search_radius, follow_target_cooldown, 0.0f);
				
				// Remove Movement component for healers (added earlier)
				registry.remove<Movement>(entity);
				break;
			}
		}

		return entity;
	}

private:
	const nlohmann::json& _config;
};

