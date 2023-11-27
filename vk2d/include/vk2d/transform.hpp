#pragma once

#include "vector_type.h"

VK2D_BEGIN

class Transform {
public:
	static Transform identity();
public:	
	Transform();
	Transform(float a00, float a01, float a02,
			  float a10, float a11, float a12,
			  float a20, float a21, float a22);
	Transform(const float mat[9]);
	Transform(const mat4x4& rhs);

	Transform& translate(float x, float y);
	Transform& translate(const vec2& offset);

	Transform& rotate(float degree);
	Transform& rotate(float degree, float center_x, float center_y);
	Transform& rotate(float degree, const vec2& center);

	Transform& scale(float scale);
	Transform& scale(float scale_x, float scale_y);
	Transform& scale(const vec2& scale);

	Transform& operator=(const Transform& rhs);
	Transform& operator*=(const Transform& rhs);
	Transform operator*(const Transform& rhs) const;
	vec2 operator*(const vec2& rhs) const;

	bool operator==(const Transform& rhs) const;
	bool operator!=(const Transform& rhs) const;

private:
	mat4x4 mat;
};

VK2D_END