#pragma once

#include "vector_type.h"

enum class Direction {
	Up    = 0,
	Right = 1,
	Down  = 2,
	Left  = 3,
};

inline float to_degree(Direction dir) {
	return (float)dir * 90.f;
}

inline Direction invert_dir(Direction dir) {
	switch (dir) {
	case Direction::Up:    return Direction::Up;
	case Direction::Right: return Direction::Left;
	case Direction::Down:  return Direction::Down;
	case Direction::Left:  return Direction::Right;
	default: return (Direction)(-1);
	}
}

inline Direction rotate_cw(Direction dir) {
	return (Direction)(((int)dir + 1) % 4);
}

inline Direction rotate_ccw(Direction dir) {
	return (Direction)(((int)dir + 3) % 4);
}

inline Direction rotate_dir(Direction dir, Direction rotation) {
	return (Direction)(((int)dir + (int)rotation) % 4);
}

inline vec2 rotate_vector(const vec2& v, Direction dir) {
	switch (dir) {
	case Direction::Up:    return { v.x, v.y };
	case Direction::Right: return { -v.y, v.x };
	case Direction::Down:  return { -v.x, -v.y };
	case Direction::Left:  return { v.y, -v.x };
	default: return {};
	}
}

inline Rect rotate_rect(const Rect& rect, Direction dir) {
	switch (dir) {
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