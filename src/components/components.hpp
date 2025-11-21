#pragma once

// This file aggregates all ECS component definitions
// Following the SDD and plan

// Make sure glm is included correctly. 
// Since we use FetchContent/find_package usually, but here we didn't fetch glm!
// Wait, SDD doesn't list GLM as dependency in Table 2.1.
// But the component design (Table 4) lists glm::vec2.
// We missed adding GLM to CMakeLists.txt!

// For now, let's assume we need to add it.
// But wait, EnTT doesn't depend on GLM.
// We need to add GLM to our project.

// Checking Table 2.1 in SDD:
// "EnTT, SDL3, Glad, nlohmann/json, ImGui, RVO2, stb_image, Google Test"
// It missed GLM! But section 4 says "glm::vec2".
// I must add GLM to CMakeLists.txt.

// Since I cannot edit CMakeLists.txt right now (I will do it in next step),
// I will just use a struct for now to fix compilation if I can't add dependency.
// BUT, the plan is to use GLM. So I should add GLM.

// Temporary fix: simple vec2 struct to allow compilation until I fix CMake.
struct Vec2 {
	float x, y;
};

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
