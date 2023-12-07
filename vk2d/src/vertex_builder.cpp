#include "../include/vk2d/graphics/vertex_builder.h"

VK2D_BEGIN

static void add_hline(DrawCommand& cmd, float lineLength, float lineTop, const Color& color, float offset, float thickness, float outlineThickness = 0)
{
	float top    = std::floor(lineTop + offset - (thickness / 2) + 0.5f);
	float bottom = top + std::floor(thickness + 0.5f);

	cmd.vertices.emplace_back(Vertex(vec2(-outlineThickness, top - outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(lineLength + outlineThickness, top - outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(-outlineThickness, bottom + outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(-outlineThickness, bottom + outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(lineLength + outlineThickness, top - outlineThickness), color, vec2(1, 1)));
	cmd.vertices.emplace_back(Vertex(vec2(lineLength + outlineThickness, bottom + outlineThickness), color, vec2(1, 1)));
}

static Rect add_glyph_quad(DrawCommand& cmd, const vec2& pos, const Color& color, const Glyph& glyph, float italicShear)
{
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
	cmd.vertices.emplace_back(Vertex(vec2(p0.x - italicShear * p1.y, p1.y), color, vec2(u0, v1)));
	cmd.vertices.emplace_back(Vertex(vec2(p1.x - italicShear * p0.y, p0.y), color, vec2(u1, v0)));
	cmd.vertices.emplace_back(Vertex(vec2(p1.x - italicShear * p1.y, p1.y), color, vec2(u1, v1)));

	return { p0, p1 - p0 };
}

static float lerp(float a, float b, float t) {
	return a + (b - a) * t;
}

VertexBuilder::VertexBuilder() :
	last_font(nullptr),
	last_font_size(0)
{}

void VertexBuilder::init()
{
	draw_list.clear();

	last_font      = nullptr;
	last_font_size = 0;
}

void VertexBuilder::addRect(const vec2& pos, const vec2& size, const Color& color)
{
	auto& cmd = prepareCommand();

	Vertex v0(pos, color);
	Vertex v1(pos + vec2(size.x, 0), color);
	Vertex v2(pos + size, color);
	Vertex v3(pos + vec2(0, size.y), color);

	cmd.vertices.emplace_back(v0);
	cmd.vertices.emplace_back(v1);
	cmd.vertices.emplace_back(v2);
	cmd.vertices.emplace_back(v0);
	cmd.vertices.emplace_back(v2);
	cmd.vertices.emplace_back(v3);
}

void VertexBuilder::addText(const std::string& text, const vec2& pos, const TextStyle& style)
{

}

const DrawList& VertexBuilder::getDrawList() const
{
	return draw_list;
}

DrawCommand& VertexBuilder::prepareCommand(const Font* font, uint32_t font_size)
{
	if (font == nullptr) {
		if (last_font == nullptr && draw_list.commands.empty()) {
			return draw_list.commands.emplace_back();
		} else {
			return draw_list.commands.back();
		}
	}

	if (font != last_font || font_size != last_font_size) {
		auto& cmd = draw_list.commands.emplace_back();

		cmd.options.texture = &font->getTexture(font_size);
		
		last_font      = font;
		last_font_size = font_size;

		return cmd;
	} else {
		return draw_list.commands.back();
	}
}

VK2D_END
