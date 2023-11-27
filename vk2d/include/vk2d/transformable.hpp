#pragma once

#include "transform.hpp"

VK2D_BEGIN

class Transformable abstract {
public:
	VK2D_INLINE Transformable();
	Transformable(const Transformable&) = default;
	Transformable(Transformable&&) VK2D_NOTHROW = default;

	Transformable& operator=(const Transformable&) = default;
	Transformable& operator=(Transformable&&) VK2D_NOTHROW = default;

	VK2D_INLINE vec2 getPosition() const;
	VK2D_INLINE void setPosition(float pos_x, float pos_y);
	VK2D_INLINE void setPosition(const vec2& pos);

	VK2D_INLINE void move(float delta_x, float delta_y);
	VK2D_INLINE void move(const vec2& delta);

	VK2D_INLINE vec2 getOrigin() const;
	VK2D_INLINE void setOrigin(float origin_x, float origin_y);
	VK2D_INLINE void setOrigin(const vec2& origin);

	VK2D_INLINE vec2 getScale() const;
	VK2D_INLINE void setScale(float scale);
	VK2D_INLINE void setScale(float scale_x, float scale_y);
	VK2D_INLINE void setScale(const vec2& scale);

	VK2D_INLINE float getRotation() const;
	VK2D_INLINE void setRotation(float degree);

	VK2D_INLINE void rotate(float degree);

protected:
	VK2D_INLINE Transform getTransform() const;

private:
	vec2 pos;
	vec2 origin;
	vec2 scale;
	float rotation;
};

inline Transformable::Transformable() :
	pos(),
	origin(),
	scale(1.f, 1.f),
	rotation() {}

VK2D_INLINE vec2 Transformable::getPosition() const
{
	return pos;
}

VK2D_INLINE void Transformable::setPosition(float pos_x, float pos_y)
{
	pos = { pos_x, pos_y };
}

VK2D_INLINE void Transformable::setPosition(const vec2& pos)
{
	this->pos = pos;
}

VK2D_INLINE void Transformable::move(float delta_x, float delta_y)
{
	pos += vec2(delta_x, delta_y);
}

VK2D_INLINE void Transformable::move(const vec2& delta)
{
	this->pos += delta;
}

VK2D_INLINE vec2 Transformable::getOrigin() const
{
	return origin;
}

VK2D_INLINE void Transformable::setOrigin(float origin_x, float origin_y)
{
	origin = { origin_x, origin_y };
}

VK2D_INLINE void Transformable::setOrigin(const vec2& origin)
{
	this->origin = origin;
}

VK2D_INLINE vec2 Transformable::getScale() const
{
	return scale;
}

VK2D_INLINE void Transformable::setScale(float scale)
{
	this->scale = { scale, scale };
}

VK2D_INLINE void Transformable::setScale(float scale_x, float scale_y)
{
	scale = { scale_x, scale_y };
}

VK2D_INLINE void Transformable::setScale(const vec2& scale)
{
	this->scale = scale;
}

VK2D_INLINE float Transformable::getRotation() const
{
	return rotation;
}

VK2D_INLINE void Transformable::setRotation(float degree)
{
	rotation = degree;
}

VK2D_INLINE void Transformable::rotate(float degree)
{
	rotation += degree;
}

VK2D_INLINE Transform Transformable::getTransform() const
{
	return Transform::identity().
		translate(pos).
		rotate(rotation).
		scale(scale).
		translate(origin);
}

VK2D_END