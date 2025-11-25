#pragma once

#include <entt/entt.hpp>
#include "../utils/vec2.hpp"
#include <vector>
#include <functional>

// Function types for callbacks
using EntityCallback = std::function<void(entt::entity)>;
using EntityFilter = std::function<bool(entt::entity)>;

class SpatialGrid {
public:
	SpatialGrid(entt::registry& registry, int width, int height, int cell_size);

	// O(1) - No Allocations
	void Insert(entt::entity entity, const Vec2& pos);

	// O(1) - No Allocations
	void Remove(entt::entity entity);

	// O(1) - The "Movement System" calls this
	void Update(entt::entity entity, const Vec2& old_pos, const Vec2& new_pos);

	// Query all entities within a rectangle
	void QueryRect(const Vec2& min, const Vec2& max, EntityCallback callback);

	// Find nearest entity to a given position within a radius (with optional faction filter)
	entt::entity FindNearest(const Vec2& pos, float radius, int faction = -1, bool same_faction = false);

	// Find all entities within a radius (with optional faction filter)
	void QueryRadius(const Vec2& pos, float radius, EntityCallback callback, int faction = -1, bool same_faction = false);

private:
	// Internal function that queries cells and applies a filter function
	void queryCells(const Vec2& min, const Vec2& max, EntityFilter filter, EntityCallback callback);

	int getCellIndex(const Vec2& pos) const;

	entt::registry& _registry;
	int _width, _height;
	int _cell_size;
	int _cols, _rows;

	// The grid only stores the "Head" of the list for that cell
	std::vector<entt::entity> _cells;
};
