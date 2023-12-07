#include "../include/vk2d/graphics/vertex_buffer.h"

#include "../include/vk2d/core/vk2d_context_impl.h"
#include "../include/vk2d/graphics/render_target.h"
#include "../include/vk2d/graphics/render_states.h"

VK2D_BEGIN

VertexBuffer::VertexBuffer() :
	allocated(0),
	vertex_size(0),
	map(nullptr),
	primitive_topology(vk::PrimitiveTopology::eTriangleList) {}

VertexBuffer::VertexBuffer(size_t size) :
	VertexBuffer() {
	resize(size);
}

VertexBuffer::VertexBuffer(vk::PrimitiveTopology topology) :
	VertexBuffer() {
	primitive_topology = topology;
}

VertexBuffer::VertexBuffer(size_t size, vk::PrimitiveTopology topology) :
	VertexBuffer()
{
	primitive_topology = topology;
	resize(size);
}

VertexBuffer::VertexBuffer(const VertexBuffer& rhs)
{
	resize(rhs.vertex_size);
	memcpy(map, rhs.map, vertex_size * sizeof(Vertex));
	update();
}

VertexBuffer::~VertexBuffer()
{
	auto& device = VK2DContext::get().device;

	device.waitIdle();
	device.destroy(buffer);
	device.freeMemory(memory);
}

vk::PrimitiveTopology VertexBuffer::getPrimitiveTopology() const
{
	return primitive_topology;
}

void VertexBuffer::setPrimitiveTopology(vk::PrimitiveTopology topology)
{
	primitive_topology = topology;
}

VertexBuffer::VertexBuffer(VertexBuffer&& rhs) VK2D_NOTHROW
{
	memory = std::exchange(rhs.memory, nullptr);
	buffer = std::exchange(rhs.buffer, nullptr);
	map    = std::exchange(rhs.map, nullptr);

	allocated   = std::exchange(rhs.allocated, 0);
	vertex_size = std::exchange(rhs.vertex_size, 0);
}

void VertexBuffer::resize(size_t size)
{
	if (vertex_size == size) return;

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(sizeof(Vertex) * size);

	{ // recreate buffer
		device.waitIdle();
		device.destroy(buffer);

		vk::BufferCreateInfo buffer_info = {
			{},
			aligned,
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::SharingMode::eExclusive
		};

		buffer      = device.createBuffer(buffer_info);
		vertex_size = size;
	}

	if (allocated < aligned) { 
		device.freeMemory(memory);
		
		memory    = inst.allocateDeviceMemory(buffer, aligned);
		map       = (Vertex*)device.mapMemory(memory, 0, aligned);
		allocated = aligned;
	}

	device.bindBufferMemory(buffer, memory, 0);
}

void VertexBuffer::reserve(size_t size)
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(sizeof(Vertex) * size);

	if (aligned <= allocated) return;

	device.freeMemory(memory);
	
	memory    = inst.allocateDeviceMemory(buffer, aligned);
	map       = (Vertex*)device.mapMemory(memory, 0, aligned);
	allocated = aligned;

	device.bindBufferMemory(buffer, memory, 0);
}

void VertexBuffer::shrink_to_fit()
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;
	auto aligned = inst.alignMemorySize(sizeof(Vertex) * vertex_size);
	
	if (allocated == aligned) return;

	{ // recreate buffer
		device.destroy(buffer);

		vk::BufferCreateInfo buffer_info = {
			{},
			aligned,
			vk::BufferUsageFlagBits::eVertexBuffer,
			vk::SharingMode::eExclusive
		};

		buffer = device.createBuffer(buffer_info);
	}

	{ // allocate memory 
		device.freeMemory(memory);
		
		memory    = inst.allocateDeviceMemory(buffer, aligned);
		map       = (Vertex*)device.mapMemory(memory, 0, aligned);
		allocated = aligned;
	}

	device.bindBufferMemory(buffer, memory, 0);
}

void VertexBuffer::update()
{
	if (vertex_size == 0) return;

	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	vk::MappedMemoryRange range = {
		memory, 0,  inst.alignMemorySize(vertex_size * sizeof(Vertex))
	};

	(void)device.flushMappedMemoryRanges(1, &range);
}

void VertexBuffer::update(const Vertex* data, size_t size, size_t offset)
{
	auto& inst   = VK2DContext::get();
	auto& device = inst.device;

	memcpy(map, data, size * sizeof(Vertex));

	vk::MappedMemoryRange range = {
		memory, offset, inst.alignMemorySize(size * sizeof(Vertex))
	};

	(void)device.flushMappedMemoryRanges(1, &range);
}

Vertex* VertexBuffer::data()
{
	return map;
}

const Vertex* VertexBuffer::data() const
{
	return map;
}

Vertex* VertexBuffer::begin()
{
	return map;
}

const Vertex* VertexBuffer::begin() const
{
	return map;
}

Vertex* VertexBuffer::end()
{
	return map + vertex_size;
}

const Vertex* VertexBuffer::end() const
{
	return map + vertex_size;
}

size_t VertexBuffer::size() const
{
	return vertex_size;
}

size_t VertexBuffer::capacity() const
{
	return allocated / sizeof(Vertex);
}

const vk::Buffer& VertexBuffer::getBuffer() const
{
	return buffer;
}

void VertexBuffer::draw(RenderTarget& target, RenderStates& states) const
{
	auto& inst = VK2DContext::get();

	if (!buffer) return;
	
	auto cmd_buffer = target.getCommandBuffer();

	const Pipeline* pipeline = nullptr;

	 // bind pipeline
	if (states.options.pipeline.has_value())
		pipeline = states.options.pipeline.value();
	else if (states.options.texture.has_value())
		pipeline = &inst.basic_pipelines[0];
	else 
		pipeline = &inst.basic_pipelines[1];
	
	cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);

	states.bindTVS(cmd_buffer, *pipeline->pipeline_layout);

	vk::DeviceSize offset = 0;
	cmd_buffer.bindVertexBuffers(0, 1, &buffer, &offset);
	cmd_buffer.setPrimitiveTopology(primitive_topology);

	if (states.options.texture.has_value()) {
		auto desc_set = states.options.texture.value()->getDescriptorSet();
		cmd_buffer.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, 
			*pipeline->pipeline_layout,
			0, 1, &desc_set, 0, nullptr);

		vec2 texture_size = (vec2)states.options.texture.value()->size();

		cmd_buffer.pushConstants(
			*pipeline->pipeline_layout,
			vk::ShaderStageFlagBits::eVertex,
			sizeof(Transform),
			sizeof(vec2),
			&texture_size);
	}
	
	if (states.options.buffer.has_value()) { // has index buffer
		auto& buffer = *states.options.buffer.value();
		cmd_buffer.bindIndexBuffer(buffer.getBuffer(), 0, vk::IndexType::eUint32);
		cmd_buffer.drawIndexed(buffer.size() / sizeof(uint32_t), 1, 0, 0, 0);
	} else {
		cmd_buffer.draw((uint32_t)vertex_size, 1, 0, 0);
	}
}

VK2D_END
