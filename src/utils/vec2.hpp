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
};

// Scalar multiplication from left side (e.g., 2.0f * vec)
inline Vec2 operator*(float scalar, const Vec2& vec) {
	return vec * scalar;
}

// Dot product
inline float dot(const Vec2& a, const Vec2& b) {
	return a.x * b.x + a.y * b.y;
}

// Cross product (returns scalar for 2D)
inline float cross(const Vec2& a, const Vec2& b) {
	return a.x * b.y - a.y * b.x;
}

