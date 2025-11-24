#pragma once

#include <imgui.h>
#include <string>
#include "../components/components.hpp"
#include "../world/world.hpp"

class InputSystem;
class TimeController;

class UISystem {
public:
	UISystem() = default;

	// Render all UI elements
	void Render(World& world, InputSystem& inputSystem, float dt, TimeController& timeController);

	// Get spawn parameters
	UnitType GetSpawnType() const { return static_cast<UnitType>(_spawnTypeIdx); }
	int GetSpawnFaction() const { return _spawnFaction; }
	int GetSpawnCount() const { return _spawnCount; }

private:
	void renderDebugWindow(World& world, float dt, TimeController& timeController);
	void renderSelectionRect(World& world, InputSystem& inputSystem);

	// Spawn parameters
	int _spawnTypeIdx = 0;
	int _spawnFaction = 0;
	int _spawnCount = 10;

	// Save/Load parameters
	std::string _saveFilePath = "saves/game.json";
	std::string _saveLoadStatus = "";
};

