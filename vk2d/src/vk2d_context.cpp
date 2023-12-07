#include "../include/vk2d/vk2d_context.h"
#include "../include/vk2d/core/vk2d_context_impl.h"

#include "../include/vk2d/core/vertex.h"
#include "../include/vk2d/graphics/transform.h"
#include "shaders/shaders.h"

#ifdef VK2D_PLATFORM_WINDOWS
#define PLATFORM_SURFACE_EXT_NAME "VK_KHR_win32_surface"
#endif

static bool check_layer_support(const char* layer_name) {
	auto props = vk::enumerateInstanceLayerProperties();

	for (const auto& prop : props) {
		if (!strcmp(prop.layerName.data(), layer_name)) return true;
	}
	return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
		fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
		return VK_FALSE;
	}
	else {
		fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
		return VK_FALSE;
	}
}

static struct DebugCallbackDispatcher {
	void create(vk::Instance instance) {
		vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	}

	static int getVkHeaderVersion() { return VK_HEADER_VERSION; }

	PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = nullptr;
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = nullptr;
} debug_callback_dispatcher;

VK2D_BEGIN

VK2D_PRIV_NAME::VK2DContextImpl* VK2DContext::impl = nullptr;

VK2DContext::VK2DContext(const vk::ApplicationInfo& info, std::vector<const char*> layers, std::vector<const char*> extensions, bool debug_enable)
{
	if (impl) VK2D_ERROR("VK2D Context already exists");

	impl = new VK2D_PRIV_NAME::VK2DContextImpl();
	impl->init(info, std::move(layers), std::move(extensions), debug_enable);
}

VK2DContext::~VK2DContext()
{
	delete impl;
}

VK2D_PRIV_BEGIN

VK2DContextImpl::~VK2DContextImpl()
{
	device.waitIdle();

	basic_pipelines.clear();

	device.destroy(renderpass);
	device.destroy(command_pool);
	device.destroy(descriptor_pool);
	device.destroy(pipeline_cache);
	device.destroy();

	if (debug_callback)
		instance.destroyDebugReportCallbackEXT(debug_callback, nullptr, debug_callback_dispatcher);

	instance.destroy();
}

VK2DContextImpl::VK2DContextImpl()
{}

void VK2DContextImpl::init(const vk::ApplicationInfo& info, std::vector<const char*> layers, std::vector<const char*> extensions, bool debug_enable)
{
	if (debug_enable) {
		if (!check_layer_support("VK_LAYER_KHRONOS_validation"))
			VK2D_ERROR("VK_LAYER_KHRONOS_validation not supported");

		layers.push_back("VK_LAYER_KHRONOS_validation");
		extensions.push_back("VK_EXT_debug_report");
	}

	extensions.push_back(PLATFORM_SURFACE_EXT_NAME);
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	vk::InstanceCreateInfo create_info(
		{},
		&info,
		layers,
		extensions);

	instance = vk::createInstance(create_info);

	if (debug_enable) {
		vk::DebugReportCallbackCreateInfoEXT debug_report_create_info = {
			vk::DebugReportFlagBitsEXT::eError |
			vk::DebugReportFlagBitsEXT::eWarning |
			vk::DebugReportFlagBitsEXT::eInformation |
			vk::DebugReportFlagBitsEXT::ePerformanceWarning,
			debug_report,
			nullptr
		};

		debug_callback_dispatcher.create(instance);
		debug_callback = instance.createDebugReportCallbackEXT(debug_report_create_info, nullptr, debug_callback_dispatcher);
	} else {
		debug_callback = nullptr;
	}

	{ // select physical device
		auto physical_devices = instance.enumeratePhysicalDevices();

		for (const auto& device : physical_devices) {
			auto type = device.getProperties().deviceType;

			if (type == vk::PhysicalDeviceType::eDiscreteGpu) {
				physical_device = device;
				break;
			}
		}

		if (!physical_device)
			physical_device = physical_devices.front();

		physical_device_props = physical_device.getProperties();
		physical_device_memory_props = physical_device.getMemoryProperties();
	}

	{ // create device and queue
		auto properties = physical_device.getQueueFamilyProperties();
		float queue_priority = 1.f;
		std::vector<const char*> device_extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			"VK_EXT_extended_dynamic_state"
		};

		std::vector<vk::DeviceQueueCreateInfo> queue_infos;

		for (uint32_t i = 0; i < properties.size(); ++i) {
			if (properties[i].queueFlags & vk::QueueFlagBits::eGraphics) {
				graphics_queue_family_idx = i;
			}

			vk::DeviceQueueCreateInfo queue_info = {
				{},
				i,
				1,
				&queue_priority
			};

			queue_infos.push_back(queue_info);
		}

		vk::DeviceCreateInfo device_info = {
			{}, queue_infos, {}, device_extensions
		};

		device = physical_device.createDevice(device_info);

		for (size_t i = 0; i < properties.size(); ++i) {
			queues.push_back(device.getQueue(graphics_queue_family_idx, 0));
		}
	}

	{ // create pipeline cache
		vk::PipelineCacheCreateInfo pipeline_cache_info = {
			{}, 0, nullptr
		};

		pipeline_cache = device.createPipelineCache(pipeline_cache_info);
	}

	{ // create descriptor pool
		std::array<vk::DescriptorPoolSize, 1> pool_sizes = { {
			{ vk::DescriptorType::eCombinedImageSampler, 100 }
		} };

		vk::DescriptorPoolCreateInfo pool_info = {
			vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
			100,
			pool_sizes
		};

		descriptor_pool = device.createDescriptorPool(pool_info);
	}

	{ // create renderpass
		std::array<vk::AttachmentDescription, 1> color_attachments = { {
			{
				{},
				vk::Format::eR8G8B8A8Unorm,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::ePresentSrcKHR,
			},
		} };

		std::array<vk::AttachmentReference, 1> attachment_refs = { {
			{ 0, vk::ImageLayout::eColorAttachmentOptimal }
		} };

		std::array<vk::SubpassDescription, 1> subpasses = { {
			{
				{},
				vk::PipelineBindPoint::eGraphics,
				{},
				attachment_refs,
				{}
			}
		} };

		vk::RenderPassCreateInfo renderpass_info(
			{},
			color_attachments,
			subpasses
		);

		renderpass = device.createRenderPass(renderpass_info);
	}
	{ // create command pool
		vk::CommandPoolCreateInfo pool_info = {
			vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
			graphics_queue_family_idx,
		};

		command_pool = device.createCommandPool(pool_info);
	}

	{ // create basic pipeline0 (vertex + texture)
		PipelineBuilder builder;

		builder.addShader(glsl_shader0_vert, sizeof(glsl_shader0_vert), vk::ShaderStageFlagBits::eVertex);
		builder.addShader(glsl_shader0_frag, sizeof(glsl_shader0_frag), vk::ShaderStageFlagBits::eFragment);

		builder.addVertexInputBinding(0, sizeof(Vertex));
		builder.setVertexInputAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos));
		builder.setVertexInputAttribute(0, 1, vk::Format::eR8G8B8A8Unorm, offsetof(Vertex, color));
		builder.setVertexInputAttribute(0, 2, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));

		builder.addDynamicState(vk::DynamicState::ePrimitiveTopology);

		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(Transform));
		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(vec2));
		builder.addDescriptorSetLayoutBinding(vk::ShaderStageFlagBits::eFragment, 0, vk::DescriptorType::eCombinedImageSampler);

		basic_pipelines.emplace_back(builder.build());
	}

	{ // create basic pipeline1 (vertex only)
		PipelineBuilder builder;

		builder.addShader(glsl_shader1_vert, sizeof(glsl_shader1_vert), vk::ShaderStageFlagBits::eVertex);
		builder.addShader(glsl_shader1_frag, sizeof(glsl_shader1_frag), vk::ShaderStageFlagBits::eFragment);

		builder.addVertexInputBinding(0, sizeof(Vertex));
		builder.setVertexInputAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, pos));
		builder.setVertexInputAttribute(0, 1, vk::Format::eR8G8B8A8Unorm, offsetof(Vertex, color));

		builder.addDynamicState(vk::DynamicState::ePrimitiveTopology);

		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(Transform));

		basic_pipelines.emplace_back(builder.build());
	}

	{ // create basic pipeline2 (texture_only)
		PipelineBuilder builder;

		builder.addShader(glsl_shader2_vert, sizeof(glsl_shader2_vert), vk::ShaderStageFlagBits::eVertex);
		builder.addShader(glsl_shader2_frag, sizeof(glsl_shader2_frag), vk::ShaderStageFlagBits::eFragment);

		builder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleFan);

		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(Transform));
		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(vec4));
		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, 4 * sizeof(vec2));
		builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, 4 * sizeof(vec2));

		builder.addDescriptorSetLayoutBinding(vk::ShaderStageFlagBits::eFragment, 0, vk::DescriptorType::eCombinedImageSampler);

		basic_pipelines.emplace_back(builder.build());
	}
}

vk::CommandBuffer VK2DContextImpl::beginSingleTimeCommmand()
{
	vk::CommandBufferAllocateInfo buffer_info = {
		command_pool,
		vk::CommandBufferLevel::ePrimary,
		1,
	};

	auto cmd_buffer = device.allocateCommandBuffers(buffer_info).front();

	cmd_buffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

	return cmd_buffer;
}

void VK2DContextImpl::endSingleTimeCommmand(vk::CommandBuffer cmd_buffer)
{
	cmd_buffer.end();

	vk::SubmitInfo submit_info = {
		0, nullptr,
		nullptr,
		1, &cmd_buffer
	};

	VK2D_CHECK(queues[graphics_queue_family_idx].submit(1, &submit_info, nullptr));
	queues[graphics_queue_family_idx].waitIdle();
	device.freeCommandBuffers(command_pool, 1, &cmd_buffer);
}

void VK2DContextImpl::transitionImageLayout(vk::CommandBuffer cmd_buffer, vk::Image image, vk::Format format, vk::ImageLayout old_layout, vk::ImageLayout new_layout)
{
	vk::ImageMemoryBarrier barrier = {
		vk::AccessFlagBits::eNone,
		vk::AccessFlagBits::eNone,
		old_layout,
		new_layout,
		VK_QUEUE_FAMILY_IGNORED,
		VK_QUEUE_FAMILY_IGNORED,
		image,
		{
			vk::ImageAspectFlagBits::eColor,
			0, 1, 0, 1
		}
	};


	vk::PipelineStageFlags src_stage;
	vk::PipelineStageFlags dst_stage;

	if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eTransferSrcOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (old_layout == vk::ImageLayout::eTransferDstOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eNone;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else if (old_layout == vk::ImageLayout::eTransferSrcOptimal && new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		src_stage = vk::PipelineStageFlagBits::eTransfer;
		dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
	}
	else {
		VK2D_ERROR("unsupported layout transition");
	}

	cmd_buffer.pipelineBarrier(
		src_stage,
		dst_stage,
		{},
		0, nullptr,
		0, nullptr,
		1, &barrier);
}

void VK2DContextImpl::copyBuffer(vk::CommandBuffer cmd_buffer, vk::Buffer dst, vk::Buffer src, vk::DeviceSize dst_off, vk::DeviceSize src_off, vk::DeviceSize size)
{
	vk::BufferCopy region = {
		src_off, dst_off, size
	};

	cmd_buffer.copyBuffer(src, dst, 1, &region);
}

std::pair<vk::DeviceMemory, vk::Buffer> VK2DContextImpl::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags props)
{
	vk::BufferCreateInfo buffer_info = {
		{}, size, usage, vk::SharingMode::eExclusive
	};

	auto buffer = device.createBuffer(buffer_info);

	auto req = device.getBufferMemoryRequirements(buffer);

	vk::MemoryAllocateInfo memory_info = {
		req.size, findMemoryType(req.memoryTypeBits, props)
	};

	auto memory = device.allocateMemory(memory_info);

	device.bindBufferMemory(buffer, memory, 0);

	return std::make_pair(memory, buffer);
}

vk::DeviceMemory VK2DContextImpl::allocateDeviceMemory(vk::Buffer buffer, vk::DeviceSize size)
{
	vk::MemoryRequirements req;
	device.getBufferMemoryRequirements(buffer, &req);

	vk::MemoryAllocateInfo memory_info = {
		req.size,
		findMemoryType(req.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible)
	};

	return device.allocateMemory(memory_info);
}

vk::DeviceSize VK2DContextImpl::alignMemorySize(vk::DeviceSize size) const
{
	auto align = physical_device_props.limits.nonCoherentAtomSize;
	return ((size - 1) / align + 1) * align;
}

uint32_t VK2DContextImpl::findMemoryType(uint32_t type_filter, vk::MemoryPropertyFlags props) const
{
	const auto& mem_props = physical_device_memory_props;
	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		if (type_filter & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & props) == props) {
			return i;
		}
	}

	VK2D_ERROR("failed to find suitable memory type");
	return 0;
}

VK2D_PRIV_END
VK2D_END