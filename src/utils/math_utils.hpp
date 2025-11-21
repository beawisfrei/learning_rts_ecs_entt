#pragma once

#include "../components/components.hpp"
#include "vec2.hpp"
#include <cmath>

namespace MathUtils {
	// Calculate distance between two Vec2 points
	inline float distance(const Vec2& a, const Vec2& b) {
		Vec2 diff = b - a;
		return std::sqrt(dot(diff, diff));
	}

	// Calculate length of a Vec2 vector
	inline float length(const Vec2& v) {
		return std::sqrt(dot(v, v));
	}

	// Normalize a Vec2 vector
	inline Vec2 normalize(const Vec2& v) {
		float len = length(v);
		if (len < 0.0001f) {
			return Vec2{0.0f, 0.0f};
		}
		return v / len;
	}

	// Get direction from point a to point b
	inline Vec2 direction_to(const Vec2& from, const Vec2& to) {
		Vec2 dir = to - from;
		return normalize(dir);
	}

	// Check if point is inside rectangle
	inline bool point_in_rect(const Vec2& point, const Vec2& rect_min, const Vec2& rect_max) {
		return point.x >= rect_min.x && point.x <= rect_max.x &&
		       point.y >= rect_min.y && point.y <= rect_max.y;
	}
}

