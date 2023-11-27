#pragma once

#include "../system/font.h"
#include "draw_list.h"

VK2D_BEGIN

class VertexBuilder {
public:
	VertexBuilder();

	void init();

	void addRect(const vec2& pos, const vec2& size, const Color& color);
	void addText(const std::string& text, const vec2& pos, const TextStyle& style);

	const DrawList& getDrawList() const;

private:
	DrawCommand& prepareCommand(const Font* font = nullptr, uint32_t font_size = 0);

private:
	DrawList draw_list;

private:
	const Font* last_font;
	uint32_t    last_font_size;
};

VK2D_END