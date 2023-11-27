#pragma once

#include "vector_type.h"

#define AABB_INLINE inline

struct AABB
{
	using point_type = vec2;
	using rect_type = Rect;

	AABB() = default;

	AABB_INLINE AABB(const point_type& min, const point_type& max) :
		min(min), max(max) { }

	AABB_INLINE AABB(const rect_type& rect) :
		min(rect.getPosition()), max(min + rect.getSize()) { }

	AABB_INLINE AABB(float min_x, float min_y, float max_x, float max_y) :
		min(min_x, min_y), max(max_x, max_y) { }

	AABB_INLINE vec2 size() const {
		return { max.x - min.x, max.y - min.y };
	}

	AABB_INLINE float width() const {
		return max.x - min.x;
	}

	AABB_INLINE float height() const {
		return max.y - min.y;
	}

	AABB_INLINE point_type center() const {
		return { (min.x + max.x) / 2.f, (min.y + max.y) / 2.f };
	}

	AABB_INLINE float area() const {
		return (max.x - min.x) * (max.y - min.y);
	}

	AABB_INLINE bool contain(const AABB& rhs) const {
		return min.x <= rhs.min.x && min.y <= rhs.min.y && max.x >= rhs.max.x && max.y >= rhs.max.y;
	}

	AABB_INLINE bool contain(const point_type& v) const {
		return min.x <= v.x && v.x <= max.x && min.y <= v.y && v.y <= max.y;
	}

	AABB_INLINE bool overlap(const AABB& rhs) const {
		return !(min.x > rhs.max.x || max.x < rhs.min.x || min.y > rhs.max.y || max.y < rhs.min.y);
	}

	AABB_INLINE AABB union_of(const AABB& rhs) const {
		return {
			std::min(min.x, rhs.min.x),
			std::min(min.y, rhs.min.y),
			std::max(max.x, rhs.max.x),
			std::max(max.y, rhs.max.y)
		};
	}

	AABB_INLINE bool operator==(const AABB& rhs) const {
		return min == rhs.min && max == rhs.max;
	}

	AABB_INLINE bool operator!=(const AABB& rhs) const {
		return !operator==(rhs);
	}

	AABB_INLINE operator rect_type() const {
		return { min, max - min };
	}

	point_type min;
	point_type max;
};