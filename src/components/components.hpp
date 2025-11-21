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

struct Velocity {
	Vec2 value;
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

// ... Other components will be added in later steps (RVOAgent, Stats, etc.)
