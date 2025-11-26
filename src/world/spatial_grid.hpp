#pragma once

#include <entt/entt.hpp>
#include "../utils/vec2.hpp"
#include <vector>
#include <array>
#include <functional>
#include "../components/components.hpp"

// Function types for callbacks
using EntityCallback = std::function<void(entt::entity)>;
using EntityFilter = std::function<bool(entt::entity)>;

// Internal per-faction grid - stores cells for a single faction
class FactionGrid {
public:
	FactionGrid() = default;

	// Resize the grid to hold specified number of cells
	void Resize(int size);

	// Insert entity into a specific cell
	void Insert(int cell_index, entt::entity entity, entt::registry& registry);

	// Remove entity from a specific cell
	void Remove(int cell_index, entt::entity entity, entt::registry& registry);

	// Query entities in a cell rect (integer coords)
	void Query(int min_x, int min_y, int max_x, int max_y, int cols, entt::registry& registry, EntityCallback callback);

	// Clear all cells
	void Clear();

	// Check if grid is empty
	bool IsEmpty() const { return _entity_count == 0; }

	// Get entity count
	int GetEntityCount() const { return _entity_count; }

private:
	// The grid only stores the "Head" of the list for that cell
	std::vector<entt::entity> _cells;
	int _entity_count = 0;
};

class SpatialGrid {
public:
	SpatialGrid(entt::registry& registry, int width, int height, int cell_size);

	// O(1) - No Allocations
	void Insert(entt::entity entity, const Vec2& pos, int faction = -1);

	// O(1) - No Allocations
	void Remove(entt::entity entity);

	// O(1) - The "Movement System" calls this
	void Update(entt::entity entity, const Vec2& old_pos, const Vec2& new_pos);

	// Just clears vector of entities
	void Clear();

	// Query all entities within a rectangle
	void QueryRect(const Vec2& min, const Vec2& max, EntityCallback callback);

	// Find nearest entity to a given position within a radius (with optional faction filter)
	entt::entity FindNearest(const Vec2& pos, float radius, int faction = -1, bool same_faction = false);

	// Find all entities within a radius (with optional faction filter)
	void QueryRadius(const Vec2& pos, float radius, EntityCallback callback, int faction = -1, bool same_faction = false);

	// Get world dimensions
	int GetWidth() const { return _width; }
	int GetHeight() const { return _height; }

private:
	// Get or create a faction grid
	FactionGrid& getGrid(int faction);

	// Convert float position to cell index
	int getCellIndex(const Vec2& pos) const;

	// Convert float position to cell coords
	void getCellCoords(const Vec2& pos, int& x, int& y) const;

	// Helper to iterate over relevant faction grids based on faction filter
	template<typename Func>
	void forEachRelevantGrid(int faction, bool same_faction, Func&& func);

	entt::registry& _registry;
	int _width, _height;
	int _cell_size;
	int _cols, _rows;

	// Per-faction grids (fixed array for optimization)
	std::array<FactionGrid, MAX_FACTIONS> _grids;
};
