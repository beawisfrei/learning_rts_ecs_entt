#include "spatial_grid.hpp"
#include "../components/components.hpp"
#include <algorithm>

SpatialGrid::SpatialGrid(entt::registry& registry, int width, int height, int cell_size)
	: _registry(registry), _width(width), _height(height), _cell_size(cell_size) {
	_cols = width / cell_size;
	_rows = height / cell_size;
	_cells.resize(_cols * _rows, entt::null);
}

void SpatialGrid::Insert(entt::entity entity, const Vec2& pos) {
	int index = getCellIndex(pos);
	auto& node = _registry.get_or_emplace<SpatialNode>(entity);

	node.cell_index = index;
	node.next = _cells[index]; // Old head becomes next
	node.prev = entt::null;    // We are the new head, no prev

	// If there was an old head, update its prev to point to us
	if (node.next != entt::null) {
		_registry.get<SpatialNode>(node.next).prev = entity;
	}

	// We become the new head
	_cells[index] = entity;
}

void SpatialGrid::Remove(entt::entity entity) {
	if (!_registry.all_of<SpatialNode>(entity)) return;

	auto& node = _registry.get<SpatialNode>(entity);
	int index = node.cell_index;

	if (index == -1) return; // Not in grid

	// 1. Unlink Prev
	if (node.prev != entt::null) {
		_registry.get<SpatialNode>(node.prev).next = node.next;
	} else {
		// If no prev, we were the head. Update the bucket.
		_cells[index] = node.next;
	}

	// 2. Unlink Next
	if (node.next != entt::null) {
		_registry.get<SpatialNode>(node.next).prev = node.prev;
	}

	node.cell_index = -1;
}

void SpatialGrid::Update(entt::entity entity, const Vec2& old_pos, const Vec2& new_pos) {
	int old_idx = getCellIndex(old_pos);
	int new_idx = getCellIndex(new_pos);

	// Only overhead is if we actually crossed a cell boundary
	if (old_idx != new_idx) {
		Remove(entity);
		Insert(entity, new_pos);
	}
}

void SpatialGrid::Clear() {
	_cells.clear();
	_cells.resize(_cols * _rows, entt::null);
}

void SpatialGrid::QueryRect(const Vec2& min, const Vec2& max, EntityCallback callback) {
	queryCells(min, max, [this, &min, &max](entt::entity e) {
		if (!_registry.all_of<Position>(e)) return false;
		const auto& pos = _registry.get<Position>(e);
		return pos.value.x >= min.x && pos.value.x <= max.x &&
		       pos.value.y >= min.y && pos.value.y <= max.y;
	}, callback);
}

entt::entity SpatialGrid::FindNearest(const Vec2& pos, float radius, int faction, bool same_faction) {
	entt::entity best_entity = entt::null;
	float best_dist_sq = radius * radius;
	float radius_sq = radius * radius;

	Vec2 min = {pos.x - radius, pos.y - radius};
	Vec2 max = {pos.x + radius, pos.y + radius};

	queryCells(min, max, [this, &pos, radius_sq, faction, same_faction](entt::entity e) {
		if (!_registry.all_of<Position>(e)) {
			return false;
		}
		const auto& target_pos = _registry.get<Position>(e);
		
		// Always require Faction component
		if (!_registry.all_of<Faction>(e)) return false;

		// Faction filter (specific faction filtering)
		if (faction >= 0) {
			const auto& entity_faction = _registry.get<Faction>(e);
			if (entity_faction.id < 0) return false;
			if (same_faction && entity_faction.id != faction) return false;
			if (!same_faction && entity_faction.id == faction) return false;
		}

		return true;
	}, [this, &pos, &best_entity, &best_dist_sq](entt::entity e) {
		// Update best entity
		const auto& target_pos = _registry.get<Position>(e);
		float dist_sq = Vec2::distance_squared(pos, target_pos.value);
		if (dist_sq < best_dist_sq) {
			best_dist_sq = dist_sq;
			best_entity = e;
		}
	});

	return best_entity;
}

void SpatialGrid::QueryRadius(const Vec2& pos, float radius, EntityCallback callback, int faction, bool same_faction) {
	Vec2 min = {pos.x - radius, pos.y - radius};
	Vec2 max = {pos.x + radius, pos.y + radius};
	float radius_sq = radius * radius;

	queryCells(min, max, [this, &pos, radius_sq, faction, same_faction](entt::entity e) {
		if (!_registry.all_of<Position>(e)) return false;
		const auto& entity_pos = _registry.get<Position>(e);

		if (!_registry.all_of<Faction>(e)) return false;

		// Faction filtering
		if (faction >= 0) {
			const auto& entity_faction = _registry.get<Faction>(e);
			if (entity_faction.id < 0) return false;
			if (same_faction && entity_faction.id != faction) return false;
			if (!same_faction && entity_faction.id == faction) return false;
		}

		float dist_sq = Vec2::distance_squared(pos, entity_pos.value);
		return dist_sq <= radius_sq;
	}, callback);
}

void SpatialGrid::queryCells(const Vec2& min, const Vec2& max, EntityFilter filter, EntityCallback callback) {
	int start_x = static_cast<int>(min.x) / _cell_size;
	int end_x = static_cast<int>(max.x) / _cell_size;
	int start_y = static_cast<int>(min.y) / _cell_size;
	int end_y = static_cast<int>(max.y) / _cell_size;

	// Clamp
	start_x = std::max(0, start_x); end_x = std::min(_cols - 1, end_x);
	start_y = std::max(0, start_y); end_y = std::min(_rows - 1, end_y);

	for (int y = start_y; y <= end_y; ++y) {
		for (int x = start_x; x <= end_x; ++x) {
			entt::entity curr = _cells[x + y * _cols];

			// Traverse the linked list in this cell
			while (curr != entt::null) {
				const auto& node = _registry.get<SpatialNode>(curr);
				
				// Apply filter function - if it returns true, call callback
				if (filter(curr)) {
					callback(curr);
				}

				curr = node.next;
			}
		}
	}
}

int SpatialGrid::getCellIndex(const Vec2& pos) const {
	int x = static_cast<int>(pos.x / _cell_size);
	int y = static_cast<int>(pos.y / _cell_size);
	// Clamp to boundaries to prevent crash
	x = std::max(0, std::min(x, _cols - 1));
	y = std::max(0, std::min(y, _rows - 1));
	return x + y * _cols;
}

