#pragma once

#include "vector_type.h"

enum class Direction {
	Up    = 0,
	Right = 1,
	Down  = 2,
	Left  = 3,
};

inline float to_degree(Direction path) {
	return (float)path * 90.f;
}

inline Direction invert_dir(Direction path) {
	switch (path) {
	case Direction::Up:    return Direction::Up;
	case Direction::Right: return Direction::Left;
	case Direction::Down:  return Direction::Down;
	case Direction::Left:  return Direction::Right;
	default: return (Direction)(-1);
	}
}

inline Direction rotate_cw(Direction path) {
	return (Direction)(((int)path + 1) % 4);
}

inline Direction rotate_ccw(Direction path) {
	return (Direction)(((int)path + 3) % 4);
}

inline Direction rotate_dir(Direction path, Direction rotation) {
	return (Direction)(((int)path + (int)rotation) % 4);
}

inline vec2 rotate_vector(const vec2& v, Direction path) {
	switch (path) {
	case Direction::Up:    return { v.x, v.y };
	case Direction::Right: return { -v.y, v.x };
	case Direction::Down:  return { -v.x, -v.y };
	case Direction::Left:  return { v.y, -v.x };
	default: return {};
	}
}

inline Rect rotate_rect(const Rect& rect, Direction path) {
	switch (path) {
	case Direction::Up:
		return rect;
	case Direction::Right:
		return { -rect.top - rect.height, -rect.left - rect.width, rect.height, rect.width };
	case Direction::Down:
		return { -rect.left - rect.width, -rect.top - rect.height, rect.width, rect.height };
	case Direction::Left:
		return { -rect.top - rect.height, -rect.left - rect.width, rect.height, rect.width };
	default: return {};
	}
}