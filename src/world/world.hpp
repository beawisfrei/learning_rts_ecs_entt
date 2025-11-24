#pragma once

#include <entt/entt.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "../components/components.hpp"
#include "spatial_grid.hpp"
#include "../systems/gameplay_system.hpp"
#include "../systems/render_system.hpp"
#include "unit_factory.hpp"

struct UnitCountData {
	int footmanCount[8] = {0};
	int archerCount[8] = {0};
	int ballistaCount[8] = {0};
	int healerCount[8] = {0};
	int selectedCount = 0;
	int projectileCount = 0;
};

class World {
public:
	World();
	~World() = default;

	// Initialize world with configuration
	bool Initialize(const nlohmann::json& config);

	// Update gameplay systems
	void Update(float dt);

	// Render the world
	void Render();

	// Spawn a unit at the specified position
	entt::entity SpawnUnit(UnitType type, int faction, const Vec2& position);

	// Accessors
	entt::registry& GetRegistry() { return _registry; }
	SpatialGrid& GetSpatialGrid() { return *_spatialGrid; }
	entt::entity GetCameraEntity() const { return _cameraEntity; }
	Camera* GetCamera();

	// Get unit statistics
	UnitCountData GetUnitCounts() const;

	// Save/Load game state
	bool SaveGame(const std::string& filepath);
	bool LoadGame(const std::string& filepath);

private:
	entt::registry _registry;
	entt::entity _cameraEntity;

	// Systems and utilities (owned by World)
	SpatialGrid* _spatialGrid;
	GameplaySystem* _gameplaySystem;
	RenderSystem* _renderSystem;
	UnitFactory* _unitFactory;
};

