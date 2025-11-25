#pragma once

// This file aggregates all ECS component definitions
// Following the SDD and plan

#include "../utils/vec2.hpp"
#include <entt/entt.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>

struct Position {
	Vec2 value;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(value));
	}
};

enum class MovementState {
	NotMoving,
	Paused,
	Moving
};

struct Movement {
	Vec2 velocity;
	Vec2 target;
	float speed;
	MovementState state;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(velocity), CEREAL_NVP(target), CEREAL_NVP(speed));
		// Serialize enum as int
		int stateInt = static_cast<int>(state);
		archive(CEREAL_NVP(stateInt));
		state = static_cast<MovementState>(stateInt);
	}
};

struct Color {
	float r, g, b, a;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(r), CEREAL_NVP(g), CEREAL_NVP(b), CEREAL_NVP(a));
	}
};

struct UVRect {
	float x, y, w, h;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(w), CEREAL_NVP(h));
	}
};

enum class UnitType {
	Footman,
	Archer,
	Ballista,
	Healer
};

struct Unit {
	UnitType type;
	int faction;

	template<class Archive>
	void serialize(Archive &archive) {
		// Serialize enum as int
		int typeInt = static_cast<int>(type);
		archive(CEREAL_NVP(typeInt), CEREAL_NVP(faction));
		type = static_cast<UnitType>(typeInt);
	}
};

struct Camera {
	Vec2 offset;
	float zoom;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(offset), CEREAL_NVP(zoom));
	}
};

// Tag for the player/input camera (should be only one)
struct MainCamera {
	template<class Archive>
	void serialize(Archive &archive) {
		// Empty tag component
	}
};

// Gameplay Components

// Faction component - wrapper for faction ID
struct Faction {
	int id;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(id));
	}
};

// Health component - for all units
struct Health {
	float current;
	float max;
	float shield;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(current), CEREAL_NVP(max), CEREAL_NVP(shield));
	}
};

// DirectDamage component - for melee units (Footman)
struct DirectDamage {
	float damage;
	float range;
	float cooldown;
	float timer;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(damage), CEREAL_NVP(range), CEREAL_NVP(cooldown), CEREAL_NVP(timer));
	}
};

// ProjectileEmitter component - for ranged units (Archer, Ballista)
struct ProjectileEmitter {
	float damage;
	float range;
	float cooldown;
	float timer;
	float projectile_speed;
	int projectile_type; // 0 = normal, 1 = AOE
	float aoe_radius; // Only used if projectile_type == 1

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(damage), CEREAL_NVP(range), CEREAL_NVP(cooldown), CEREAL_NVP(timer),
		        CEREAL_NVP(projectile_speed), CEREAL_NVP(projectile_type), CEREAL_NVP(aoe_radius));
	}
};

// Healer component - for healer units
struct Healer {
	float heal_amount;
	float range;
	float cooldown;
	float timer;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(heal_amount), CEREAL_NVP(range), CEREAL_NVP(cooldown), CEREAL_NVP(timer));
	}
};

// AttackTarget component - stores current target entity
struct AttackTarget {
	entt::entity target;

	template<class Archive>
	void serialize(Archive &archive) {
		// Serialize entity as underlying integer type
		archive(CEREAL_NVP(target));
	}
};

// Projectile component - for projectile entities
struct Projectile {
	float damage;
	int faction;
	bool is_aoe;
	float aoe_radius;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(damage), CEREAL_NVP(faction), CEREAL_NVP(is_aoe), CEREAL_NVP(aoe_radius));
	}
};

// Selection tag
struct Selected {
	template<class Archive>
	void serialize(Archive &archive) {
		// Empty tag component
	}
};

// Sprite component for rendering
struct Sprite {
	int texture_id;
	UVRect uv;
	Color color;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(texture_id), CEREAL_NVP(uv), CEREAL_NVP(color));
	}
};

// SpatialNode component - for intrusive doubly-linked list in spatial grid
struct SpatialNode {
	entt::entity next = entt::null;
	entt::entity prev = entt::null;
	int cell_index = -1;

	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(next), CEREAL_NVP(prev), CEREAL_NVP(cell_index));
	}
};