#pragma once

#include "vector_type.h"

VK2D_BEGIN

template <class T>
struct _Rect_vector { using type = void; };

template <>
struct _Rect_vector<float> { using type = vec2; };

template <>
struct _Rect_vector<int32_t> { using type = ivec2; };

template <>
struct _Rect_vector<uint32_t> { using type = uvec2; };

template <class T>
struct RectBase {
	using vector_type = typename _Rect_vector<T>::type;

	RectBase() = default;
	RectBase(const RectBase&) = default;
	RectBase(RectBase&&) = default;

	RectBase(const T& left, const T& top, const T& width, const T& height) :
		left(left),
		top(top),
		width(width),
		height(height) {}

	RectBase(const vector_type& pos, const vector_type& size) :
		left(pos.x),
		top(pos.y),
		width(size.x),
		height(size.y) {}

	template <class U>
	RectBase(const RectBase<U>& rhs) VK2D_NOTHROW :
		left(static_cast<T>(rhs.left)), 
		top(static_cast<T>(rhs.top)),
		width(static_cast<T>(rhs.width)),
		height(static_cast<T>(rhs.height)) {}

	RectBase& operator=(const RectBase&) = default;
	RectBase& operator=(RectBase&&) = default;

	vector_type getPosition() const VK2D_NOTHROW {
		return { left, top };
	}

	void setPosition(const vector_type& pos) VK2D_NOTHROW {
		left = pos.x;
		top  = pos.y;
	}

	vector_type getSize() const VK2D_NOTHROW {
		return { width, height };
	}

	void setSize(const vector_type& size) VK2D_NOTHROW {
		width  = size.x;
		height = size.y;
	}

	vector_type center() const VK2D_NOTHROW {
		return {
			left + width / static_cast<T>(2.f),
			top + height / static_cast<T>(2.f),
		};
	}

	float area() const VK2D_NOTHROW {
		return width * height;
	}

	bool contain(const vector_type& pos) const VK2D_NOTHROW {
		return left <= pos.x && pos.x <= left + width && top <= pos.y && pos.y <= top + height;
	}

	T left;
	T top;
	T width;
	T height;
};

using Rect  = RectBase<float>;
using iRect = RectBase<int32_t>;
using uRect = RectBase<uint32_t>;

VK2D_END