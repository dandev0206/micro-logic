#include "../include/vk2d/graphics/render_target.h"

VK2D_BEGIN

Color RenderTarget::getClearColor()
{
	auto arr = clear_value.color.float32;
	return Color(arr[0], arr[1], arr[2], arr[3]);
}

void RenderTarget::setClearColor(const Color& color)
{
	clear_value = {
		vk::ClearColorValue{ 
			255.99f * color.r, 
			255.99f * color.g, 
			255.99f * color.b, 
			255.99f * color.a 
		}
	};
}

VK2D_END