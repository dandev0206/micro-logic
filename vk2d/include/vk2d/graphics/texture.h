#pragma once

#include "image.h"
#include <vulkan/vulkan.hpp>

VK2D_BEGIN

class Texture {
	friend class RenderTexture;

public:
	static uvec2 getMaximumSize();

public:
	Texture();
	Texture(const Texture& rhs);
	Texture(Texture&& rhs) VK2D_NOTHROW;
	Texture(const Image& image);
	Texture(uint32_t width, uint32_t height, const Color& color = Colors::Black);
	~Texture();

	Texture& operator=(const Texture& rhs);
	Texture& operator=(Texture&& rhs) VK2D_NOTHROW;

	void resize(uint32_t width, uint32_t height, const Color& color = Colors::Black);
	void resize(const uvec2& size);
	
	void destroy();

	void update();
	void update(const Color* pixels, const uvec2& size, const uvec2& offset = {});
	void update(const Image& image, const uvec2& offset = {});
	void update(const Texture& texture, const uvec2& offset = {});

	Image getImage() const;

	VK2D_INLINE Color* data();
	VK2D_INLINE const Color* data() const;
	
	VK2D_INLINE uvec2 size() const;

	VK2D_INLINE bool empty() const;

	VK2D_INLINE vk::DescriptorSet getDescriptorSet() const;

private:
	void copyImageToBuffer(vk::CommandBuffer cmd_buffer) const;
	void invalidateStagingBuffer() const;
	void resize_impl(uint32_t width, uint32_t height, const Color& color, vk::ImageUsageFlags usage);

private:
	mutable vk::DeviceMemory staging_buffer_memory;
	mutable vk::Buffer       staging_buffer;
	mutable Color*           map;

	vk::DeviceMemory  memory;
	vk::Image         image;
	vk::ImageView     image_view;
	vk::DescriptorSet descriptor_set;
	vk::Sampler       sampler;
	vk::DeviceSize    allocated;
	uint32_t          width;
	uint32_t          height;
};

VK2D_INLINE Color* Texture::data()
{
	return map;
}

VK2D_INLINE const Color* Texture::data() const
{
	return map;
}

VK2D_INLINE uvec2 Texture::size() const
{
	return { width, height };
}

VK2D_INLINE bool Texture::empty() const
{
	return !image;
}

VK2D_INLINE vk::DescriptorSet Texture::getDescriptorSet() const
{
	return descriptor_set;
}

VK2D_END