#include <gtest/gtest.h>
#include "../src/world/spatial_grid.hpp"
#include "../src/components/components.hpp"
#include "../src/utils/vec2.hpp"
#include <algorithm>
#include <set>
#include <memory>
#include <vector>
#include <functional>

class SpatialGridTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Create grid with explicit dimensions for testing
		grid = std::make_unique<SpatialGrid>(registry, 1000, 1000, 50);
	}

	void TearDown() override {
		grid.reset();
		registry.clear();
	}

	// Helper method to create entity with Position and optional Faction, and insert into grid
	entt::entity createEntity(Vec2 pos, int faction = -1) {
		auto entity = registry.create();
		registry.emplace<Position>(entity, Position{pos});
		if (faction >= 0) {
			registry.emplace<Faction>(entity, Faction{faction});
		}
		// Insert into spatial grid
		grid->Insert(entity, pos);
		return entity;
	}

	// Helper method to create entity with only Position (no Faction), and insert into grid
	entt::entity createEntityWithoutFaction(Vec2 pos) {
		auto entity = registry.create();
		registry.emplace<Position>(entity, Position{pos});
		// Insert into spatial grid
		grid->Insert(entity, pos);
		return entity;
	}

	// Helper to collect entities from callback-based query
	std::vector<entt::entity> collectEntities(std::function<void(std::function<void(entt::entity)>)> query) {
		std::vector<entt::entity> result;
		query([&](entt::entity e) {
			result.push_back(e);
		});
		return result;
	}

	// Helper to check if vector contains entity
	bool containsEntity(const std::vector<entt::entity>& vec, entt::entity entity) {
		return std::find(vec.begin(), vec.end(), entity) != vec.end();
	}

	// Helper to check if two vectors contain same entities (order independent)
	bool sameEntities(const std::vector<entt::entity>& vec1, const std::vector<entt::entity>& vec2) {
		std::set<entt::entity> set1(vec1.begin(), vec1.end());
		std::set<entt::entity> set2(vec2.begin(), vec2.end());
		return set1 == set2;
	}

	entt::registry registry;
	std::unique_ptr<SpatialGrid> grid;
};

// ============================================================================
// Insert/Remove/Update Tests
// ============================================================================

TEST_F(SpatialGridTest, Insert_AddsEntityToGrid) {
	auto e1 = registry.create();
	registry.emplace<Position>(e1, Position{Vec2(5.0f, 5.0f)});
	
	grid->Insert(e1, Vec2(5.0f, 5.0f));
	
	EXPECT_TRUE(registry.all_of<SpatialNode>(e1));
	const auto& node = registry.get<SpatialNode>(e1);
	EXPECT_GE(node.cell_index, 0);
}

TEST_F(SpatialGridTest, Remove_RemovesEntityFromGrid) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	
	grid->Remove(e1);
	
	const auto& node = registry.get<SpatialNode>(e1);
	EXPECT_EQ(node.cell_index, -1);
}

TEST_F(SpatialGridTest, Update_MovesEntityBetweenCells) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	const auto& node1 = registry.get<SpatialNode>(e1);
	int old_cell = node1.cell_index;
	
	grid->Update(e1, Vec2(5.0f, 5.0f), Vec2(200.0f, 200.0f));
	
	const auto& node2 = registry.get<SpatialNode>(e1);
	EXPECT_NE(node2.cell_index, old_cell);
}

TEST_F(SpatialGridTest, Update_NoOpWhenSameCell) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	const auto& node1 = registry.get<SpatialNode>(e1);
	int old_cell = node1.cell_index;
	
	grid->Update(e1, Vec2(5.0f, 5.0f), Vec2(10.0f, 10.0f));
	
	const auto& node2 = registry.get<SpatialNode>(e1);
	EXPECT_EQ(node2.cell_index, old_cell);
}

// ============================================================================
// QueryRect Tests
// ============================================================================

TEST_F(SpatialGridTest, QueryRect_ReturnsEntitiesInsideRectangle) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	auto e2 = createEntity(Vec2(15.0f, 15.0f));
	auto e3 = createEntity(Vec2(25.0f, 25.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(20.0f, 20.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_FALSE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRect_ReturnsEntitiesOnBoundary) {
	auto e1 = createEntity(Vec2(0.0f, 0.0f));  // On min corner
	auto e2 = createEntity(Vec2(10.0f, 10.0f)); // On max corner
	auto e3 = createEntity(Vec2(5.0f, 0.0f));   // On min edge
	auto e4 = createEntity(Vec2(0.0f, 5.0f));   // On min edge

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 4);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
	EXPECT_TRUE(containsEntity(result, e4));
}

TEST_F(SpatialGridTest, QueryRect_ReturnsEmptyWhenNoEntitiesInRectangle) {
	createEntity(Vec2(50.0f, 50.0f));
	createEntity(Vec2(100.0f, 100.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_TRUE(result.empty());
}

TEST_F(SpatialGridTest, QueryRect_ReturnsAllEntitiesWhenRectangleContainsAll) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	auto e2 = createEntity(Vec2(15.0f, 15.0f));
	auto e3 = createEntity(Vec2(25.0f, 25.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(100.0f, 100.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRect_SinglePointRectangle) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	auto e2 = createEntity(Vec2(5.1f, 5.1f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(5.0f, 5.0f), Vec2(5.0f, 5.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_FALSE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRect_InvalidRectangleMinGreaterThanMax) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));

	// Invalid rectangle: min > max
	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(10.0f, 10.0f), Vec2(0.0f, 0.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_TRUE(result.empty());
}

TEST_F(SpatialGridTest, QueryRect_MultipleEntitiesInsideAndOutside) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));   // Inside
	auto e2 = createEntity(Vec2(15.0f, 15.0f));  // Inside
	auto e3 = createEntity(Vec2(25.0f, 25.0f));  // Outside
	auto e4 = createEntity(Vec2(-5.0f, -5.0f));  // Outside
	auto e5 = createEntity(Vec2(10.0f, 10.0f));   // Inside

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(20.0f, 20.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e5));
	EXPECT_FALSE(containsEntity(result, e3));
	EXPECT_FALSE(containsEntity(result, e4));
}

TEST_F(SpatialGridTest, QueryRect_NegativeCoordinates) {
	auto e1 = createEntity(Vec2(-5.0f, -5.0f));
	auto e2 = createEntity(Vec2(-15.0f, -15.0f));
	auto e3 = createEntity(Vec2(5.0f, 5.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(-20.0f, -20.0f), Vec2(0.0f, 0.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_FALSE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRect_VeryLargeRectangle) {
	auto e1 = createEntity(Vec2(1000.0f, 1000.0f));
	auto e2 = createEntity(Vec2(-1000.0f, -1000.0f));
	auto e3 = createEntity(Vec2(0.0f, 0.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(-10000.0f, -10000.0f), Vec2(10000.0f, 10000.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRect_ZeroWidthRectangle) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	auto e2 = createEntity(Vec2(5.0f, 10.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(5.0f, 0.0f), Vec2(5.0f, 20.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRect_ZeroHeightRectangle) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f));
	auto e2 = createEntity(Vec2(10.0f, 5.0f));

	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 5.0f), Vec2(20.0f, 5.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRect_EmptyRegistry) {
	std::vector<entt::entity> result;
	grid->QueryRect(Vec2(0.0f, 0.0f), Vec2(10.0f, 10.0f), [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_TRUE(result.empty());
}

// ============================================================================
// FindNearest Tests
// ============================================================================

TEST_F(SpatialGridTest, FindNearest_BasicFunctionality) {
	createEntity(Vec2(10.0f, 10.0f), 0);
	auto e2 = createEntity(Vec2(5.0f, 5.0f), 0);
	createEntity(Vec2(20.0f, 20.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f);

	EXPECT_EQ(result, e2);
}

TEST_F(SpatialGridTest, FindNearest_NoEntitiesInRadius) {
	createEntity(Vec2(100.0f, 100.0f), 0);
	createEntity(Vec2(200.0f, 200.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 10.0f);

	EXPECT_TRUE(result == entt::null);
}

TEST_F(SpatialGridTest, FindNearest_MultipleEntitiesIdentifiesNearest) {
	createEntity(Vec2(20.0f, 20.0f), 0);
	auto e2 = createEntity(Vec2(5.0f, 5.0f), 0);
	createEntity(Vec2(15.0f, 15.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f);

	EXPECT_EQ(result, e2);
}

TEST_F(SpatialGridTest, FindNearest_ExactDistanceAtRadiusBoundary) {
	auto e1 = createEntity(Vec2(10.0f, 0.0f), 0);  // Exactly at radius
	createEntity(Vec2(20.0f, 0.0f), 0);  // Outside radius

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 10.0f);

	// Should return null because dist < min_dist (not <=)
	EXPECT_TRUE(result == entt::null);
}

TEST_F(SpatialGridTest, FindNearest_ZeroRadius) {
	createEntity(Vec2(0.1f, 0.1f), 0);
	createEntity(Vec2(1.0f, 1.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 0.0f);

	EXPECT_TRUE(result == entt::null);
}

TEST_F(SpatialGridTest, FindNearest_EntityAtExactSearchPosition) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	createEntity(Vec2(10.0f, 10.0f), 0);

	auto result = grid->FindNearest(Vec2(5.0f, 5.0f), 100.0f);

	EXPECT_EQ(result, e1);
}

TEST_F(SpatialGridTest, FindNearest_FactionFilterSameFactionTrue) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);  // Same faction, closer
	createEntity(Vec2(10.0f, 10.0f), 1);  // Different faction
	createEntity(Vec2(15.0f, 15.0f), 0);  // Same faction, farther

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f, 0, true);

	EXPECT_EQ(result, e1);  // Should find the closest entity of faction 0
	EXPECT_TRUE(result != entt::null);
}

TEST_F(SpatialGridTest, FindNearest_FactionFilterSameFactionFalse) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 1);  // Different faction, closer
	createEntity(Vec2(10.0f, 10.0f), 0);  // Same faction
	createEntity(Vec2(15.0f, 15.0f), 1);  // Different faction, farther

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f, 0, false);

	EXPECT_EQ(result, e1);  // Should find the closest entity NOT of faction 0
}

TEST_F(SpatialGridTest, FindNearest_FactionFilterNoFilter) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	createEntity(Vec2(10.0f, 10.0f), 1);
	createEntity(Vec2(15.0f, 15.0f), 2);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f, -1, false);

	EXPECT_EQ(result, e1);  // Should find nearest regardless of faction
}

TEST_F(SpatialGridTest, FindNearest_IgnoresEntitiesWithoutFaction) {
	createEntityWithoutFaction(Vec2(5.0f, 5.0f));  // No Faction component
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);  // Has Faction

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f);

	EXPECT_EQ(result, e2);  // Should only consider entities with Faction
}

TEST_F(SpatialGridTest, FindNearest_TieBreakingMultipleEntitiesAtSameDistance) {
	// Create entities at same distance from origin
	auto e1 = createEntity(Vec2(5.0f, 0.0f), 0);
	auto e2 = createEntity(Vec2(0.0f, 5.0f), 0);
	auto e3 = createEntity(Vec2(-5.0f, 0.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f);

	// Should return one of them (implementation dependent, but should be valid)
	EXPECT_TRUE(result == e1 || result == e2 || result == e3);
	EXPECT_TRUE(result != entt::null);
}

TEST_F(SpatialGridTest, FindNearest_VeryLargeRadius) {
	auto e1 = createEntity(Vec2(1000.0f, 1000.0f), 0);
	createEntity(Vec2(2000.0f, 2000.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 10000.0f);

	EXPECT_EQ(result, e1);
}

TEST_F(SpatialGridTest, FindNearest_VerySmallRadius) {
	createEntity(Vec2(0.1f, 0.1f), 0);
	createEntity(Vec2(1.0f, 1.0f), 0);

	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 0.05f);

	EXPECT_TRUE(result == entt::null);
}

TEST_F(SpatialGridTest, FindNearest_EmptyRegistry) {
	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f);

	EXPECT_TRUE(result == entt::null);
}

TEST_F(SpatialGridTest, FindNearest_MultipleFactions) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(6.0f, 6.0f), 1);
	auto e3 = createEntity(Vec2(7.0f, 7.0f), 2);

	// Find nearest of faction 1
	auto result = grid->FindNearest(Vec2(0.0f, 0.0f), 100.0f, 1, true);

	EXPECT_EQ(result, e2);
}

// ============================================================================
// QueryRadius Tests
// ============================================================================

TEST_F(SpatialGridTest, QueryRadius_BasicFunctionality) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);
	createEntity(Vec2(20.0f, 20.0f), 0);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 15.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRadius_EmptyResults) {
	createEntity(Vec2(100.0f, 100.0f), 0);
	createEntity(Vec2(200.0f, 200.0f), 0);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 10.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_TRUE(result.empty());
}

TEST_F(SpatialGridTest, QueryRadius_BoundaryConditionsEntityExactlyAtRadius) {
	auto e1 = createEntity(Vec2(10.0f, 0.0f), 0);  // Exactly at radius
	createEntity(Vec2(11.0f, 0.0f), 0);  // Just outside radius

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 10.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e1));
}

TEST_F(SpatialGridTest, QueryRadius_MultipleEntitiesInsideAndOutside) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);   // Inside
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);  // Inside
	createEntity(Vec2(20.0f, 20.0f), 0);  // Outside
	createEntity(Vec2(-15.0f, -5.0f), 0);  // Outside

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 15.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRadius_ZeroRadius) {
	auto e1 = createEntity(Vec2(0.0f, 0.0f), 0);  // At exact position
	createEntity(Vec2(0.1f, 0.1f), 0);  // Very close but not exact

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 0.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e1));
}

TEST_F(SpatialGridTest, QueryRadius_FactionFilterSameFactionTrue) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);
	createEntity(Vec2(6.0f, 6.0f), 1);  // Different faction
	createEntity(Vec2(7.0f, 7.0f), 1);  // Different faction

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 20.0f, [&](entt::entity e) {
		result.push_back(e);
	}, 0, true);

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRadius_FactionFilterSameFactionFalse) {
	createEntity(Vec2(5.0f, 5.0f), 0);  // Same faction
	createEntity(Vec2(10.0f, 10.0f), 0);  // Same faction
	auto e3 = createEntity(Vec2(6.0f, 6.0f), 1);  // Different faction
	auto e4 = createEntity(Vec2(7.0f, 7.0f), 1);  // Different faction

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 20.0f, [&](entt::entity e) {
		result.push_back(e);
	}, 0, false);

	EXPECT_EQ(result.size(), 2);
	EXPECT_TRUE(containsEntity(result, e3));
	EXPECT_TRUE(containsEntity(result, e4));
}

TEST_F(SpatialGridTest, QueryRadius_FactionFilterNoFilter) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 1);
	auto e3 = createEntity(Vec2(15.0f, 15.0f), 2);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 25.0f, [&](entt::entity e) {
		result.push_back(e);
	}, -1, false);

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRadius_IgnoresEntitiesWithoutFaction) {
	createEntityWithoutFaction(Vec2(5.0f, 5.0f));  // No Faction component
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);  // Has Faction

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 20.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e2));
}

TEST_F(SpatialGridTest, QueryRadius_OrderIndependence) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(10.0f, 10.0f), 0);
	auto e3 = createEntity(Vec2(15.0f, 15.0f), 0);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 25.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRadius_VeryLargeRadius) {
	auto e1 = createEntity(Vec2(1000.0f, 1000.0f), 0);
	auto e2 = createEntity(Vec2(-1000.0f, -1000.0f), 0);
	auto e3 = createEntity(Vec2(0.0f, 0.0f), 0);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 10000.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 3);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
}

TEST_F(SpatialGridTest, QueryRadius_VerySmallRadius) {
	auto e1 = createEntity(Vec2(0.0f, 0.0f), 0);
	createEntity(Vec2(0.1f, 0.1f), 0);

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 0.05f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e1));
}

TEST_F(SpatialGridTest, QueryRadius_EmptyRegistry) {
	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 100.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_TRUE(result.empty());
}

TEST_F(SpatialGridTest, QueryRadius_CirclePattern) {
	// Create entities in a circle pattern
	auto e1 = createEntity(Vec2(10.0f, 0.0f), 0);   // Right
	auto e2 = createEntity(Vec2(0.0f, 10.0f), 0);   // Up
	auto e3 = createEntity(Vec2(-10.0f, 0.0f), 0);  // Left
	auto e4 = createEntity(Vec2(0.0f, -10.0f), 0);  // Down
	createEntity(Vec2(15.0f, 0.0f), 0);  // Outside radius

	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 12.0f, [&](entt::entity e) {
		result.push_back(e);
	});

	EXPECT_EQ(result.size(), 4);
	EXPECT_TRUE(containsEntity(result, e1));
	EXPECT_TRUE(containsEntity(result, e2));
	EXPECT_TRUE(containsEntity(result, e3));
	EXPECT_TRUE(containsEntity(result, e4));
}

TEST_F(SpatialGridTest, QueryRadius_MultipleFactions) {
	auto e1 = createEntity(Vec2(5.0f, 5.0f), 0);
	auto e2 = createEntity(Vec2(6.0f, 6.0f), 1);
	auto e3 = createEntity(Vec2(7.0f, 7.0f), 2);

	// Query for faction 1
	std::vector<entt::entity> result;
	grid->QueryRadius(Vec2(0.0f, 0.0f), 20.0f, [&](entt::entity e) {
		result.push_back(e);
	}, 1, true);

	EXPECT_EQ(result.size(), 1);
	EXPECT_TRUE(containsEntity(result, e2));
}
