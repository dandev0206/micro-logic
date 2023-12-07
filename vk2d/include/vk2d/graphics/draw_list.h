#pragma once

#include "../core/vertex.h"
#include "../core/rect.h"
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
	void addRect(const vec2& pos, const vec2& size, float width, const Color& col);
	void addFilledRect(const vec2& pos, const vec2& size, const Color& col);
	void addFilledRect(const vec2& pos, const vec2& size, const Color& col, const Rect& uv_rect);
	void addCircle(const vec2& pos, float radius, float width, const Color& col, uint32_t seg_count = 30);
	void addFilledCircle(const vec2& pos, float radius, const Color& col, uint32_t seg_count = 30);
	void addEllipse(const vec2& pos, const vec2& size, float width, const Color& col, uint32_t seg_count = 30);
	void addFilledEllipse(const vec2& pos, const vec2& size, const Color& col, uint32_t seg_count = 30);
	void addFilledCapsule(const vec2& p0, const vec2& p1, float width, const Color& col, uint32_t seg_count = 30);
	void addFilledHalfCapsule(const vec2& p, const vec2& mid, float width, const Color& col_p, Color& col_mid, uint32_t seg_count = 30);

	void clear(bool clear_options = true);

	uint32_t reservePrims(uint32_t vertex_count, uint32_t index_count);

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

class DrawList : public Drawable {
public:
	DrawList();

	void resize(size_t size);
	void clear();

	const DrawCommand& operator[](size_t idx) const;
	DrawCommand& operator[](size_t idx);

private:
	void draw(RenderTarget& target, RenderStates& states) const;

public:
	mutable std::vector<DrawCommand> commands;
};

VK2D_END