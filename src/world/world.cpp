#include "world.hpp"

World::World()
	: _cameraEntity(entt::null)
	, _spatialGrid(nullptr)
	, _gameplaySystem(nullptr)
	, _renderSystem(nullptr)
	, _unitFactory(nullptr)
{
}

bool World::Initialize(const nlohmann::json& config) {
	// Create systems
	_spatialGrid = new SpatialGrid(_registry);
	_gameplaySystem = new GameplaySystem(*_spatialGrid);
	_renderSystem = new RenderSystem();
	_unitFactory = new UnitFactory(config);

	// Initialize render system
	_renderSystem->init(config);

	// Create camera entity
	_cameraEntity = _registry.create();
	_registry.emplace<Camera>(_cameraEntity, Vec2{0.0f, 0.0f}, 1.0f);
	_registry.emplace<MainCamera>(_cameraEntity);

	return true;
}

void World::Update(float dt) {
	_gameplaySystem->update(_registry, dt);
}

void World::Render() {
	_renderSystem->update(_registry);
}

entt::entity World::SpawnUnit(UnitType type, int faction, const Vec2& position) {
	return _unitFactory->spawn_unit(_registry, type, faction, position);
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

