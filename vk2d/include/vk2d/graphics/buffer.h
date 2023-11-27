#pragma once

#include <vulkan/vulkan.hpp>
#include "../vk2d_config.h"

VK2D_BEGIN

class Buffer {
public:
	Buffer();
	Buffer(size_t size, vk::BufferUsageFlags usage);
	Buffer(const Buffer& rhs);
	Buffer(Buffer&& rhs) VK2D_NOTHROW;
	~Buffer();

	vk::BufferUsageFlags getBufferUsage() const;

	void resize(size_t size, vk::BufferUsageFlags usage);
	void reserve(size_t size, vk::BufferUsageFlags usage);
	void shrink_to_fit();

	void update();
	void update(const void* data, size_t size, size_t offset = 0);

	void* data();
	const void* data() const;

	uint32_t size() const;
	uint32_t capacity() const;

	const vk::Buffer& getBuffer() const;

private:
	vk::DeviceMemory     memory;
	vk::Buffer           buffer;
	vk::BufferUsageFlags usage;
	vk::DeviceSize       memory_size;
	vk::DeviceSize       buffer_size;
	void*                map;
};

VK2D_END