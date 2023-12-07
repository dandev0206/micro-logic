#include "../include/vk2d/graphics/transform.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/constants.hpp>

VK2D_BEGIN

Transform Transform::identity()
{
	return Transform(
		1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,
		0.f, 0.f, 1.f
	);
}

Transform::Transform() :
	mat(1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f) {}

Transform::Transform(float a00, float a01, float a02, 
					 float a10, float a11, float a12, 
					 float a20, float a21, float a22) :
	mat(a00, a10, 0.f, a20, 
		a01, a11, 0.f, a21, 
		0.f, 0.f, 1.f, 0.f,
		a02, a21, 0.f, a22) {}

Transform::Transform(const float mat[9])
{
	memcpy(this, mat, sizeof(mat4x4));
}

Transform::Transform(const mat4x4& rhs) :
	mat(rhs) {}

Transform& Transform::translate(float x, float y)
{
	return translate({ x, y });
}

Transform& Transform::translate(const vec2& offset)
{
	mat = glm::translate(mat, vec3(offset, 0.f));
	return *this;
}

Transform& Transform::rotate(float degree)
{
	float rad = degree * glm::pi<float>() / 180.f;
	float cos = std::cos(rad);
	float sin = std::sin(rad);

	*this *= Transform(
		cos, -sin, 0.f,
		sin, cos, 0.f,
		0.f, 0.f, 1.f
	);

	return *this;
}

Transform& Transform::rotate(float degree, float center_x, float center_y)
{
	float rad = degree * glm::pi<float>() / 180.f;
	float cos = std::cos(rad);
	float sin = std::sin(rad);

	*this *= Transform(
		cos, -sin, center_x * (1 - cos) + center_y * sin,
		sin, cos, center_y * (1 - cos) - center_x * sin,
		0.f, 0.f, 1.f
	);

	return *this;
}

Transform& Transform::rotate(float degree, const vec2& center)
{
	return rotate(degree, center.x, center.y);
}

Transform& Transform::scale(float scale)
{
	*this *= Transform(
		scale, 0.f, 0.f,
		0.f, scale, 0.f,
		0.f, 0.f, 1.f
	);

	return *this;
}

Transform& Transform::scale(float scale_x, float scale_y)
{
	return scale({ scale_x, scale_y });
}

Transform& Transform::scale(const vec2& scale)
{
	mat = glm::scale(mat, vec3(scale, 1.f));
	return *this;
}

Transform& Transform::operator=(const Transform& rhs)
{
	memcpy(this, &rhs, sizeof(mat4x4));
	return *this;
}

Transform& Transform::operator*=(const Transform& rhs)
{
	mat *= rhs.mat;
	return *this;
}

Transform Transform::operator*(const Transform& rhs) const
{
	return { mat * rhs.mat };
}

vec2 Transform::operator*(const vec2& rhs) const
{
	vec4 temp(rhs.x, rhs.y, 0.f, 1.f);
	temp = mat * temp;
	return { temp.x, temp.y };
}

bool Transform::operator==(const Transform& rhs) const
{
	return !(*this != rhs);
}

bool Transform::operator!=(const Transform& rhs) const
{
	return memcmp(this, &rhs, sizeof(mat4x4));
}

VK2D_END
