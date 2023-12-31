#pragma once

#include "draw_command.h"
#include "text_style.h"

VK2D_BEGIN

class DrawList : public Drawable {
public:
	DrawList();

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
	void addText(const std::string& text, const vec2& pos, const TextStyle& style);

	void setNextCommand(size_t idx);
	DrawCommand& getCommand(const Texture* texture = nullptr);

	void resize(size_t size);
	void clear();

	const DrawCommand& operator[](size_t idx) const;
	DrawCommand& operator[](size_t idx);

private:
	void draw(RenderTarget& target, RenderStates& states) const;

public:
	mutable std::vector<DrawCommand> commands;

private:
	int64_t curr_cmd;
};

VK2D_END