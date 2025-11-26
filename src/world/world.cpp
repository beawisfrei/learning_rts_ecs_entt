#include "world.hpp"
#include "../utils/resource_loader.hpp"
#include "../utils/profiler.hpp"
#include <cereal/archives/json.hpp>
#include <cereal/types/map.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>

World::World()
	: _cameraEntity(entt::null)
	, _spatialGrid(nullptr)
	, _gameplaySystem(nullptr)
	, _renderSystem(nullptr)
	, _unitFactory(nullptr)
{
}

bool World::Initialize(const nlohmann::json& config, bool enableRender) {
	// Get terrain texture dimensions to calculate world bounds
	int terrain_width = 0, terrain_height = 0;
	if (!ResourceLoader::GetImageDimensions("data/terrain.png", terrain_width, terrain_height)) {
		std::cerr << "Failed to get terrain dimensions, using defaults" << std::endl;
		terrain_width = 1000;
		terrain_height = 1000;
	}

	// Calculate world bounds: terrain_size * tile_size
	int tile_size = config["global"].value("tile_size", 1);
	int world_width = terrain_width * tile_size;
	int world_height = terrain_height * tile_size;

	// Get cell_size from config
	int cell_size = config["global"].value("cell_size", 50);

	// Create systems
	_spatialGrid = new SpatialGrid(_registry, world_width, world_height, cell_size);
	_gameplaySystem = new GameplaySystem(*_spatialGrid);
	_unitFactory = new UnitFactory(config);

	// Initialize render system
	if (enableRender) {
		_renderSystem = new RenderSystem();
		_renderSystem->init(config);
		_renderSystem->SetWorldBounds(world_width, world_height);
	}

	// Create camera entity
	_cameraEntity = _registry.create();
	_registry.emplace<Camera>(_cameraEntity, Vec2{0.0f, 0.0f}, 1.0f);
	_registry.emplace<MainCamera>(_cameraEntity);

	return true;
}

void World::Update(float dt) {
	ZoneScopedN("World::Update");
	_gameplaySystem->update(_registry, dt);
}

void World::Render() {
	ZoneScopedN("World::Render");
	if (_renderSystem) {
		_renderSystem->update(_registry);
	}
}

entt::entity World::SpawnUnit(UnitType type, int faction, const Vec2& position) {
	// Check if position is within world bounds
	if (_spatialGrid) {
		if (position.x < 0 || position.x >= _spatialGrid->GetWidth() ||
			position.y < 0 || position.y >= _spatialGrid->GetHeight()) {
			// Position is outside world borders, skip unit creation
			return entt::null;
		}
	}

	auto entity = _unitFactory->spawn_unit(_registry, type, faction, position);
	
	// Insert entity into spatial grid with faction
	if (_spatialGrid && _registry.all_of<Position>(entity)) {
		const auto& pos = _registry.get<Position>(entity);
		_spatialGrid->Insert(entity, pos.value, faction);
	}
	
	return entity;
}

Camera* World::GetCamera() {
	if (_cameraEntity == entt::null) {
		return nullptr;
	}
	return _registry.try_get<Camera>(_cameraEntity);
}

UnitCountData World::GetUnitCounts() const {
	UnitCountData counts;

	auto unitView = _registry.view<Unit, Faction>();
	for (auto entity : unitView) {
		const auto& unit = unitView.get<Unit>(entity);
		const auto& faction = unitView.get<Faction>(entity);
		
		// Skip projectiles (they have Unit component but shouldn't be counted)
		if (_registry.all_of<Projectile>(entity)) {
			counts.projectileCount++;
			continue;
		}
		
		int f = faction.id;
		if (f >= 0 && f < 8) {
			switch (unit.type) {
				case UnitType::Footman: counts.footmanCount[f]++; break;
				case UnitType::Archer: counts.archerCount[f]++; break;
				case UnitType::Ballista: counts.ballistaCount[f]++; break;
				case UnitType::Healer: counts.healerCount[f]++; break;
			}
		}
		
		if (_registry.all_of<Selected>(entity)) {
			counts.selectedCount++;
		}
	}

	return counts;
}

const std::vector<Color>& World::GetFactionColors() const {
	static std::vector<Color> empty_colors;
	if (_renderSystem) {
		return _renderSystem->GetFactionColors();
	}
	return empty_colors;
}

bool World::SaveGame(const std::string& filepath) {
	try {
		// Create directory if it doesn't exist
		std::filesystem::path path(filepath);
		std::filesystem::path dir = path.parent_path();
		if (!dir.empty() && !std::filesystem::exists(dir)) {
			std::filesystem::create_directories(dir);
		}

		// Open file for writing
		std::ofstream os(filepath);
		if (!os.is_open()) {
			std::cerr << "Failed to open file for writing: " << filepath << std::endl;
			return false;
		}

		// Create JSON output archive
		{
			cereal::JSONOutputArchive archive(os);

			// Create snapshot and serialize all entities and components using EnTT's .get() API
			entt::snapshot snapshot{ _registry };
			snapshot.get<entt::entity>(archive)
				.get<Position>(archive)
				.get<Movement>(archive)
				.get<Color>(archive)
				.get<Unit>(archive)
				.get<Camera>(archive)
				.get<MainCamera>(archive)
				.get<Faction>(archive)
				.get<Health>(archive)
				.get<DirectDamage>(archive)
				.get<ProjectileEmitter>(archive)
				.get<Healer>(archive)
				.get<AttackTarget>(archive)
				.get<Projectile>(archive)
				.get<StateAttackingTag>(archive);
		}

		os.close();
		return true;
	} catch (const std::exception& e) {
		std::cerr << "Error saving game: " << e.what() << std::endl;
		return false;
	}
}

bool World::LoadGame(const std::string& filepath) {
	try {
		// Check if file exists
		if (!std::filesystem::exists(filepath)) {
			std::cerr << "Save file does not exist: " << filepath << std::endl;
			return false;
		}

		// Open file for reading
		std::ifstream is(filepath);
		if (!is.is_open()) {
			std::cerr << "Failed to open file for reading: " << filepath << std::endl;
			return false;
		}

		// Clear current registry
		_registry.clear();
		_spatialGrid->Clear();
		_cameraEntity = entt::null;

		// Create JSON input archive
		cereal::JSONInputArchive archive(is);

		// Create continuous loader for entity remapping
		entt::continuous_loader loader{_registry};

		// Load entities and all components using EnTT's .get() API
		loader.get<entt::entity>(archive)
			.get<Position>(archive)
			.get<Movement>(archive)
			.get<Color>(archive)
			.get<Unit>(archive)
			.get<Camera>(archive)
			.get<MainCamera>(archive)
			.get<Faction>(archive)
			.get<Health>(archive)
			.get<DirectDamage>(archive)
			.get<ProjectileEmitter>(archive)
			.get<Healer>(archive)
			.get<AttackTarget>(archive)
			.get<Projectile>(archive)
			.get<StateAttackingTag>(archive);

		// Post-process: Fix entity references in AttackTarget components
		// The continuous_loader automatically handles entity remapping internally,
		// but AttackTarget stores an entity that needs manual remapping
		auto attackTargetView = _registry.view<AttackTarget>();
		for (auto entity : attackTargetView) {
			auto& at = attackTargetView.get<AttackTarget>(entity);
			if (at.target != entt::null && loader.contains(at.target)) {
				at.target = loader.map(at.target);
			} else {
				at.target = entt::null; // Reference died or invalid
			}
		}

		// Find the camera entity (should have MainCamera tag)
		auto cameraView = _registry.view<MainCamera>();
		if (!cameraView.empty()) {
			_cameraEntity = *cameraView.begin();
		}

		// Insert all loaded entities with Position into spatial grid
		if (_spatialGrid) {
			auto positionView = _registry.view<Position>();
			for (auto entity : positionView) {
				const auto& pos = positionView.get<Position>(entity);
				_spatialGrid->Insert(entity, pos.value);
			}
		}

		// Clean up orphaned entities
		loader.orphans();

		is.close();
		return true;
	} catch (const std::exception& e) {
		std::cerr << "Error loading game: " << e.what() << std::endl;
		return false;
	}
}

