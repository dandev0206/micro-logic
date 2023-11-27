#pragma once

#include "../rect.hpp"
#include "color.hpp"

#include <vector>

VK2D_BEGIN

class Image {
public:
	Image();
	Image(const Image& rhs);
	Image(Image&& rhs) VK2D_NOTHROW;
	Image(uint32_t width, uint32_t height);
	Image(uint32_t width, uint32_t height, Color* pixels);
	Image(uint32_t width, uint32_t height, const Color& color);

	Image& operator=(const Image& rhs);
	Image& operator=(Image&& rhs) VK2D_NOTHROW;

	void loadFromFile(const char* path);
	void loadFromMemory(const Color* pixels, uint32_t width, uint32_t height);
	void loadFromMemory(const Color* pixels, const uvec2& size);

	void saveToFile(const char* path);

	void resize(uint32_t width, uint32_t height, const Color& color = Colors::Black);

	VK2D_INLINE Color& getPixel(const uvec2& pos) VK2D_NOTHROW;
	VK2D_INLINE const Color& getPixel(const uvec2& pos) const VK2D_NOTHROW;
	VK2D_INLINE Color& getPixel(uint32_t x, uint32_t y) VK2D_NOTHROW;
	VK2D_INLINE const Color& getPixel(uint32_t x, uint32_t y) const VK2D_NOTHROW;

	VK2D_INLINE void setPixel(const uvec2& pos, const Color& color) VK2D_NOTHROW;
	VK2D_INLINE void setPixel(uint32_t x, uint32_t y, const Color& color) VK2D_NOTHROW;

	Image blit(const uRect& region) const;
	Image createMask(const Color& color_from, const Color& color_to, bool include_color = true) const;

	Color* data();
	const Color* data() const;

	uvec2 size() const;

private:
	std::vector<Color> pixels;
	uint32_t width;
	uint32_t height;
};

VK2D_INLINE Color& Image::getPixel(const uvec2& pos) VK2D_NOTHROW {
	return pixels[width * pos.y + pos.x];
}

VK2D_INLINE const Color& Image::getPixel(const uvec2& pos) const VK2D_NOTHROW {
	return pixels[width * pos.y + pos.x];
}

VK2D_INLINE Color& Image::getPixel(uint32_t x, uint32_t y) VK2D_NOTHROW {
	return pixels[width * y + x];
}

VK2D_INLINE const Color& Image::getPixel(uint32_t x, uint32_t y) const VK2D_NOTHROW {
	return pixels[width * y + x];
}

VK2D_INLINE void Image::setPixel(const uvec2& pos, const Color& color) VK2D_NOTHROW {
	getPixel(pos) = color;
}

VK2D_INLINE void Image::setPixel(uint32_t x, uint32_t y, const Color& color) VK2D_NOTHROW {
	getPixel(x, y) = color;
}

VK2D_END