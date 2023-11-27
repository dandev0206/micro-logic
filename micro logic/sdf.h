#include "vector_type.h"

inline vec2 matrix_transform(float a00, float a01, float a10, float a11, const vec2& v) {
	return { a00 * v.x + a01 * v.y, a10 * v.x + a11 * v.y };
}

inline float SDF_circle(const vec2& p, const vec2& pos, float r) {
	return length(p - pos) - r;
}

inline float SDF_oriendted_box(const vec2& p, const vec2& a, const vec2& b, float th)
{
	float l = length(b - a);
	vec2  d = (b - a) / l;
	vec2  q = (p - (a + b) * 0.5f);
	q = matrix_transform(d.x, -d.y, d.y, d.x, q);
	q = abs(q) - vec2(l, th) * 0.5f;
	return length(max(q, 0.f)) + std::min(std::max(q.x, q.y), 0.f);
}