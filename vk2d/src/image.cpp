#include "../include/vk2d/graphics/image.h"

#define __STDC_LIB_EXT1__
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../thirdparty/stb/stb_image.h"
#include "../thirdparty/stb/stb_image_write.h"

VK2D_BEGIN

Image::Image() :
	width(0),
	height(0) 
{}

Image::Image(const Image& rhs) :
	pixels(rhs.pixels),
	width(rhs.width),
	height(rhs.height) 
{}

Image::Image(Image&& rhs) VK2D_NOTHROW :
	pixels(std::move(rhs.pixels)),
	width(std::exchange(rhs.width, 0)),
	height(std::exchange(rhs.height, 0))
{}

Image::Image(uint32_t width, uint32_t height) :
	pixels(width * height),
	width(width),
	height(height) 
{}

Image::Image(uint32_t width, uint32_t height, Color* pixels) :
	pixels(width* height),
	width(width),
	height(height) 
{
	memcpy(this->pixels.data(), pixels, width * height * sizeof(Color));
}

Image::Image(uint32_t width, uint32_t height, const Color& color) :
	pixels(width* height, color),
	width(width),
	height(height)
{}

Image& Image::operator=(const Image& rhs)
{
	pixels = rhs.pixels;
	width  = rhs.width;
	height = rhs.height;
	return *this;
}

Image& Image::operator=(Image&& rhs) VK2D_NOTHROW
{
	pixels = std::move(rhs.pixels);
	width  = std::exchange(rhs.width, 0);
	height = std::exchange(rhs.height, 0);
	return *this;
}

void Image::loadFromFile(const char* path)
{
	int w, h, comp = 0;
	Color* pixels = (Color*)stbi_load(path, &w, &h, &comp, STBI_rgb_alpha);

	loadFromMemory(pixels, w, h);

	stbi_image_free(pixels);
}

void Image::loadFromMemory(const Color* pixels, uint32_t width, uint32_t height)
{
	auto size = width * height;

	this->pixels.resize(size);
	this->width  = width;
	this->height = height;

	memcpy(this->pixels.data(), pixels, size * sizeof(Color));
}

void Image::loadFromMemory(const Color* pixels, const uvec2& size)
{
	loadFromMemory(pixels, size.x, size.y);
}

void Image::saveToFile(const char* path)
{
	stbi_write_png(path, width, height, 4, pixels.data(), 0);
}

void Image::resize(uint32_t width, uint32_t height, const Color& color)
{
	this->width  = width;
	this->height = height;

	pixels.clear();
	pixels.resize(width * height, color); 
}

Image Image::blit(const uRect& region) const
{
	VK2D_ASSERT(region.left + region.width < width);
	VK2D_ASSERT(region.top + region.height < height);

	Image new_image(region.width, region.height);

	size_t stride = region.width * sizeof(Color);
	for (uint32_t h = 0; h < region.height; ++h) {
		const Color* begin = &getPixel(region.left, region.top + h);
		memcpy(&new_image.getPixel(0, h), begin, stride);
	}

	return new_image;
}

Image Image::createMask(const Color& color_from, const Color& color_to, bool include_color) const
{
	Image result(width, height);

	auto iter = result.pixels.begin();

	for (auto& pixel : pixels) {
		if (include_color == (pixel == color_from)) {
			*iter = color_to;
		} else {
			*iter = pixel;
		}
		++iter;
	}

	return result;
}

Color* Image::data()
{
	return pixels.data();
}

const Color* Image::data() const
{
	return pixels.data();
}

uvec2 Image::size() const
{
	return { width, height };
}

VK2D_END