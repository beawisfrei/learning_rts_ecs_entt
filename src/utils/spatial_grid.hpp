#pragma once

#include <entt/entt.hpp>
#include "../components/components.hpp"
#include "math_utils.hpp"
#include <vector>

class SpatialGrid {
public:
	SpatialGrid(entt::registry& registry) : _registry(registry) {}

	// Query all entities with Position component within the given rectangle
	std::vector<entt::entity> query_rect(const Vec2& min, const Vec2& max) {
		std::vector<entt::entity> result;
		
		// Brute-force implementation for now
		auto view = _registry.view<Position>();
		for (auto entity : view) {
			const auto& pos = view.get<Position>(entity);
			if (MathUtils::point_in_rect(pos.value, min, max)) {
				result.push_back(entity);
			}
		}
		
		return result;
	}

	// Find nearest entity to a given position within a radius (with optional faction filter)
	entt::entity find_nearest(const Vec2& pos, float radius, int faction = -1, bool same_faction = false) {
		entt::entity nearest = entt::null;
		float min_dist = radius;

		auto view = _registry.view<Position, Faction>();
		for (auto entity : view) {
			const auto& entity_pos = view.get<Position>(entity);
			const auto& entity_faction = view.get<Faction>(entity);

			// Faction filtering
			if (faction >= 0) {
				if (same_faction && entity_faction.id != faction) continue;
				if (!same_faction && entity_faction.id == faction) continue;
			}

			float dist = MathUtils::distance(pos, entity_pos.value);
			if (dist < min_dist) {
				min_dist = dist;
				nearest = entity;
			}
		}

		return nearest;
	}

	// Find all entities within a radius (with optional faction filter)
	std::vector<entt::entity> query_radius(const Vec2& pos, float radius, int faction = -1, bool same_faction = false) {
		std::vector<entt::entity> result;

		auto view = _registry.view<Position, Faction>();
		for (auto entity : view) {
			const auto& entity_pos = view.get<Position>(entity);
			const auto& entity_faction = view.get<Faction>(entity);

			// Faction filtering
			if (faction >= 0) {
				if (same_faction && entity_faction.id != faction) continue;
				if (!same_faction && entity_faction.id == faction) continue;
			}

			float dist = MathUtils::distance(pos, entity_pos.value);
			if (dist <= radius) {
				result.push_back(entity);
			}
		}

		return result;
	}

private:
	entt::registry& _registry;
};

