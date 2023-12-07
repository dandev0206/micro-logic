#include "../include/vk2d/graphics/texture.h"

#include "../include/vk2d/core/vk2d_context_impl.h"

VK2D_BEGIN

Texture::Texture() :
	map(nullptr),
	allocated(0),
	width(0),
	height(0)
{}

uvec2 Texture::getMaximumSize()
{
	auto& inst = VK2DContext::get();
	auto size  = inst.physical_device_props.limits.maxImageDimension2D;
	
	return uvec2(size, size);
}

Texture::Texture(const Texture& rhs)
{
	resize(rhs.width, rhs.height);
	update(rhs);
}

Texture::Texture(Texture&& rhs) VK2D_NOTHROW : 
	staging_buffer_memory(std::exchange(rhs.staging_buffer_memory, nullptr)),
	staging_buffer(std::exchange(rhs.staging_buffer, nullptr)),
	map(std::exchange(rhs.map, nullptr)),
	memory(std::exchange(rhs.memory, nullptr)),
	image(std::exchange(rhs.image, nullptr)),
	image_view(std::exchange(rhs.image_view, nullptr)),
	descriptor_set(std::exchange(rhs.descriptor_set, nullptr)),
	sampler(std::exchange(rhs.sampler, nullptr)),
	allocated(std::exchange(rhs.allocated, 0)),
	width(std::exchange(rhs.width, 0)),
	height(std::exchange(rhs.height, 0)) {}

Texture::Texture(const Image& image)
{
	resize(image.size());
	memcpy(map, image.data(), width * height * sizeof(Color));
	update();
}

Texture::Texture(uint32_t width, uint32_t height, const Color& color)
{
	resize(width, height, color);
	update();
}

Texture::~Texture()
{
	destroy();
}

Texture& Texture::operator=(const Texture& rhs)
{
	if (&rhs == this) return *this;

	destroy();
	resize(rhs.width, rhs.height);
	update(rhs);

	return *this;
}

Texture& Texture::operator=(Texture&& rhs) VK2D_NOTHROW
{
	if (&rhs == this) return *this;

	destroy();

	staging_buffer_memory = std::exchange(rhs.staging_buffer_memory, nullptr);
	staging_buffer        = std::exchange(rhs.staging_buffer, nullptr);
	map                   = std::exchange(rhs.map, nullptr);
	memory                = std::exchange(rhs.memory, nullptr);
	image                 = std::exchange(rhs.image, nullptr);
	image_view            = std::exchange(rhs.image_view, nullptr);
	descriptor_set        = std::exchange(rhs.descriptor_set, nullptr);
	sampler               = std::exchange(rhs.sampler, nullptr);
	allocated             = std::exchange(rhs.allocated, 0);
	width                 = std::exchange(rhs.width, 0);
	height                = std::exchange(rhs.height, 0);

	return *this;
}

void Texture::resize(uint32_t width, uint32_t height, const Color& color)
{
	resize_impl(width, height, color,
		vk::ImageUsageFlagBits::eSampled | 
		vk::ImageUsageFlagBits::eTransferSrc |
		vk::ImageUsageFlagBits::eTransferDst);
}

void Texture::resize(const uvec2& size)
{
	resize(size.x, size.y);
}

void Texture::destroy()
{
	if (empty()) return;

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	device.waitIdle();
	device.destroy(staging_buffer);
	device.free(staging_buffer_memory);
	device.free(inst.descriptor_pool, 1, &descriptor_set);
	device.destroy(image_view);
	device.destroy(image);
	device.destroy(sampler);
	device.free(memory);

	staging_buffer        = nullptr;
	staging_buffer_memory = nullptr;
	descriptor_set        = nullptr;
	image_view            = nullptr;
	image                 = nullptr;
	sampler               = nullptr;
	memory                = nullptr;
}

void Texture::update()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto size    = width * height;

	vk::MappedMemoryRange range = {
		staging_buffer_memory, 0, size * sizeof(Color)
	};

	VK2D_CHECK(device.flushMappedMemoryRanges(1, &range));

	auto cmd_buffer = inst.beginSingleTimeCommmand();

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal);

	vk::BufferImageCopy image_copy = {
		0, 0, 0,
		{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
		{ 0, 0, 0 },
		{ width, height, 1}
	};

	cmd_buffer.copyBufferToImage(
		staging_buffer, 
		image, 
		vk::ImageLayout::eTransferDstOptimal, 
		1, &image_copy);

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	inst.endSingleTimeCommmand(cmd_buffer);
}

void Texture::update(const Color* pixels, const uvec2& size, const uvec2& offset)
{
	VK2D_ASSERT(pixels);
	VK2D_ASSERT(offset.x + size.x <= width);
	VK2D_ASSERT(offset.y + size.y <= height);

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	vk::MappedMemoryRange range = {
		staging_buffer_memory, 0, VK_WHOLE_SIZE
	};

	VK2D_CHECK(device.invalidateMappedMemoryRanges(1, &range));

	for (uint32_t h = 0; h < size.y; ++h) {
		auto dst_off = (h + offset.y) * width + offset.x;
		memcpy(map + dst_off, pixels + h * size.x, size.x * sizeof(Color));
	}

	VK2D_CHECK(device.flushMappedMemoryRanges(1, &range));

	auto cmd_buffer = inst.beginSingleTimeCommmand();

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal);

	vk::BufferImageCopy image_copy = {
		0, 0, 0,
		{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
		{ 0, 0, 0 },
		{ width, height, 1}
	};

	cmd_buffer.copyBufferToImage(
		staging_buffer,
		image,
		vk::ImageLayout::eTransferDstOptimal,
		1, &image_copy);

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	inst.endSingleTimeCommmand(cmd_buffer);
}

void Texture::update(const Image& image, const uvec2& offset)
{
	update(image.data(), image.size(), offset);
}

void Texture::update(const Texture& texture, const uvec2& offset)
{
	VK2D_ASSERT(!texture.empty());
	VK2D_ASSERT(offset.x + texture.width <= width);
	VK2D_ASSERT(offset.y + texture.height <= height);

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	auto cmd_buffer = inst.beginSingleTimeCommmand();

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferDstOptimal);

	inst.transitionImageLayout(
		cmd_buffer,
		texture.image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferSrcOptimal);

	vk::ImageCopy image_copy = {
		{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
		{ 0, 0, 0 },
		{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
		{ (int32_t)offset.x, (int32_t)offset.y, 0 },
		{ texture.width, texture.height, 1}
	};

	cmd_buffer.copyImage(
		texture.image,
		vk::ImageLayout::eTransferSrcOptimal,
		image,
		vk::ImageLayout::eTransferDstOptimal,
		1, &image_copy
	);

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eTransferDstOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	inst.transitionImageLayout(
		cmd_buffer,
		texture.image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);

	copyImageToBuffer(cmd_buffer);

	inst.endSingleTimeCommmand(cmd_buffer);

	vk::MappedMemoryRange range = {
		staging_buffer_memory, 0, VK_WHOLE_SIZE
	};

	VK2D_CHECK(device.invalidateMappedMemoryRanges(1, &range));
}

Image Texture::getImage() const
{
	invalidateStagingBuffer();

	Image new_image(width, height);
	memcpy(new_image.data(), map, width * height * sizeof(Color));

	return new_image;
}

void Texture::copyImageToBuffer(vk::CommandBuffer cmd_buffer) const
{
	auto& inst = VK2DContext::get();

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eTransferSrcOptimal);

	vk::BufferImageCopy buffer_copy = {
		0, 0, 0,
		{ vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
		{ 0, 0, 0 },
		{ width, height, 1}
	};

	cmd_buffer.copyImageToBuffer(
		image,
		vk::ImageLayout::eTransferSrcOptimal,
		staging_buffer,
		1, &buffer_copy);

	inst.transitionImageLayout(
		cmd_buffer,
		image,
		vk::Format::eR8G8B8A8Unorm,
		vk::ImageLayout::eTransferSrcOptimal,
		vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::invalidateStagingBuffer() const
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto size    = width * height;

	auto cmd_buffer = inst.beginSingleTimeCommmand();

	copyImageToBuffer(cmd_buffer);

	inst.endSingleTimeCommmand(cmd_buffer);

	vk::MappedMemoryRange range = {
		staging_buffer_memory, 0, VK_WHOLE_SIZE
	};

	VK2D_CHECK(device.invalidateMappedMemoryRanges(1, &range));
}

void Texture::resize_impl(uint32_t width, uint32_t height, const Color& color, vk::ImageUsageFlags usage)
{
	VK2D_ASSERT(width != 0 && height != 0);

	if (!empty()) destroy();
	
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto  size   = width * height;

	this->width  = width;
	this->height = height;
	
	{ // create staging buffer
		auto buffer = inst.createBuffer(size * sizeof(Color), 
			vk::BufferUsageFlagBits::eTransferSrc | 
			vk::BufferUsageFlagBits::eTransferDst, 
			vk::MemoryPropertyFlagBits::eHostVisible);

		staging_buffer_memory = buffer.first;
		staging_buffer        = buffer.second;

		map = (Color*)device.mapMemory(staging_buffer_memory, 0, size * sizeof(Color));
	}
	{ // create image
		vk::ImageCreateInfo image_info = {
			{},
			vk::ImageType::e2D,
			vk::Format::eR8G8B8A8Unorm,
			{ width, height, 1 },
			1,
			1,
			vk::SampleCountFlagBits::e1,
			vk::ImageTiling::eOptimal,
			usage,
			vk::SharingMode::eExclusive,
			0, nullptr,
			vk::ImageLayout::eUndefined
		};

		image = device.createImage(image_info);

		auto req = device.getImageMemoryRequirements(image);

		vk::MemoryAllocateInfo memory_info = {
			req.size, inst.findMemoryType(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
		};

		memory = device.allocateMemory(memory_info);
		device.bindImageMemory(image, memory, 0);
	}
	{ // create image view
		vk::ImageViewCreateInfo view_info = {
			{},
			image,
			vk::ImageViewType::e2D,
			vk::Format::eR8G8B8A8Unorm,
			{
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
				vk::ComponentSwizzle::eIdentity,
			},
			{ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
		};

		image_view = device.createImageView(view_info);
	}
	{ // create sampler
		vk::SamplerCreateInfo sampler_info = {
			{},
			vk::Filter::eLinear,
			vk::Filter::eLinear,
			vk::SamplerMipmapMode::eLinear,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			vk::SamplerAddressMode::eRepeat,
			false,
			false,
			1.f,
			false,
			vk::CompareOp::eNever,
			-1000.f,
			1000.f,
		};

		sampler = device.createSampler(sampler_info);
	}
	{ // create descriptor set
		vk::DescriptorSetAllocateInfo set_info = {
			inst.descriptor_pool,
			1, inst.basic_pipelines[0].descriptor_set_layout.get()
		};

		descriptor_set = device.allocateDescriptorSets(set_info).front();

		vk::DescriptorImageInfo image_info = {
			sampler,
			image_view,
			vk::ImageLayout::eShaderReadOnlyOptimal,
		};

		vk::WriteDescriptorSet write_info = {
			descriptor_set, 
			0,
			0,
			1,
			vk::DescriptorType::eCombinedImageSampler,
			&image_info,
		};

		device.updateDescriptorSets(1, &write_info, 0, nullptr);
	}

	for (uint32_t i = 0; i < size; ++i)
		map[i] = color;
	update();
}

VK2D_END