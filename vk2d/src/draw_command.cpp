#define _USE_MATH_DEFINES

#include "../include/vk2d/graphics/draw_list.h"

#include "../include/vk2d/core/vk2d_context_impl.h"
#include "../include/vk2d/graphics/render_target.h"
#include "../include/vk2d/graphics/render_states.h"
#include <math.h>

VK2D_BEGIN

DrawCommand::DrawCommand() :
	update_buffers(true)
{}

void DrawCommand::addLine(const vec2& p0, const vec2& p1, float width, const Color& col)
{
	uint32_t idx = reservePrims(4, 6);

	vec2 normal = glm::normalize(p1 - p0);
	normal = vec2(-normal.y, normal.x);

	vertices.emplace_back(p0 + normal * width, col);
	vertices.emplace_back(p0 - normal * width, col);
	vertices.emplace_back(p1 + normal * width, col);
	vertices.emplace_back(p1 - normal * width, col);

	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 3);
}

void DrawCommand::addTriangle(const vec2& p0, const vec2& p1, const vec2& p2, float width, const Color& col)
{
	addLine(p0, p1, width, col);
	addLine(p1, p2, width, col);
	addLine(p2, p0, width, col);
}

void DrawCommand::addFilledTriangle(const vec2& p0, const vec2& p1, const vec2& p2, const Color& col)
{
	uint32_t idx = reservePrims(4, 6);

	vertices.emplace_back(p0, col);
	vertices.emplace_back(p1, col);
	vertices.emplace_back(p2, col);

	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + 2);
}

void DrawCommand::addRect(const vec2& pos, const vec2& size, float width, const Color& col)
{
	vec2 p0 = pos;
	vec2 p1 = pos + vec2(size.x, 0.f);
	vec2 p2 = pos + vec2(0.f, size.y);
	vec2 p3 = pos + size;

	addLine(p0, p1, width, col);
	addLine(p1, p3, width, col);
	addLine(p3, p2, width, col);
	addLine(p2, p0, width, col);
}

void DrawCommand::addFilledRect(const vec2& pos, const vec2& size, const Color& col)
{
	uint32_t idx = reservePrims(4, 6);

	vertices.emplace_back(pos, col);
	vertices.emplace_back(pos + vec2(size.x, 0.f), col);
	vertices.emplace_back(pos + size, col);
	vertices.emplace_back(pos + vec2(0.f, size.y), col);

	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 3);
}

void DrawCommand::addFilledRect(const vec2& pos, const vec2& size, const Color& col, const Rect& uv_rect)
{
	uint32_t idx = reservePrims(4, 6);

	vec2 uv_pos = uv_rect.getPosition();
	vec2 uv_size = uv_rect.getSize();

	vertices.emplace_back(pos, col, uv_pos);
	vertices.emplace_back(pos + vec2(size.x, 0.f), col, uv_pos + vec2(uv_size.x, 0.f));
	vertices.emplace_back(pos + vec2(0.f, size.y), col, uv_pos + vec2(0.f, uv_size.y));
	vertices.emplace_back(pos + size, col, uv_pos + uv_size);

	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 0);
	indices.emplace_back(idx + 2);
	indices.emplace_back(idx + 3);
}

void DrawCommand::addCircle(const vec2& pos, float radius, float width, const Color& col, uint32_t seg_count)
{
	addEllipse(pos, vec2(radius), width, col, seg_count);
}

void DrawCommand::addFilledCircle(const vec2& pos, float radius, const Color& col, uint32_t seg_count)
{
	addFilledEllipse(pos, vec2(radius), col, seg_count);
}

void DrawCommand::addFilledCircleFan(const vec2& pos, float radius, float theta_min, float theta_max, const Color& col, uint32_t seg_count)
{
	auto idx  = reservePrims(seg_count + 1, 3 * seg_count);

	vertices.emplace_back(pos, col);
	for (uint32_t i = 0; i <= seg_count; ++i) {
		float theta = (theta_max - theta_min) * i / seg_count + theta_min;
		vertices.emplace_back(pos + radius * vec2(cosf(theta), sinf(theta)), col);
	}

	for (uint32_t i = 1; i <= seg_count; ++i) {
		indices.emplace_back(idx);
		indices.emplace_back(idx + i);
		indices.emplace_back(idx + i + 1);
	}
}

void DrawCommand::addEllipse(const vec2& pos, const vec2& size, float width, const Color& col, uint32_t seg_count)
{
	vec2 p0 = pos + vec2(size.x, 0);

	for (uint32_t i = 1; i <= seg_count; ++i) {
		float theta = 2.f * (float)M_PI * i / seg_count;
		vec2 p1 = pos + vec2(cosf(theta), sinf(theta));

		addLine(p0, p1, width, col);
		p0 = p1;
	}
}

void DrawCommand::addFilledEllipse(const vec2& pos, const vec2& size, const Color& col, uint32_t seg_count)
{
	uint32_t idx = reservePrims(seg_count + 1, 3 * seg_count);

	vertices.emplace_back(pos, col);
	for (uint32_t i = 0; i < seg_count; ++i) {
		float theta = 2.f * (float)M_PI * i / seg_count;
		vertices.emplace_back(pos + size * vec2(cosf(theta), sinf(theta)), col);
	}

	for (uint32_t i = 1; i < seg_count; ++i) {
		indices.emplace_back(idx);
		indices.emplace_back(idx + i);
		indices.emplace_back(idx + i + 1);
	}
	indices.emplace_back(idx);
	indices.emplace_back(idx + seg_count);
	indices.emplace_back(idx + 1);
}

void DrawCommand::addFilledCapsule(const vec2& p0, const vec2& p1, float width, const Color& col, uint32_t seg_count)
{
	uint32_t idx = reservePrims(seg_count + 4, 3 * seg_count + 6);

	vec2 normal = (width / 2.f) * glm::normalize(p1 - p0);
	normal = vec2(-normal.y, normal.x);

	vertices.emplace_back(p0, col);
	for (uint32_t i = 0; i <= seg_count / 2; ++i) {
		float theta = 2.f * (float)M_PI * i / seg_count;
		vertices.emplace_back(p0 + glm::rotate(normal, theta), col);
	}

	vertices.emplace_back(p1, col);
	for (uint32_t i = seg_count / 2; i <= seg_count; ++i) {
		float theta = 2.f * (float)M_PI * i / seg_count;
		vertices.emplace_back(p1 + glm::rotate(normal, theta), col);
	}

	for (uint32_t i = 1; i <= seg_count / 2; ++i) {
		indices.emplace_back(idx);
		indices.emplace_back(idx + i);
		indices.emplace_back(idx + i + 1);
	}

	uint32_t off = seg_count / 2 + 2;
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + seg_count + 3);
	indices.emplace_back(idx + off + 1);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + off - 1);
	indices.emplace_back(idx + off + 1);

	for (uint32_t i = 1; i <= seg_count / 2; ++i) {
		indices.emplace_back(idx + off);
		indices.emplace_back(idx + off + i);
		indices.emplace_back(idx + off + i + 1);
	}
}

void DrawCommand::addFilledHalfCapsule(const vec2& p, const vec2& mid, float width, const Color& col_p, Color& col_mid, uint32_t seg_count)
{
	uint32_t idx = reservePrims(seg_count / 2 + 4, 3 * seg_count / 2 + 6);

	vec2 normal = (width / 2.f) * glm::normalize(mid - p);
	normal = vec2(-normal.y, normal.x);

	vertices.emplace_back(p, col_p);
	for (uint32_t i = 0; i <= seg_count / 2; ++i) {
		float theta = 2.f * (float)M_PI * i / seg_count;
		vertices.emplace_back(p + glm::rotate(normal, theta), col_p);
	}
	vertices.emplace_back(mid + normal, col_mid);
	vertices.emplace_back(mid - normal, col_mid);

	for (uint32_t i = 1; i <= seg_count / 2; ++i) {
		indices.emplace_back(idx);
		indices.emplace_back(idx + i);
		indices.emplace_back(idx + i + 1);
	}

	uint32_t off = seg_count / 2 + 2;
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + off);
	indices.emplace_back(idx + off + 1);
	indices.emplace_back(idx + 1);
	indices.emplace_back(idx + off - 1);
	indices.emplace_back(idx + off + 1);
}

uint32_t DrawCommand::reservePrims(uint32_t vertex_count, uint32_t index_count)
{
	update_buffers = true;

	vertices.reserve(vertices.size() + vertex_count);
	indices.reserve(indices.size() + index_count);

	return (uint32_t)vertices.size();
}

void DrawCommand::clear(bool clear_options)
{
	vertices.clear();
	indices.clear();

	if (clear_options) 
		options.clear();
	
	update_buffers = true;
}


void DrawCommand::draw(RenderTarget& target, RenderStates& states) const
{
	if (vertices.empty()) return;

	auto& inst      = VK2DContext::get();
	auto cmd_buffer = target.getCommandBuffer();

	//prepare buffers
	if (update_buffers) {
		vertex_buffer.resize(vertices.size());
		vertex_buffer.update(vertices.data(), vertices.size());

		if (!indices.empty()) {
			index_buffer.resize(indices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer);
			index_buffer.update(indices.data(), indices.size() * sizeof(uint32_t));
			options.buffer = &index_buffer;
		}
		update_buffers = false;
	}

	states.reset(options);

	const Pipeline* pipeline = nullptr;

	if (options.pipeline.has_value())
		pipeline = options.pipeline.value();
	else if (options.texture.has_value())
		pipeline = &inst.basic_pipelines[0];
	else
		pipeline = &inst.basic_pipelines[1];

	cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);
	cmd_buffer.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);

	states.bindTVS(cmd_buffer, *pipeline->pipeline_layout);

	vk::DeviceSize offset = 0;
	cmd_buffer.bindVertexBuffers(0, 1, &vertex_buffer.getBuffer(), &offset);

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

	if (states.options.buffer.has_value()) { // has idx buffer
		auto& index_buffer = *states.options.buffer.value();
		cmd_buffer.bindIndexBuffer(index_buffer.getBuffer(), 0, vk::IndexType::eUint32);
		cmd_buffer.drawIndexed(index_buffer.size() / sizeof(uint32_t), 1, 0, 0, 0);
	} else {
		cmd_buffer.draw((uint32_t)vertex_buffer.size(), 1, 0, 0);
	}
}

VK2D_END