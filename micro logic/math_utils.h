#pragma once

#include "vector_type.h"

inline float dot(const vec2& a, const vec2& b) {
	return a.x * b.x + a.y * b.y;
}

inline float equal(const vec2& a, const vec2& b) {
	return dot(a - b, a - b) < 1e5;
}

inline vec2 lerp(const vec2& a, const vec2& b, float t) {
	return a + t * (b - a);
}

inline float unlerp(float a, float b, float x) {
	return (x - a) / (b - a);
}

inline bool in_range(float x, float min, float max) {
	return min <= x && x <= max;
}

inline float slope(const vec2& a, const vec2& b) {
	return (b.y - a.y) / (b.x - a.x);
}

inline bool is_vertical(const vec2& a, const vec2& b) {
	return std::abs(b.x - a.x) < 1e-7f;
}

inline bool is_horizontal(const vec2& a, const vec2& b) {
	return std::abs(b.y - a.y) < 1e-7f;
}