#pragma once

#include "../core/include_vulkan.h"

VK2D_BEGIN

enum class ShaderStage {
	Vertex,
	Fragment
};

class Pipeline {
	friend class PipelineBuilder;

public:
	Pipeline() = default;
	Pipeline(Pipeline&& rhs) VK2D_NOTHROW;
	~Pipeline();

	Pipeline& operator=(Pipeline&& rhs) VK2D_NOTHROW;

	void destroy();

	bool operator==(const Pipeline& rhs) const;
	bool operator!=(const Pipeline& rhs) const;

	vk::Pipeline                             pipeline;
	std::shared_ptr<vk::DescriptorSetLayout> descriptor_set_layout;
	std::shared_ptr<vk::PipelineLayout>      pipeline_layout;
	std::vector<vk::ShaderModule>            shader_modules;
};

class PipelineBuilder {
public:
	PipelineBuilder();

	Pipeline build();

	std::shared_ptr<vk::PipelineLayout> getPipelineLayout();
	void setPipelineLayout(std::shared_ptr<vk::PipelineLayout>& layout);

	void addShader(const uint32_t* bytes, size_t size_in_byte, vk::ShaderStageFlagBits stage);
	void addShader(const std::vector<uint32_t>& bytes, vk::ShaderStageFlagBits stage);

	void addVertexInputBinding(uint32_t binding, uint32_t size);
	void setVertexInputAttribute(uint32_t binding, uint32_t location, vk::Format format, uint32_t offset);

	void addDynamicState(vk::DynamicState state);
	void setPolygonMode(vk::PolygonMode mode);
	void setPrimitiveTopology(vk::PrimitiveTopology topology);

	void addDescriptorSetLayoutBinding(vk::ShaderStageFlags stage, uint32_t binding, vk::DescriptorType type);
	void addPushConstant(vk::ShaderStageFlagBits stage, uint32_t size);

private:
	std::shared_ptr<vk::PipelineLayout>      pipeline_layout;
	std::shared_ptr<vk::DescriptorSetLayout> descriptor_set_layout;

	std::vector<vk::PipelineShaderStageCreateInfo>   shader_modules;
	std::vector<vk::VertexInputBindingDescription>   vertex_inputs;
	std::vector<vk::VertexInputAttributeDescription> vertex_attributes;

	std::vector<vk::DynamicState>                    dynamic_states;

	std::vector<vk::DescriptorSetLayoutBinding>      descriptor_set_layouts_bindings;
	std::vector<vk::PushConstantRange>               push_constants;

	vk::PolygonMode       polygon_mode;
	vk::PrimitiveTopology primitive_topology;
};

VK2D_END