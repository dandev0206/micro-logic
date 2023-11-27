#include "../include/vk2d/system/font.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/ftbitmap.h>
#include <freetype/ftstroke.h>
#include <map>

VK2D_BEGIN

using GlyphTable = std::map<uint64_t, Glyph>;

static uint64_t combine(float outlineThickness, bool bold, uint32_t index)
{
	return (static_cast<uint64_t>(*(uint32_t*)(&outlineThickness)) << 32) | ((uint64_t)bold << 31) | index;
}

struct Row {
	Row(uint32_t rowTop, uint32_t rowHeight)
		: width(0), top(rowTop), height(rowHeight)
	{}

	uint32_t width;
	uint32_t top;
	uint32_t height;
};

struct Page {
	Page() :
		nextRow(3)
	{
		Image image(128, 128, Colors::Black);

		image.setPixel(0, 0, Colors::White);
		image.setPixel(1, 0, Colors::White);
		image.setPixel(0, 1, Colors::White);
		image.setPixel(1, 1, Colors::White);

		texture.resize(image.size());
		texture.update(image);
	}

	GlyphTable       glyphs;
	Texture          texture;
	uint32_t         nextRow;
	std::vector<Row> rows;
};

using PageTable = std::map<uint32_t, Page>;

class Font::FontImpl {
public:
	FontImpl() :
		library(nullptr),
		face(nullptr),
		stroker(nullptr)
	{}

	~FontImpl() {
		destroy();
	}

	void destroy() {
		if (stroker) FT_Stroker_Done(stroker);
		if (face) FT_Done_Face(face);
		if (library) FT_Done_FreeType(library);

		library = nullptr;
		face    = nullptr;
		stroker = nullptr;
		pages.clear();
		pixel_buffer.clear();
	}

	Page& loadPage(uint32_t characterSize) const {
		auto item = pages.find(characterSize);
		if (item == pages.end())
			item = pages.insert(std::make_pair(characterSize, Page())).first;

		return item->second;
	}

	Glyph loadGlyph(uint32_t codePoint, uint32_t characterSize, bool bold, float outlineThickness) const {
		Glyph glyph;

		if (!face)
			return glyph;

		if (!setCurrentSize(characterSize))
			return glyph;

		FT_Int32 flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_FORCE_AUTOHINT;
		if (outlineThickness != 0)
			flags |= FT_LOAD_NO_BITMAP;
		if (FT_Load_Char(face, codePoint, flags) != 0)
			return glyph;

		FT_Glyph glyphDesc;
		if (FT_Get_Glyph(face->glyph, &glyphDesc) != 0)
			return glyph;

		FT_Pos weight = 1 << 6;
		bool outline = (glyphDesc->format == FT_GLYPH_FORMAT_OUTLINE);
		if (outline) {
			if (bold) {
				FT_OutlineGlyph outlineGlyph = reinterpret_cast<FT_OutlineGlyph>(glyphDesc);
				FT_Outline_Embolden(&outlineGlyph->outline, weight);
			}

			if (outlineThickness != 0) {
				FT_Stroker_Set(stroker, static_cast<FT_Fixed>(outlineThickness * static_cast<float>(1 << 6)), FT_STROKER_LINECAP_ROUND, FT_STROKER_LINEJOIN_ROUND, 0);
				FT_Glyph_Stroke(&glyphDesc, stroker, true);
			}
		}

		FT_Glyph_To_Bitmap(&glyphDesc, FT_RENDER_MODE_NORMAL, 0, 1);
		FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(glyphDesc);
		FT_Bitmap& bitmap = bitmapGlyph->bitmap;

		if (!outline) {
			if (bold)
				FT_Bitmap_Embolden(library, &bitmap, weight, weight);

			if (outlineThickness != 0)
				VK2D_ERROR("Failed to outline glyph (no fallback available)");
		}

		glyph.advance = static_cast<float>(bitmapGlyph->root.advance.x >> 16);
		if (bold)
			glyph.advance += static_cast<float>(weight) / static_cast<float>(1 << 6);

		glyph.lsbDelta = static_cast<int>(face->glyph->lsb_delta);
		glyph.rsbDelta = static_cast<int>(face->glyph->rsb_delta);

		uint32_t width  = bitmap.width;
		uint32_t height = bitmap.rows;

		if ((width > 0) && (height > 0)) {
			const uint32_t padding = 2;

			width  += 2 * padding;
			height += 2 * padding;

			Page& page = loadPage(characterSize);

			glyph.textureRect = findGlyphRect(page, width, height);

			glyph.textureRect.left   += static_cast<int>(padding);
			glyph.textureRect.top    += static_cast<int>(padding);
			glyph.textureRect.width  -= static_cast<int>(2 * padding);
			glyph.textureRect.height -= static_cast<int>(2 * padding);

			glyph.bounds.left   = static_cast<float>(bitmapGlyph->left);
			glyph.bounds.top    = static_cast<float>(-bitmapGlyph->top);
			glyph.bounds.width  = static_cast<float>(bitmap.width);
			glyph.bounds.height = static_cast<float>(bitmap.rows);

			pixel_buffer.resize(width * height);

			Color* current = pixel_buffer.data();
			Color* end     = current + width * height;

			for (; current != end; ++current) {
				current->r = 255;
				current->g = 255;
				current->b = 255;
				current->a = 0;
			}

			const uint8_t* pixels = bitmap.buffer;
			if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO) {
				for (uint32_t y = padding; y < height - padding; ++y) {
					for (uint32_t x = padding; x < width - padding; ++x) {
						size_t index = x + y * width;
						pixel_buffer[index].a = ((pixels[(x - padding) / 8]) & (1 << (7 - ((x - padding) % 8)))) ? 255 : 0;
					}
					pixels += bitmap.pitch;
				}
			} else {
				for (uint32_t y = padding; y < height - padding; ++y) {
					for (uint32_t x = padding; x < width - padding; ++x) {
						size_t index = x + y * width;
						pixel_buffer[index].a = pixels[x - padding];
					}
					pixels += bitmap.pitch;
				}
			}

			uint32_t x = static_cast<uint32_t>(glyph.textureRect.left) - padding;
			uint32_t y = static_cast<uint32_t>(glyph.textureRect.top) - padding;
			uint32_t w = static_cast<uint32_t>(glyph.textureRect.width) + 2 * padding;
			uint32_t h = static_cast<uint32_t>(glyph.textureRect.height) + 2 * padding;
			page.texture.update(pixel_buffer.data(), uvec2(w, h), uvec2(x, y));
		}

		FT_Done_Glyph(glyphDesc);

		return glyph;
	}

	uRect findGlyphRect(Page& page, uint32_t width, uint32_t height) const
	{
		Row* row = nullptr;

		float bestRatio = 0;
		for (auto& item : page.rows) {
			float ratio = (float)height / (float)item.height;

			if ((ratio < 0.7f) || (ratio > 1.f))
				continue;

			if (width > page.texture.size().x - item.width)
				continue;

			if (ratio < bestRatio)
				continue;

			row = &item;
			bestRatio = ratio;
			if (!row) break;
		}

		if (!row) {
			uint32_t max_size  = Texture::getMaximumSize().x;
			uint32_t rowHeight = height + height / 10;

			while ((page.nextRow + rowHeight >= page.texture.size().y) || (width >= page.texture.size().x)) {
				uint32_t textureWidth  = page.texture.size().x;
				uint32_t textureHeight = page.texture.size().y;
				if ((textureWidth * 2 <= max_size) && (textureHeight * 2 <= max_size)) {
					Texture newTexture(textureWidth * 2, textureHeight * 2);
					newTexture.update(page.texture);
					page.texture = std::move(newTexture);
				} else {
					VK2D_ERROR("Failed to add a new character to the font: the maximum texture size has been reached");
				}
			}

			page.rows.push_back(Row(page.nextRow, rowHeight));
			page.nextRow += rowHeight;
			row = &page.rows.back();
		}

		iRect rect(row->width, row->top, width, height);
		row->width += width;

		return rect;
	}

	bool setCurrentSize(uint32_t characterSize) const 
	{
		FT_UShort currentSize = face->size->metrics.x_ppem;

		if (currentSize != characterSize) {
			FT_Error result = FT_Set_Pixel_Sizes(face, 0, characterSize);

			if (result == FT_Err_Invalid_Pixel_Size) {
				if (!FT_IS_SCALABLE(face))
				{
					std::stringstream ss;
					ss << "Failed to set bitmap font size to " << characterSize << "\n";
					ss << "Available sizes are: ";
					for (int i = 0; i < face->num_fixed_sizes; ++i) {
						const long size = (face->available_sizes[i].y_ppem + 32) >> 6;
						ss << size << " ";
					}
					ss << "\n";

					VK2D_ERROR(ss.str());
				} else {
					VK2D_ERROR("Failed to set font size to " + std::to_string(characterSize));
				}
			}

			return result == FT_Err_Ok;
		}

		return true;
	}

public:
	FT_Library                 library;
	FT_Face                    face;
	FT_Stroker                 stroker;
	GlyphTable                 glyphs;
	mutable PageTable          pages;
	std::string                family;
	mutable std::vector<Color> pixel_buffer;
};

TextStyle::TextStyle() VK2D_NOTHROW :
	font(nullptr),
	size(0),
	color(Colors::White),
	background_color(Colors::Transparent),
	letter_spacing(1.f),
	line_spacing(1.f),
	align(0.f),
	outline_thickness(0.f),
	outline_color(Colors::Black),
	bold(false),
	under_line(false),
	strike_through(false),
	italic(false)
{}

Font::Font() :
	impl(nullptr)
{}

Font::~Font()
{
	if (impl) 
		impl->destroy();

	delete impl;
}

Font::Font(Font&& rhs) VK2D_NOTHROW :
	impl(std::move(rhs.impl))
{}

Font& Font::operator=(Font&& rhs) VK2D_NOTHROW
{
	impl = std::move(rhs.impl);
	
	return *this;
}

void Font::loadFromFile(std::string path)
{
	if (impl)
		impl->destroy();
	else
		impl = new FontImpl();

	if (FT_Init_FreeType(&impl->library) != 0) {
		VK2D_ERROR("Failed to load font \"" + path + "\" (failed to initialize FreeType)");
	}

	if (FT_New_Face(static_cast<FT_Library>(impl->library), path.c_str(), 0, &impl->face) != 0)
	{
		VK2D_ERROR("Failed to load font \"" + path + "\" (failed to create the font face)");
	}

	// Load the stroker that will be used to outline the font
	FT_Stroker stroker;
	if (FT_Stroker_New(static_cast<FT_Library>(impl->library), &stroker) != 0) {
		FT_Done_Face(impl->face);
		VK2D_ERROR("Failed to load font \"" + path + "\" (failed to create the stroker)");
	}

	// Select the unicode character map
	if (FT_Select_Charmap(impl->face, FT_ENCODING_UNICODE) != 0) {
		FT_Stroker_Done(stroker);
		FT_Done_Face(impl->face);
		VK2D_ERROR("Failed to load font \"" + path + "\" (failed to set the Unicode character set)");
	}

	impl->family = std::string(impl->face->family_name);
}

const Glyph& Font::getGlyph(uint32_t codePoint, uint32_t characterSize, bool bold, float outlineThickness) const
{
	GlyphTable& glyphs = impl->loadPage(characterSize).glyphs;
	
	uint64_t key = combine(outlineThickness, bold, FT_Get_Char_Index(impl->face, codePoint));

	auto item = glyphs.find(key);
	if (item != glyphs.end()) {
		return item->second;
	} else {
		Glyph glyph = impl->loadGlyph(codePoint, characterSize, bold, outlineThickness);
		return glyphs.insert(std::make_pair(key, glyph)).first->second;
	}
}

bool Font::hasGlyph(uint32_t codePoint) const
{
	return FT_Get_Char_Index(impl->face, codePoint) != 0;
}

float Font::getKerning(uint32_t first, uint32_t second, uint32_t characterSize, bool bold) const
{
	if (first == 0 || second == 0)
		return 0.f;

	if (impl->face && impl->setCurrentSize(characterSize)) {
		FT_UInt index1 = FT_Get_Char_Index(impl->face, first);
		FT_UInt index2 = FT_Get_Char_Index(impl->face, second);

		float firstRsbDelta  = (float)getGlyph(first, characterSize, bold).rsbDelta;
		float secondLsbDelta = (float)getGlyph(second, characterSize, bold).lsbDelta;

		FT_Vector kerning;
		kerning.x = kerning.y = 0;
		if (FT_HAS_KERNING(impl->face))
			FT_Get_Kerning(impl->face, index1, index2, FT_KERNING_UNFITTED, &kerning);

		if (!FT_IS_SCALABLE(impl->face))
			return (float)kerning.x;

		return std::floor((secondLsbDelta - firstRsbDelta + static_cast<float>(kerning.x) + 32) / static_cast<float>(1 << 6));
	} else {
		return 0.f;
	}
}

float Font::getLineSpacing(uint32_t characterSize) const
{
	if (impl->face && impl->setCurrentSize(characterSize)) {
		return static_cast<float>(impl->face->size->metrics.height) / static_cast<float>(1 << 6);
	} else {
		return 0.f;
	}
}

float Font::getUnderlinePosition(uint32_t characterSize) const
{
	if (impl->face && impl->setCurrentSize(characterSize)) {
		if (!FT_IS_SCALABLE(impl->face))
			return static_cast<float>(characterSize) / 10.f;

		return -static_cast<float>(FT_MulFix(impl->face->underline_position, impl->face->size->metrics.y_scale)) / static_cast<float>(1 << 6);
	} else {
		return 0.f;
	}
}

float Font::getUnderlineThickness(uint32_t characterSize) const
{
	if (impl->face && impl->setCurrentSize(characterSize)) {
		if (!FT_IS_SCALABLE(impl->face))
			return static_cast<float>(characterSize) / 14.f;

		return static_cast<float>(FT_MulFix(impl->face->underline_thickness, impl->face->size->metrics.y_scale)) / static_cast<float>(1 << 6);
	} else {
		return 0.f;
	}
}

vec2 Font::getTextSize(const char* text, uint32_t characterSize, const TextStyle& style)
{
	return vec2();
}

vec2 Font::getTextSize(const std::string& text, uint32_t characterSize, const TextStyle& style)
{
	return getTextSize(text.c_str(), characterSize, style);
}

const Texture& Font::getTexture(uint32_t characterSize) const
{
	return impl->loadPage(characterSize).texture;
}

VK2D_END
