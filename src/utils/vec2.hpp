#pragma once

#include <cmath>
#include <cereal/cereal.hpp>

struct Vec2 {
	float x, y;

	// Constructors
	Vec2() : x(0.0f), y(0.0f) {}
	Vec2(float x, float y) : x(x), y(y) {}

	// Arithmetic operators
	Vec2 operator+(const Vec2& other) const {
		return Vec2(x + other.x, y + other.y);
	}

	Vec2 operator-(const Vec2& other) const {
		return Vec2(x - other.x, y - other.y);
	}

	Vec2 operator*(float scalar) const {
		return Vec2(x * scalar, y * scalar);
	}

	Vec2 operator/(float scalar) const {
		return Vec2(x / scalar, y / scalar);
	}

	Vec2 operator-() const {
		return Vec2(-x, -y);
	}

	// Compound assignment operators
	Vec2& operator+=(const Vec2& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	Vec2& operator-=(const Vec2& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	Vec2& operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		return *this;
	}

	Vec2& operator/=(float scalar) {
		x /= scalar;
		y /= scalar;
		return *this;
	}

	// Comparison operators
	bool operator==(const Vec2& other) const {
		return x == other.x && y == other.y;
	}

	bool operator!=(const Vec2& other) const {
		return x != other.x || y != other.y;
	}

	// Serialization for Cereal
	template<class Archive>
	void serialize(Archive &archive) {
		archive(CEREAL_NVP(x), CEREAL_NVP(y));
	}

	// Instance methods
	float length_squared() const {
		return Vec2::dot(*this, *this);
	}

	float length() const {
		return std::sqrt(length_squared());
	}

	// Static utility functions
	static float dot(const Vec2& a, const Vec2& b) {
		return a.x * b.x + a.y * b.y;
	}

	static float cross(const Vec2& a, const Vec2& b) {
		return a.x * b.y - a.y * b.x;
	}

	static float distance_squared(const Vec2& a, const Vec2& b) {
		return (b - a).length_squared();
	}

	static float distance(const Vec2& a, const Vec2& b) {
		return (b - a).length();
	}

	static Vec2 normalize(const Vec2& v) {
		float len = v.length();
		if (len < 0.0001f) {
			return Vec2{0.0f, 0.0f};
		}
		return v / len;
	}

	static Vec2 direction_to(const Vec2& from, const Vec2& to) {
		Vec2 dir = to - from;
		return Vec2::normalize(dir);
	}

	static bool point_in_rect(const Vec2& point, const Vec2& rect_min, const Vec2& rect_max) {
		return point.x >= rect_min.x && point.x <= rect_max.x &&
		       point.y >= rect_min.y && point.y <= rect_max.y;
	}
};

// Scalar multiplication from left side (e.g., 2.0f * vec)
inline Vec2 operator*(float scalar, const Vec2& vec) {
	return vec * scalar;
}

