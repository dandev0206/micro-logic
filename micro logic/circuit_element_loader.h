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
	void loadExtents(tinyxml2::XMLElement* root);
	void calculatePacking();
	void loadElements(tinyxml2::XMLElement* root);
	void renderTexture(tinyxml2::XMLElement* drawings, uint64_t id);
	void getPinLayouts(tinyxml2::XMLElement* elem, std::vector<PinLayout>& pin_layouts);
	LogicGate makeLogicGate(tinyxml2::XMLElement* elem, uint64_t id);
	LogicUnit makeLogicUnit(tinyxml2::XMLElement* elem, uint64_t id);

	Rect getExtent(tinyxml2::XMLElement* elem);

public:
	struct TextureData {
		vk2d::RenderTexture texture;
		vk2d::Texture       texture_mask;
		vk2d::Image         image_mask;
		vk2d::DrawList      drawlist;
	};

	std::vector<TextureData>                texture_datas;
	std::vector<std::pair<uint32_t, iRect>> texture_coords;
	std::vector<Rect>                       extents;
	std::vector<LogicGate>                  logic_gates;
	std::vector<LogicUnit>                  logic_units;
	const vk2d::Font*                       font;
};