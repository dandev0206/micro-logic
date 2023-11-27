#pragma once

#include "../graphics/texture.h"
#include "glyph.h"

VK2D_BEGIN

class Font;

struct TextStyle {
	TextStyle() VK2D_NOTHROW;

	const Font* font;
	uint32_t    size;
	Color       color;
	Color       background_color;
	vec2        align;
	float       letter_spacing;
	float       line_spacing;
	float       outline_thickness;
	Color       outline_color;
	bool        bold;
	bool        under_line;
	bool        strike_through;
	bool        italic;
};

class Font {
public:
	Font();
	Font(Font&& rhs) VK2D_NOTHROW;
	~Font();

	Font& operator=(Font&& rhs) VK2D_NOTHROW;

	void loadFromFile(std::string path);

	const Glyph& getGlyph(uint32_t codePoint, uint32_t characterSize, bool bold, float outlineThickness = 0.f) const;
	
	bool hasGlyph(uint32_t codePoint) const;
	
	float getKerning(uint32_t first, uint32_t second, uint32_t characterSize, bool bold = false) const;
	float getLineSpacing(uint32_t characterSize) const;
	float getUnderlinePosition(uint32_t characterSize) const;
	float getUnderlineThickness(uint32_t characterSize) const;
	vec2 getTextSize(const char* text, uint32_t characterSize, const TextStyle& style);
	vec2 getTextSize(const std::string& text, uint32_t characterSize, const TextStyle& style);
	
	const Texture& getTexture(uint32_t characterSize) const;

private:
	class FontImpl;

	FontImpl* impl;
};

VK2D_END