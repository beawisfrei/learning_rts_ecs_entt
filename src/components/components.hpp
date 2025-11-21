#pragma once

// This file aggregates all ECS component definitions
// Following the SDD and plan

#include "../utils/vec2.hpp"

struct Position {
	Vec2 value;
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
};

struct Color {
	float r, g, b, a;
};

struct UVRect {
	float x, y, w, h;
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
};

struct Camera {
	Vec2 offset;
	float zoom;
};

// Tag for the player/input camera (should be only one)
struct MainCamera {};

// Gameplay Components

// Faction component - wrapper for faction ID
struct Faction {
	int id;
};

// Health component - for all units
struct Health {
	float current;
	float max;
	float shield;
};

// DirectDamage component - for melee units (Footman)
struct DirectDamage {
	float damage;
	float range;
	float cooldown;
	float timer;
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
};

// Healer component - for healer units
struct Healer {
	float heal_amount;
	float range;
	float cooldown;
	float timer;
};

// AttackTarget component - stores current target entity
struct AttackTarget {
	entt::entity target;
};

// Projectile component - for projectile entities
struct Projectile {
	float damage;
	int faction;
	bool is_aoe;
	float aoe_radius;
};

// Selection tag
struct Selected {};

// Sprite component for rendering
struct Sprite {
	int texture_id;
	UVRect uv;
	Color color;
};
