#include "../include/vk2d/graphics/pipeline.h"

#include "../include/vk2d/vk_instance.h"

VK2D_BEGIN

Pipeline::Pipeline(Pipeline&& rhs) VK2D_NOTHROW
{
	pipeline              = std::exchange(rhs.pipeline, nullptr);
	pipeline_layout       = rhs.pipeline_layout;
	descriptor_set_layout = rhs.descriptor_set_layout;

	rhs.pipeline_layout.reset();
	rhs.descriptor_set_layout.reset();
}

Pipeline::~Pipeline()
{
	destroy();
}

Pipeline& Pipeline::operator=(Pipeline&& rhs) VK2D_NOTHROW
{
	pipeline = std::exchange(rhs.pipeline, nullptr);
	rhs.pipeline_layout.swap(pipeline_layout);
	rhs.descriptor_set_layout.swap(descriptor_set_layout);
	
	return *this;
}

void Pipeline::destroy()
{
	auto& device = VKInstance::get().device;

	device.waitIdle();
	device.destroy(std::exchange(pipeline, nullptr));

	if (pipeline_layout.use_count() == 1) {
		device.destroy(*pipeline_layout);
		pipeline_layout.reset();
	}

	if (descriptor_set_layout.use_count() == 1) {
		device.destroy(*descriptor_set_layout);
		descriptor_set_layout.reset();
	}

	for (auto& shader_module : shader_modules)
		device.destroy(shader_module);
}

bool Pipeline::operator==(const Pipeline& rhs) const
{
	return pipeline == rhs.pipeline;
}

bool Pipeline::operator!=(const Pipeline& rhs) const
{
	return pipeline != rhs.pipeline;
}

PipelineBuilder::PipelineBuilder() :
	polygon_mode(vk::PolygonMode::eFill),
	primitive_topology(vk::PrimitiveTopology::eTriangleList) {

	dynamic_states.emplace_back(vk::DynamicState::eViewport);
	dynamic_states.emplace_back(vk::DynamicState::eScissor);
}

Pipeline PipelineBuilder::build()
{
	auto& inst   = VKInstance::get();
	auto& device = inst.device;

	Pipeline shader;

	shader.pipeline_layout       = getPipelineLayout();
	shader.descriptor_set_layout = descriptor_set_layout;
	for (auto& shader_module : shader_modules)
		shader.shader_modules.emplace_back(shader_module.module);

	{ // create graphics pipeline
		vk::PipelineVertexInputStateCreateInfo vertex_info = {
			{}, vertex_inputs, vertex_attributes
		};

		vk::PipelineInputAssemblyStateCreateInfo ia_info = {
			{}, primitive_topology
		};

		vk::PipelineViewportStateCreateInfo viewport_info = {
			{}, 1, nullptr, 1, nullptr
		};

		vk::PipelineRasterizationStateCreateInfo raster_info = {
			{},
			false,
			false,
			polygon_mode,
			vk::CullModeFlagBits::eNone,
			vk::FrontFace::eCounterClockwise,
			false,
			0.f,
			0.f,
			0.f,
			1.f
		};

		vk::PipelineMultisampleStateCreateInfo ms_info = {
			{}, vk::SampleCountFlagBits::e1,
		};

		std::array<vk::PipelineColorBlendAttachmentState, 1> color_attachment = {{
			{
				true,
				vk::BlendFactor::eSrcAlpha,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendOp::eAdd,
				vk::BlendFactor::eOne,
				vk::BlendFactor::eOneMinusSrcAlpha,
				vk::BlendOp::eAdd,
				vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
			}
		}};

		vk::PipelineDepthStencilStateCreateInfo depth_info = {};

		vk::PipelineColorBlendStateCreateInfo blend_info = {
			{}, false, vk::LogicOp::eClear, color_attachment
		};

		vk::PipelineDynamicStateCreateInfo dynamic_state = {
			{}, dynamic_states
		};

		vk::GraphicsPipelineCreateInfo info = {
			vk::PipelineCreateFlagBits(),
			shader_modules,
			&vertex_info,
			&ia_info,
			nullptr,
			&viewport_info,
			&raster_info,
			&ms_info,
			&depth_info,
			&blend_info,
			&dynamic_state,
			*shader.pipeline_layout,
			inst.renderpass,
			0
		};

		auto result = device.createGraphicsPipeline(inst.pipeline_cache, info);

		VK2D_ASSERT(result.result == vk::Result::eSuccess && "pipeline build failed");
		shader.pipeline = result.value;
	}

	return shader;
}

std::shared_ptr<vk::PipelineLayout> PipelineBuilder::getPipelineLayout()
{
	auto& device = VKInstance::get().device;

	if (!pipeline_layout) {
		vk::DescriptorSetLayoutCreateInfo descriptor_set_info = {
			{}, descriptor_set_layouts_bindings
		};

		auto desc_layout = device.createDescriptorSetLayout(descriptor_set_info);
		descriptor_set_layout = std::make_shared<vk::DescriptorSetLayout>(desc_layout);

		std::array<vk::DescriptorSetLayout, 1> descriptor_set_layouts = {{
			*descriptor_set_layout
		}};

		vk::PipelineLayoutCreateInfo pipeline_layout_info = {
			{}, descriptor_set_layouts, push_constants
		};

		auto pipe_layout = device.createPipelineLayout(pipeline_layout_info);
		pipeline_layout = std::make_shared<vk::PipelineLayout>(pipe_layout);
	}

	return pipeline_layout;
}

void PipelineBuilder::addShader(const uint32_t* bytes, size_t size_in_byte, vk::ShaderStageFlagBits stage)
{
	auto& device = VKInstance::get().device;

	for (const auto& module : shader_modules) {
		if (module.stage == stage)
			VK2D_ERROR("duplicate shader stage not allowed");
	}

	vk::PipelineShaderStageCreateInfo shader_stage_info = {
		{},
		stage,
		device.createShaderModule({ {}, size_in_byte, (uint32_t*)bytes }),
		"main"
	};

	shader_modules.push_back(shader_stage_info);
}

void PipelineBuilder::addShader(const std::vector<uint32_t>& bytes, vk::ShaderStageFlagBits stage)
{
	addShader(bytes.data(), bytes.size() * sizeof(uint32_t), stage);
}

void PipelineBuilder::addVertexInputBinding(uint32_t binding, uint32_t size)
{
	vertex_inputs.emplace_back(binding, size, vk::VertexInputRate::eVertex);
}

void PipelineBuilder::setVertexInputAttribute(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset)
{
	vertex_attributes.emplace_back(location, binding, format, offset);
}

void PipelineBuilder::addDynamicState(vk::DynamicState state)
{
	dynamic_states.emplace_back(state);
}

void PipelineBuilder::setPolygonMode(vk::PolygonMode mode)
{
	polygon_mode = mode;
}

void PipelineBuilder::setPrimitiveTopology(vk::PrimitiveTopology topology)
{
	primitive_topology = topology;
}

void PipelineBuilder::addDescriptorSetLayoutBinding(vk::ShaderStageFlags stage, uint32_t binding, vk::DescriptorType type)
{
	descriptor_set_layouts_bindings.emplace_back(binding, type, 1, stage, nullptr);
}

void PipelineBuilder::addPushConstant(vk::ShaderStageFlagBits stage, uint32_t size)
{
	for (auto& pc : push_constants) {
		if (pc.stageFlags == stage) {
			pc.size += size;
			return;
		}
	}

	push_constants.emplace_back(stage, 0, size);
}


VK2D_END