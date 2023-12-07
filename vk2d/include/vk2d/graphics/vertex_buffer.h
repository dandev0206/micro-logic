#pragma once

#include "../core/include_vulkan.h"
#include "../core/vertex.h"
#include "drawable.h"

VK2D_BEGIN

class VertexBuffer : public Drawable {
public:
	VertexBuffer();
	VertexBuffer(size_t size);
	VertexBuffer(vk::PrimitiveTopology topology);
	VertexBuffer(size_t size, vk::PrimitiveTopology topology);
	VertexBuffer(const VertexBuffer& rhs) ;
	VertexBuffer(VertexBuffer&& rhs) VK2D_NOTHROW;
	~VertexBuffer();

	vk::PrimitiveTopology getPrimitiveTopology() const;
	void setPrimitiveTopology(vk::PrimitiveTopology topology);

	void resize(size_t size);
	void reserve(size_t size);
	void shrink_to_fit();

	void update();
	void update(const Vertex* data, size_t size, size_t offset = 0);

	Vertex* data();
	const Vertex* data() const;

	Vertex* begin();
	const Vertex* begin() const;
	Vertex* end();
	const Vertex* end() const;

	size_t size() const;
	size_t capacity() const;

	const vk::Buffer& getBuffer() const;

private:
	void draw(RenderTarget& target, RenderStates& states)const override;

private:
	vk::DeviceMemory memory;
	vk::Buffer       buffer;
	vk::DeviceSize   allocated;
	size_t           vertex_size;
	Vertex*          map;

	vk::PrimitiveTopology primitive_topology;
};

VK2D_END