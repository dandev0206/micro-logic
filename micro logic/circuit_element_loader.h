#pragma once

#include <vk2d/graphics/render_texture.h>
#include <vk2d/graphics/draw_list.h>
#include <vk2d/system/font.h>

#include "circuit_element.h"

namespace tinyxml2 {
class XMLElement;
}

class CircuitElementLoader {
public:
	void load(const char* dir, const vk2d::Font& font);

private:
	void calculatePacking();
	void renderTexture(tinyxml2::XMLElement* drawings, uint64_t id);
	void getPinLayouts(tinyxml2::XMLElement* elem, std::vector<PinLayout>& pin_layouts);

	Rect getExtent(tinyxml2::XMLElement* elem);

public:
	std::vector<LogicGate>     logic_gates;
	std::vector<vk2d::Texture> textures;
	
private:
	const vk2d::Font* font;
	float             grid_pixel_size;
	float             grid_scale;
	float             scailing;

	struct TextureData {
		vk2d::DrawList      drawlist;
		vk2d::RenderTexture texture;
		vk2d::Image         image_mask;
	};

	std::vector<TextureData> texture_datas;
};