#pragma once

#include "../../include/vk2d/system/window.h"

#include "../../include/vk2d/vk_instance.h"
#include "../../include/vk2d/graphics/render_states.h"

#include <vulkan/vulkan.hpp>
#include <string>
#include <queue>

VK2D_BEGIN

class WindowImplBase {
public:
	WindowImplBase() :
		events(),
		mouse_prev(0, 0),
		position(0, 0),
		size(0, 0),
		fb_size(0, 0),
		size_limit_min(0, 0),
		size_limit_max(0, 0),
		transparency(0.f),
		visible(false),
		minimized(false),
		maximized(false),
		focussed(false),
		user_ptr(nullptr),
		surface(nullptr),
		swapchain(nullptr),
		present_queue_family_idx(~0),
		present_mode(vk::PresentModeKHR::eFifo),
		image_format(vk::Format::eR8G8B8A8Unorm),
		frame_idx(0),
		semaphore_idx(0),
		update_swapchain(true),
		render_begin(false)
	{}

	struct WindowFrame {
		vk::Image       image;
		vk::ImageView   image_view;
		vk::Framebuffer frameBuffer;

		vk::CommandPool   cmd_pool;
		vk::CommandBuffer cmd_buffer;

		vk::Fence fence;

		RenderStates states;
	};

	struct FrameSemaphore {
		vk::Semaphore image_acquired;
		vk::Semaphore render_complete;
	};

	std::queue<Event> events;
	std::string       title;
	ivec2             mouse_prev;
	ivec2             position;
	uvec2             size;
	uvec2             fb_size;
	uvec2             size_limit_min;
	uvec2             size_limit_max;
	float             transparency;
	bool              visible;
	bool              minimized;
	bool              maximized;
	bool              focussed;
	void*             user_ptr;

	vk::SurfaceKHR                    surface;
	vk::SwapchainKHR                  swapchain;
	uint32_t                          present_queue_family_idx;
	vk::SurfaceCapabilitiesKHR        capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR>   present_modes;
	vk::PresentModeKHR                present_mode;
	vk::Format                        image_format;

	
	std::vector<WindowFrame>    frames;
	std::vector<FrameSemaphore> semaphores;
	
	uint32_t frame_idx;
	uint32_t semaphore_idx;
	bool     update_swapchain;
	bool     render_begin;


	void init() {
		auto& inst = VKInstance::get();

		auto properties = inst.physical_device.getQueueFamilyProperties();

		for (uint32_t i = 0; i < properties.size(); ++i) {
			if (inst.physical_device.getSurfaceSupportKHR(i, surface)) {
				present_queue_family_idx = i;
			}
		}

		capabilities  = inst.physical_device.getSurfaceCapabilitiesKHR(surface);
		formats       = inst.physical_device.getSurfaceFormatsKHR(surface);
		present_modes = inst.physical_device.getSurfacePresentModesKHR(surface);

		recreate_swapchain();
	}

	void destroy() {
		auto& inst   = VKInstance::get();
		auto& device = inst.device;

		device.waitIdle();
		destroy_window_frame();
		destroy_frame_semaphores();

		device.destroy(swapchain);
		inst.instance.destroy(surface);
	}

	void recreate_swapchain() {
		auto& inst   = VKInstance::get();
		auto& device = inst.device;

		device.waitIdle();
		destroy_window_frame();
		destroy_frame_semaphores();

		capabilities = inst.physical_device.getSurfaceCapabilitiesKHR(surface);

		auto extent        = capabilities.currentExtent;
		auto old_swapchain = swapchain;

		if (extent.width == 0 || extent.height == 0) return;

		vk::SwapchainCreateInfoKHR swapchain_info(
			{},
			surface,
			get_swapchain_image_count(),
			image_format,
			vk::ColorSpaceKHR::eSrgbNonlinear,
			get_swapchain_extent(),
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			{}, {}, {},
			vk::SurfaceTransformFlagBitsKHR::eIdentity,
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			present_mode,
			true,
			old_swapchain
		);

		uint32_t queue_families[2] = {
			inst.graphics_queue_family_idx,
			present_queue_family_idx
		};

		if (inst.graphics_queue_family_idx == present_queue_family_idx) {
			swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
			swapchain_info.queueFamilyIndexCount = 0;
			swapchain_info.pQueueFamilyIndices = nullptr;
		}
		else {
			swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
			swapchain_info.queueFamilyIndexCount = 2;
			swapchain_info.pQueueFamilyIndices = queue_families;
		}

		swapchain = inst.device.createSwapchainKHR(swapchain_info);
		device.destroy(old_swapchain);

		create_window_frame();
		create_frame_semaphores();

		update_swapchain = false;
	}

	void acquireSwapchainImage() {
		auto& device = VKInstance::get().device;

		if (update_swapchain)
			recreate_swapchain();

		while(true) {
			auto res = device.acquireNextImageKHR(
				swapchain,
				UINT64_MAX,
				semaphores[semaphore_idx].image_acquired,
				nullptr,
				&frame_idx);

			if (res != vk::Result::eSuboptimalKHR && res != vk::Result::eErrorOutOfDateKHR) return;

			recreate_swapchain();
		};
	}

private:
	vk::Extent2D get_swapchain_extent() {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		} else {
			vk::Extent2D actualExtent{ size.x, size.y };

			actualExtent.width  = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	uint32_t get_swapchain_image_count() {
		uint32_t image_count = capabilities.minImageCount + 1;

		if (0 < capabilities.maxImageCount && capabilities.maxImageCount < image_count) {
			image_count = capabilities.maxImageCount;
		}

		return image_count;
	}

	void create_window_frame() {
		auto& inst   = VKInstance::get();
		auto& device = inst.device;

		auto images = device.getSwapchainImagesKHR(swapchain);

		for (auto& image : images) {
			vk::ImageViewCreateInfo image_view_info = {
				{},
				image,
				vk::ImageViewType::e2D,
				image_format,
				{
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity,
					vk::ComponentSwizzle::eIdentity
				},
				{
					vk::ImageAspectFlagBits::eColor,
					0, 1, 0, 1
				}
			};

			auto image_view = device.createImageView(image_view_info);

			vk::FramebufferCreateInfo frame_buffer_info = {
				{},
				inst.renderpass,
				1, &image_view,
				capabilities.currentExtent.width,
				capabilities.currentExtent.height, 
				1,
			};

			vk::CommandPoolCreateInfo command_pool_info = {
				vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
				inst.graphics_queue_family_idx
			};

			auto command_pool = device.createCommandPool(command_pool_info);

			vk::CommandBufferAllocateInfo command_buffer_info = {
				command_pool, vk::CommandBufferLevel::ePrimary, 1
			};

			vk::FenceCreateInfo fence_info = {
				vk::FenceCreateFlagBits::eSignaled
			};

			WindowFrame frame;
			frame.image          = image;
			frame.image_view     = image_view;
			frame.frameBuffer    = device.createFramebuffer(frame_buffer_info);
			frame.cmd_pool       = command_pool;
			frame.cmd_buffer     = device.allocateCommandBuffers(command_buffer_info).front();
			frame.fence          = device.createFence(fence_info);

			frames.push_back(frame);
		}
	}

	void create_frame_semaphores() {
		auto& device = VKInstance::get().device;

		for (int i = 0; i < frames.size(); ++i) {
			FrameSemaphore semaphore;

			semaphore.image_acquired  = device.createSemaphore({});
			semaphore.render_complete = device.createSemaphore({});

			semaphores.emplace_back(semaphore);
		}

		semaphore_idx = 0;
	}

	void destroy_window_frame() {
		auto& device = VKInstance::get().device;

		for (auto& frame : frames) {
			device.destroy(frame.image_view);
			device.destroy(frame.frameBuffer);
			device.freeCommandBuffers(frame.cmd_pool, 1, &frame.cmd_buffer);
			device.destroy(frame.cmd_pool);
			device.destroy(frame.fence);
		}

		frames.clear();
	}

	void destroy_frame_semaphores() {
		auto& device = VKInstance::get().device;

		for (auto& semaphore : semaphores) {
			device.destroy(semaphore.image_acquired);
			device.destroy(semaphore.render_complete);
		}

		semaphores.clear();
	}
};

VK2D_END