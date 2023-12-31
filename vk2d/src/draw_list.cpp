#include "../include/vk2d/graphics/draw_list.h"

#include "../include/vk2d/system/font.h"

VK2D_BEGIN

static void add_hline(DrawCommand& cmd, float lineLength, float lineTop, const Color& color, float offset, float thickness, float outlineThickness = 0)
{
	auto idx = cmd.reservePrims(4, 6);

	float top    = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
	float bottom = top + std::floor(thickness + 0.5f);

	cmd.vertices.emplace_back(Vertex(vec2(-outlineThickness, top - outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(lineLength + outlineThickness, top - outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(-outlineThickness, bottom + outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(lineLength + outlineThickness, bottom + outlineThickness), color, vec2(1, 1)));

	cmd.indices.emplace_back(idx + 0);
	cmd.indices.emplace_back(idx + 1);
	cmd.indices.emplace_back(idx + 2);
	cmd.indices.emplace_back(idx + 1);
	cmd.indices.emplace_back(idx + 2);
	cmd.indices.emplace_back(idx + 3);
}

static Rect add_glyph_quad(DrawCommand& cmd, const vec2& pos, const Color& color, const Glyph& glyph, float italicShear)
{
	auto idx = cmd.reservePrims(4, 6);

	float padding = 1.0;

	vec2 p0 = pos + glyph.bounds.getPosition() - vec2(padding);
	vec2 p1 = p0 + glyph.bounds.getSize() + 2.f * vec2(padding);

	float u0 = static_cast<float>(glyph.textureRect.left) - padding;
	float v0 = static_cast<float>(glyph.textureRect.top) - padding;
	float u1 = static_cast<float>(glyph.textureRect.left + glyph.textureRect.width) + padding;
	float v1 = static_cast<float>(glyph.textureRect.top + glyph.textureRect.height) + padding; 

	cmd.vertices.emplace_back(Vertex(vec2(p0.x - italicShear * p0.y, p0.y), color, vec2(u0, v0)));
	cmd.vertices.emplace_back(Vertex(vec2(p1.x - italicShear * p0.y, p0.y), color, vec2(u1, v0)));
	cmd.vertices.emplace_back(Vertex(vec2(p0.x - italicShear * p1.y, p1.y), color, vec2(u0, v1)));
	cmd.vertices.emplace_back(Vertex(vec2(p1.x - italicShear * p1.y, p1.y), color, vec2(u1, v1)));

	cmd.indices.emplace_back(idx + 0);
	cmd.indices.emplace_back(idx + 1);
	cmd.indices.emplace_back(idx + 2);
	cmd.indices.emplace_back(idx + 1);
	cmd.indices.emplace_back(idx + 2);
	cmd.indices.emplace_back(idx + 3);

	return { p0, p1 - p0 };
}

DrawList::DrawList() :
	curr_cmd(-1)
{}

void DrawList::addLine(const vec2& p0, const vec2& p1, float width, const Color& col)
{
	getCommand().addLine(p0, p1, width, col);
}

void DrawList::addTriangle(const vec2& p0, const vec2& p1, const vec2& p2, float width, const Color& col)
{
	getCommand().addTriangle(p0, p1, p2, width, col);
}

void DrawList::addFilledTriangle(const vec2& p0, const vec2& p1, const vec2& p2, const Color& col)
{
	getCommand().addFilledTriangle(p0, p1, p2, col);
}

void DrawList::addRect(const vec2& pos, const vec2& size, float width, const Color& col)
{
	getCommand().addRect(pos, size, width, col);
}

void DrawList::addFilledRect(const vec2& pos, const vec2& size, const Color& col)
{
	getCommand().addFilledRect(pos, size, col);
}

void DrawList::addFilledRect(const vec2& pos, const vec2& size, const Color& col, const Rect& uv_rect)
{
	getCommand().addFilledRect(pos, size, col, uv_rect);
}

void DrawList::addCircle(const vec2& pos, float radius, float width, const Color& col, uint32_t seg_count)
{
	getCommand().addEllipse(pos, vec2(radius), width, col, seg_count);
}

void DrawList::addFilledCircle(const vec2& pos, float radius, const Color& col, uint32_t seg_count)
{
	getCommand().addFilledEllipse(pos, vec2(radius), col, seg_count);
}

void DrawList::addFilledCircleFan(const vec2& pos, float radius, float theta_min, float theta_max, const Color& col, uint32_t seg_count)
{
	getCommand().addFilledCircleFan(pos, radius, theta_min, theta_max, col, seg_count);
}

void DrawList::addEllipse(const vec2& pos, const vec2& size, float width, const Color& col, uint32_t seg_count)
{
	getCommand().addEllipse(pos, size, width, col, seg_count);
}

void DrawList::addFilledEllipse(const vec2& pos, const vec2& size, const Color& col, uint32_t seg_count)
{
	getCommand().addFilledEllipse(pos, size, col, seg_count);
}

void DrawList::addFilledCapsule(const vec2& p0, const vec2& p1, float width, const Color& col, uint32_t seg_count)
{
	getCommand().addFilledCapsule(p0, p1, width, col, seg_count);
}

void DrawList::addFilledHalfCapsule(const vec2& p, const vec2& mid, float width, const Color& col_p, Color& col_mid, uint32_t seg_count)
{
	getCommand().addFilledHalfCapsule(p, mid, width, col_p, col_mid, seg_count);
}

void DrawList::addText(const std::string& text, const vec2& pos, const TextStyle& style)
{
	VK2D_ASSERT(style.font && "null font");

	if (text.empty()) return;

	auto& cmd = getCommand(&style.font->getTexture(style.size));
	cmd.update_buffers = true; // TODO: use reservePrims()

	const auto& font   = *style.font;
	uint32_t font_size = style.size;

	bool  is_bold           = style.bold;
	bool  is_underlined     = style.under_line;
	bool  is_strike_through = style.strike_through;
	float italic_shear      = style.italic ? 0.209f : 0.f; // 12 degrees in radians

	Rect xBounds = font.getGlyph(L'x', font_size, is_bold).bounds;

	float strike_through_offset = (xBounds.top + xBounds.height / 2.f);
	float whitespace_width      = font.getGlyph(L' ', font_size, is_bold).advance;
	float letter_spacing        = (whitespace_width / 3.f) * (style.letter_spacing - 1.f);
	whitespace_width           += letter_spacing;
	float line_spacing          = font.getLineSpacing(font_size) * style.line_spacing;
	
	float underline_offset    = font.getUnderlinePosition(font_size);
	float underline_thickness = font.getUnderlineThickness(font_size);

	float x     = pos.x;
	float y     = pos.y;
	float min_x = pos.x + (float)style.size;
	float min_y = pos.y + (float)style.size;
	float max_x = pos.x;
	float max_y = pos.y;

	size_t prev_vert_size = cmd.vertices.size();
	uint32_t prev_char    = 0;

	for (std::size_t i = 0; i < text.size(); ++i) {
		int32_t curChar = text[i];

		if (curChar == L'\r') continue;

		x += font.getKerning(prev_char, curChar, font_size, is_bold);

		if (is_underlined && (curChar == L'\n' && prev_char != L'\n')) {
			add_hline(cmd, x, y, style.color, underline_offset, underline_thickness);

			if (style.outline_thickness != 0)
				add_hline(cmd, x, y, style.outline_color, underline_offset, underline_thickness, style.outline_thickness);
		}

		if (is_strike_through && (curChar == L'\n' && prev_char != L'\n')) {
			add_hline(cmd, x, y, style.color, strike_through_offset, underline_thickness);

			if (style.outline_thickness != 0)
				add_hline(cmd, x, y, style.outline_color, strike_through_offset, underline_thickness, style.outline_thickness);
		}

		prev_char = curChar;

		if ((curChar == L' ') || (curChar == L'\n') || (curChar == L'\t')) {
			min_x = std::min(min_x, x);
			min_y = std::min(min_y, y);

			switch (curChar) {
			case L' ':  x += whitespace_width;     break;
			case L'\t': x += whitespace_width * 4; break;
			case L'\n': y += line_spacing; x = 0;  break;
			}

			max_x = std::max(max_x, x);
			max_y = std::max(max_y, y);

			continue;
		}

		if (style.outline_thickness != 0) {
			const Glyph& glyph = font.getGlyph(curChar, font_size, is_bold, style.outline_thickness);
			add_glyph_quad(cmd, vec2(x, y), style.outline_color, glyph, italic_shear);
		}

		const Glyph& glyph = font.getGlyph(curChar, font_size, is_bold);

		auto rect = add_glyph_quad(cmd, vec2(x, y), style.color, glyph, italic_shear);

		min_x = std::min(min_x, rect.left);
		max_x = std::max(max_x, rect.left + rect.width);
		min_y = std::min(min_y, rect.top);
		max_y = std::max(max_y, rect.top + rect.height);

		x += glyph.advance + letter_spacing;
	}

	vec2 align((min_x - max_x) * style.align.x, (max_y - min_y) * style.align.y);

	if (align != vec2(0.f)) {
		for (size_t i = prev_vert_size; i < cmd.vertices.size(); ++i)
			cmd.vertices[i].pos += align;
	}
}

void DrawList::setNextCommand(size_t idx)
{
	VK2D_ASSERT(idx < commands.size());

	curr_cmd = idx;
}

DrawCommand& DrawList::getCommand(const Texture* texture)
{
	if (commands.empty()) {
		auto& cmd = commands.emplace_back();
		if (texture)
			cmd.options.texture = texture;
		
		curr_cmd = 0;
	} else if (texture && commands[curr_cmd].options.texture != texture) {
		auto& cmd = commands.emplace_back();
		if (texture)
			cmd.options.texture = texture;

		curr_cmd = commands.size() - 1;
	}

	return commands[curr_cmd];
}

void DrawList::resize(size_t size)
{
	commands.resize(size);
	curr_cmd = size - 1;
}

void DrawList::clear()
{
	commands.clear();
	curr_cmd = -1;
}

const DrawCommand& DrawList::operator[](size_t idx) const 
{
	return commands[idx];
}

DrawCommand& DrawList::operator[](size_t idx)
{
	return commands[idx];
}

void DrawList::draw(RenderTarget& target, RenderStates& states) const
{
	for (auto& cmd : commands)
		cmd.draw(target, states);
}

VK2D_END