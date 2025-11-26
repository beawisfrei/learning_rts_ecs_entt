#include "spatial_grid.hpp"
#include "../components/components.hpp"
#include <algorithm>

// FactionGrid Implementation
void FactionGrid::Resize(int size) {
	_cells.resize(size, entt::null);
	_entity_count = 0;
}

void FactionGrid::Insert(int cell_index, entt::entity entity, entt::registry& registry) {
	auto& node = registry.get_or_emplace<SpatialNode>(entity);

	node.next = _cells[cell_index]; // Old head becomes next
	node.prev = entt::null;         // We are the new head, no prev

	// If there was an old head, update its prev to point to us
	if (node.next != entt::null) {
		registry.get<SpatialNode>(node.next).prev = entity;
	}

	// We become the new head
	_cells[cell_index] = entity;
	_entity_count++;
}

void FactionGrid::Remove(int cell_index, entt::entity entity, entt::registry& registry) {
	if (!registry.all_of<SpatialNode>(entity)) return;

	auto& node = registry.get<SpatialNode>(entity);

	// 1. Unlink Prev
	if (node.prev != entt::null) {
		registry.get<SpatialNode>(node.prev).next = node.next;
	} else {
		// If no prev, we were the head. Update the bucket.
		_cells[cell_index] = node.next;
	}

	// 2. Unlink Next
	if (node.next != entt::null) {
		registry.get<SpatialNode>(node.next).prev = node.prev;
	}

	_entity_count--;
}

void FactionGrid::Query(int min_x, int min_y, int max_x, int max_y, int cols, entt::registry& registry, EntityCallback callback) {
	for (int y = min_y; y <= max_y; ++y) {
		for (int x = min_x; x <= max_x; ++x) {
			int cell_index = x + y * cols;
			entt::entity curr = _cells[cell_index];

			// Traverse the linked list in this cell
			while (curr != entt::null) {
				const auto& node = registry.get<SpatialNode>(curr);
				callback(curr);
				curr = node.next;
			}
		}
	}
}

void FactionGrid::Clear() {
	std::fill(_cells.begin(), _cells.end(), entt::null);
	_entity_count = 0;
}

// SpatialGrid Implementation
SpatialGrid::SpatialGrid(entt::registry& registry, int width, int height, int cell_size)
	: _registry(registry), _width(width), _height(height), _cell_size(cell_size) {
	_cols = width / cell_size;
	_rows = height / cell_size;
	
	// Initialize all faction grids
	for (int i = 0; i < MAX_FACTIONS; i++) {
		_grids[i].Resize(_cols * _rows);
	}
}

FactionGrid& SpatialGrid::getGrid(int faction) {
	return _grids[faction];
}

void SpatialGrid::Insert(entt::entity entity, const Vec2& pos, int faction) {
	// Determine faction if not provided
	int entity_faction = faction;
	if (entity_faction == -1) {
		if (_registry.all_of<Faction>(entity)) {
			entity_faction = _registry.get<Faction>(entity).id;
		} else {
			// No faction, cannot insert
			return;
		}
	}

	// Validate faction range
	if (entity_faction < 0 || entity_faction >= MAX_FACTIONS) {
		return;
	}

	int cell_index = getCellIndex(pos);
	
	// Get the faction grid
	FactionGrid& grid = getGrid(entity_faction);
	
	// Update SpatialNode with cell_index and faction
	auto& node = _registry.get_or_emplace<SpatialNode>(entity);
	node.cell_index = cell_index;
	node.faction = entity_faction;
	
	// Insert into the faction grid
	grid.Insert(cell_index, entity, _registry);
}

void SpatialGrid::Remove(entt::entity entity) {
	if (!_registry.all_of<SpatialNode>(entity)) return;

	auto& node = _registry.get<SpatialNode>(entity);
	int faction = node.faction;
	int cell_index = node.cell_index;

	if (faction == -1 || cell_index == -1) return; // Not in grid
	if (faction < 0 || faction >= MAX_FACTIONS) return; // Invalid faction

	// Remove from the faction grid
	_grids[faction].Remove(cell_index, entity, _registry);
	
	node.cell_index = -1;
	node.faction = -1;
}

void SpatialGrid::Update(entt::entity entity, const Vec2& old_pos, const Vec2& new_pos) {
	if (!_registry.all_of<SpatialNode>(entity)) {
		// Entity not in grid, try to insert it
		Insert(entity, new_pos);
		return;
	}

	auto& node = _registry.get<SpatialNode>(entity);
	int old_faction = node.faction;
	int old_idx = getCellIndex(old_pos);
	int new_idx = getCellIndex(new_pos);
	
	// Get current faction
	int new_faction = -1;
	if (_registry.all_of<Faction>(entity)) {
		new_faction = _registry.get<Faction>(entity).id;
	} else {
		// No faction, remove from grid
		Remove(entity);
		return;
	}

	// Check if cell index OR faction changed
	if (old_idx != new_idx || old_faction != new_faction) {
		Remove(entity);
		Insert(entity, new_pos, new_faction);
	}
}

void SpatialGrid::Clear() {
	for (int i = 0; i < MAX_FACTIONS; i++) {
		_grids[i].Clear();
	}
}

void SpatialGrid::getCellCoords(const Vec2& pos, int& x, int& y) const {
	x = static_cast<int>(pos.x / _cell_size);
	y = static_cast<int>(pos.y / _cell_size);
	// Clamp to boundaries to prevent crash
	x = std::max(0, std::min(x, _cols - 1));
	y = std::max(0, std::min(y, _rows - 1));
}

template<typename Func>
void SpatialGrid::forEachRelevantGrid(int faction, bool same_faction, Func&& func) {
	if (faction >= 0 && faction < MAX_FACTIONS) {
		if (same_faction) {
			// Query only the same faction
			if (!_grids[faction].IsEmpty()) {
				func(_grids[faction]);
			}
		} else {
			// Query all factions except the specified one
			for (int i = 0; i < MAX_FACTIONS; i++) {
				if (i == faction) continue; // Skip same faction
				if (_grids[i].IsEmpty()) continue; // Skip empty grids
				func(_grids[i]);
			}
		}
	} else {
		// Query all factions
		for (int i = 0; i < MAX_FACTIONS; i++) {
			if (_grids[i].IsEmpty()) continue; // Skip empty grids
			func(_grids[i]);
		}
	}
}

void SpatialGrid::QueryRect(const Vec2& min, const Vec2& max, EntityCallback callback) {
	// Calculate integer cell bounds once
	int start_x, start_y, end_x, end_y;
	getCellCoords(min, start_x, start_y);
	getCellCoords(max, end_x, end_y);

	// Query all non-empty faction grids
	for (int i = 0; i < MAX_FACTIONS; i++) {
		if (_grids[i].IsEmpty()) continue; // Skip empty grids
		
		_grids[i].Query(start_x, start_y, end_x, end_y, _cols, _registry, [&](entt::entity e) {
			// Additional position check
			if (_registry.all_of<Position>(e)) {
				const auto& pos = _registry.get<Position>(e);
				if (pos.value.x >= min.x && pos.value.x <= max.x &&
				    pos.value.y >= min.y && pos.value.y <= max.y) {
					callback(e);
				}
			}
		});
	}
}

entt::entity SpatialGrid::FindNearest(const Vec2& pos, float radius, int faction, bool same_faction) {
	entt::entity best_entity = entt::null;
	float best_dist_sq = radius * radius;
	float radius_sq = radius * radius;

	Vec2 min = {pos.x - radius, pos.y - radius};
	Vec2 max = {pos.x + radius, pos.y + radius};

	// Calculate integer cell bounds once
	int start_x, start_y, end_x, end_y;
	getCellCoords(min, start_x, start_y);
	getCellCoords(max, end_x, end_y);

	// Query relevant faction grids
	forEachRelevantGrid(faction, same_faction, [&](FactionGrid& grid) {
		grid.Query(start_x, start_y, end_x, end_y, _cols, _registry, [&](entt::entity e) {
			if (!_registry.all_of<Position>(e)) return;
			const auto& target_pos = _registry.get<Position>(e);
			
			float dist_sq = Vec2::distance_squared(pos, target_pos.value);
			if (dist_sq <= radius_sq && dist_sq < best_dist_sq) {
				best_dist_sq = dist_sq;
				best_entity = e;
			}
		});
	});

	return best_entity;
}

void SpatialGrid::QueryRadius(const Vec2& pos, float radius, EntityCallback callback, int faction, bool same_faction) {
	Vec2 min = {pos.x - radius, pos.y - radius};
	Vec2 max = {pos.x + radius, pos.y + radius};
	float radius_sq = radius * radius;

	// Calculate integer cell bounds once
	int start_x, start_y, end_x, end_y;
	getCellCoords(min, start_x, start_y);
	getCellCoords(max, end_x, end_y);

	// Query relevant faction grids
	forEachRelevantGrid(faction, same_faction, [&](FactionGrid& grid) {
		grid.Query(start_x, start_y, end_x, end_y, _cols, _registry, [&](entt::entity e) {
			if (!_registry.all_of<Position>(e)) return;
			const auto& entity_pos = _registry.get<Position>(e);

			float dist_sq = Vec2::distance_squared(pos, entity_pos.value);
			if (dist_sq <= radius_sq) {
				callback(e);
			}
		});
	});
}

int SpatialGrid::getCellIndex(const Vec2& pos) const {
	int x = static_cast<int>(pos.x / _cell_size);
	int y = static_cast<int>(pos.y / _cell_size);
	// Clamp to boundaries to prevent crash
	x = std::max(0, std::min(x, _cols - 1));
	y = std::max(0, std::min(y, _rows - 1));
	return x + y * _cols;
}
