#pragma once

#include "../core/vertex.h"
#include "render_options.h"
#include "vertex_buffer.h"
#include "buffer.h"
#include "drawable.h"

VK2D_BEGIN

class DrawCommand : public Drawable {
	friend class DrawList;
public:
	DrawCommand();

	void addLine(const vec2& p0, const vec2& p1, float width, const Color& col);
	void addTriangle(const vec2& p0, const vec2& p1, const vec2& p2, float width, const Color& col);
	void addFilledTriangle(const vec2& p0, const vec2& p1, const vec2& p2, const Color& col);
	void addRect(const vec2& pos, const vec2& size, float width, const Color& col);
	void addFilledRect(const vec2& pos, const vec2& size, const Color& col);
	void addFilledRect(const vec2& pos, const vec2& size, const Color& col, const Rect& uv_rect);
	void addCircle(const vec2& pos, float radius, float width, const Color& col, uint32_t seg_count = 36);
	void addFilledCircle(const vec2& pos, float radius, const Color& col, uint32_t seg_count = 36);
	void addFilledCircleFan(const vec2& pos, float radius, float theta_min, float theta_max, const Color& col, uint32_t seg_count = 36);
	void addEllipse(const vec2& pos, const vec2& size, float width, const Color& col, uint32_t seg_count = 36);
	void addFilledEllipse(const vec2& pos, const vec2& size, const Color& col, uint32_t seg_count = 36);
	void addFilledCapsule(const vec2& p0, const vec2& p1, float width, const Color& col, uint32_t seg_count = 36);
	void addFilledHalfCapsule(const vec2& p, const vec2& mid, float width, const Color& col_p, Color& col_mid, uint32_t seg_count = 36);

	uint32_t reservePrims(uint32_t vertex_count, uint32_t index_count);

	void clear(bool clear_options = true);

	std::vector<Vertex>   vertices;
	std::vector<uint32_t> indices;
	mutable RenderOptions options;

private:
	void draw(RenderTarget& target, RenderStates& states) const;

private:
	mutable VertexBuffer vertex_buffer;
	mutable Buffer       index_buffer;
	mutable bool         update_buffers;
};

VK2D_END
