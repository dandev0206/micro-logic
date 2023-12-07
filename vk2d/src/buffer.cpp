#include "../include/vk2d/graphics/buffer.h"

#include "../include/vk2d/core/vk2d_context_impl.h"

VK2D_BEGIN

Buffer::Buffer() :
	memory_size(0),
	buffer_size(0),
	map(nullptr) {}

Buffer::Buffer(size_t size, vk::BufferUsageFlags usage)
{
	resize(size, usage);
}

Buffer::Buffer(const Buffer& rhs)
{
	resize(rhs.memory_size, rhs.usage);
	memcpy(map, rhs.data(), rhs.memory_size);
	update();
}

Buffer::Buffer(Buffer&& rhs) VK2D_NOTHROW
{
	memory       = std::exchange(rhs.memory, nullptr);
	buffer       = std::exchange(rhs.buffer, nullptr);
	usage        = std::exchange(rhs.usage, {});
	memory_size  = std::exchange(rhs.memory_size, 0);
	buffer_size  = std::exchange(rhs.buffer_size, 0);
	map          = std::exchange(rhs.map, nullptr);
}

Buffer::~Buffer()
{
	auto& device = VK2DContext::get().device;

	device.waitIdle();
	device.destroy(buffer);
	device.freeMemory(memory);
}

vk::BufferUsageFlags Buffer::getBufferUsage() const
{
	return usage;
}

void Buffer::resize(size_t size, vk::BufferUsageFlags usage)
{
	if (buffer_size == size) return;

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(size);

	{ // recreate buffer
		device.waitIdle();
		device.destroy(buffer);

		vk::BufferCreateInfo buffer_info = {
			{},
			aligned,
			usage,
			vk::SharingMode::eExclusive
		};

		buffer = device.createBuffer(buffer_info);
		buffer_size = size;
	}

	if (memory_size < aligned) {
		device.freeMemory(memory);

		memory = inst.allocateDeviceMemory(buffer, aligned);
		map    = device.mapMemory(memory, 0, aligned);
		memory_size = aligned;
	}

	device.bindBufferMemory(buffer, memory, 0);
}

void Buffer::reserve(size_t size, vk::BufferUsageFlags usage)
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(size);

	if (aligned <= memory_size) return;

	device.freeMemory(memory);

	memory      = inst.allocateDeviceMemory(buffer, aligned);
	map         = device.mapMemory(memory, 0, aligned);
	memory_size = aligned;

	device.bindBufferMemory(buffer, memory, 0);
}

void Buffer::shrink_to_fit()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(buffer_size);
	
	if (memory_size == aligned) return;

	{ // recreate buffer
		device.destroy(buffer);

		vk::BufferCreateInfo buffer_info = {
			{},
			aligned,
			usage,
			vk::SharingMode::eExclusive
		};

		buffer = device.createBuffer(buffer_info);
	}

	{ // allocate memory 
		device.freeMemory(memory);
		
		memory      = inst.allocateDeviceMemory(buffer, aligned);
		map         = device.mapMemory(memory, 0, aligned);
		memory_size = aligned;
	}

	device.bindBufferMemory(buffer, memory, 0);
}

void Buffer::update()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	vk::MappedMemoryRange range = {
		memory, 0,  inst.alignMemorySize(memory_size)
	};

	(void)device.flushMappedMemoryRanges(1, &range);
}

void Buffer::update(const void* data, size_t size, size_t offset)
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	memcpy(map, data, size);

	vk::MappedMemoryRange range = {
		memory, offset, inst.alignMemorySize(size)
	};

	(void)device.flushMappedMemoryRanges(1, &range);
}

void* Buffer::data()
{
	VK2D_ASSERT(map && "use map() before call data()");

	return map;
}

const void* Buffer::data() const
{
	VK2D_ASSERT(map && "use map() before call data()");
	
	return map;
}

uint32_t Buffer::size() const
{
	return (uint32_t)buffer_size;
}

uint32_t Buffer::capacity() const
{
	return (uint32_t)memory_size;
}

const vk::Buffer& Buffer::getBuffer() const
{
	return buffer;
}

VK2D_END
