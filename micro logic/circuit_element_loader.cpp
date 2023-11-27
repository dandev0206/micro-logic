#include "circuit_element_loader.h"

#include "vk2d/graphics/vertex_buffer.h"
#include <tinyxml2.h>

static vec2 parse_vec2(const tinyxml2::XMLElement* element, const char* name)
{
	auto* str = element->Attribute(name);
	size_t size;

	return { std::stof(str, &size), std::stof(str + size + 1) };
}

static vk2d::Color parse_color(const tinyxml2::XMLElement* element, const char* name)
{
	auto* str = element->Attribute(name);
	size_t size;

	vk2d::Color color;
	color.r = (uint8_t)std::stof(str, &size);
	color.g = (uint8_t)std::stof(str += size + 1, &size);
	color.b = (uint8_t)std::stof(str += size + 1, &size);
	
	if (*(str + size) == '\0') color.a = 255;
	else color.a = (uint8_t)std::stof(str += size + 1);

	return color;
}

void CircuitElementLoader::load(const char* path, const vk2d::Font& font)
{
	this->font = &font;

	tinyxml2::XMLDocument doc;
	doc.LoadFile(path);

	loadExtents(doc.RootElement());
	calculatePacking();
	loadElements(doc.RootElement());
}

void CircuitElementLoader::loadExtents(tinyxml2::XMLElement* root)
{
	auto* elem = root->FirstChildElement("LogicGate");
	for (; elem; elem = elem->NextSiblingElement("LogicGate")) {
		auto extent = getExtent(elem);
		extents.emplace_back(scale_rect(extent, 1.f / DEFAULT_GRID_SIZE));
		texture_coords.emplace_back(0, (uRect)extent);
	}

	elem = root->FirstChildElement("LogicUnit");
	for (; elem; elem = elem->NextSiblingElement("LogicUnit")) {
		auto extent = getExtent(elem);
		extents.emplace_back(scale_rect(extent, 1.f / DEFAULT_GRID_SIZE));
		texture_coords.emplace_back(0, (uRect)extent);
	}
}

void CircuitElementLoader::calculatePacking()
{
	const uvec2 max_texture_extent(2000, 2000);

	uint32_t max_height = 0;
	uint32_t extent_idx = 0;
	uint32_t padding    = 5;

	while (extent_idx < texture_coords.size()) {
		auto  texture_id   = (uint32_t)texture_datas.size();
		auto& texture_data = texture_datas.emplace_back();
		auto& texture      = texture_data.texture;
		
		uvec2 cursor(0, 0);

		for (; extent_idx < texture_coords.size(); ++extent_idx) {
			auto& extent = texture_coords[extent_idx].second;

			auto rb = extent.getSize();
			
			if (max_texture_extent.x < cursor.x + rb.x + padding) {
				cursor.x  = 0;
				cursor.y += max_height;
			}

			if (max_texture_extent.y < cursor.y + rb.y + padding) {
				break;
			}

			texture_coords[extent_idx].first  = texture_id;
			texture_coords[extent_idx].second.setPosition(cursor);

			cursor.x  += extent.width + padding;
			max_height = std::max(max_height, extent.height + padding);
		}

		uvec2 texture_size;
		texture_size.x = cursor.y == 0 ? cursor.x : max_texture_extent.x;
		texture_size.y = cursor.y + max_height;

		texture.resize(texture_size);
	}
}

void CircuitElementLoader::loadElements(tinyxml2::XMLElement* root)
{
	uint64_t id = 0;

	auto* elem = root->FirstChildElement("LogicGate");
	for (; elem; elem = elem->NextSiblingElement("LogicGate"))
		renderTexture(elem->FirstChildElement("Appearance")->FirstChildElement(), id++);

	elem = root->FirstChildElement("LogicUnit");
	for (; elem; elem = elem->NextSiblingElement("LogicUnit"))
		renderTexture(elem->FirstChildElement("Appearance")->FirstChildElement(), id++);

	for (auto& data : texture_datas) {
		auto& builder = data.builder;

		data.texture.draw(builder.getDrawList());
		data.texture.display();

		data.image_mask = data.texture.getTexture().getImage();
		data.image_mask = data.image_mask.createMask(vk2d::Colors::Transparent, vk2d::Colors::White, false);
		
		data.texture_mask.resize(data.image_mask.size());
		data.texture_mask.update(data.image_mask);
	}

	id = 0;

	elem = root->FirstChildElement("LogicGate");
	for (; elem; elem = elem->NextSiblingElement("LogicGate"))
		logic_gates.emplace_back(makeLogicGate(elem, id++));

	elem = root->FirstChildElement("LogicUnit");
	for (; elem; elem = elem->NextSiblingElement("LogicUnit"))
		logic_units.emplace_back(makeLogicUnit(elem, id++));
}

void CircuitElementLoader::renderTexture(tinyxml2::XMLElement* drawings, uint64_t id)
{
	auto& builder     = texture_datas[texture_coords[id].first].builder;
	auto texture_pos  = (vec2)texture_coords[id].second.getPosition();
	auto texture_size = (vec2)texture_coords[id].second.getSize();
	auto translate    = texture_pos - DEFAULT_GRID_SIZE * extents[id].getPosition();

	vk2d::TextStyle style;
	style.font = font;

	for (; drawings; drawings = drawings->NextSiblingElement()) {
		if (!std::strcmp(drawings->Name(), "Rect")) {
			auto size   = parse_vec2(drawings, "size");
			auto pos    = parse_vec2(drawings, "pos");
			auto origin = parse_vec2(drawings, "origin");
			auto color  = parse_color(drawings, "col");

			builder.addRect(pos + translate - origin, size, color);
		} else if (!std::strcmp(drawings->Name(), "Text")) {
			std::string text = drawings->GetText();
			vec2 pos         = parse_vec2(drawings, "pos");
			style.size       = drawings->IntAttribute("size");
			style.align      = parse_vec2(drawings, "align");
			builder.addText(text, pos + translate, style);
		}
	}
}

LogicGate CircuitElementLoader::makeLogicGate(tinyxml2::XMLElement* elem, uint64_t id)
{
	LogicGate logic_gate;

	auto& draw_data = texture_datas[texture_coords[id].first];

	auto shared            = std::make_shared<LogicGate::Shared>();
	shared->name           = elem->Attribute("name");
	// shared->description    = elem->Attribute("description");
	shared->shared_id      = id;
	shared->element_id     = id;
	shared->texture_id     = 2 * texture_coords[id].first + 2;
	shared->mask_id        = shared->texture_id + 1;
	shared->texture_coord  = texture_coords[id].second;
	shared->texture_extent = texture_coords[id].second.getSize();
	shared->extent         = extents[id];
	shared->image_mask     = draw_data.image_mask.blit(shared->texture_coord);

	logic_gate.shared = shared;
	logic_gate.pos    = {};
	logic_gate.dir    = Direction::Up;

	return logic_gate;
}

LogicUnit CircuitElementLoader::makeLogicUnit(tinyxml2::XMLElement* elem, uint64_t id)
{
	LogicUnit logic_unit;

	auto shared            = std::make_shared<LogicUnit::Shared>();
	shared->element_id     = id;
	shared->texture_id     = 2 * texture_coords[id].first + 2;
	shared->mask_id        = shared->texture_id + 1;
	shared->texture_coord  = texture_coords[id].second;
	shared->texture_extent = texture_coords[id].second.getSize();
	shared->extent         = extents[id];

	logic_unit.shared = shared;
	logic_unit.pos    = {};
	logic_unit.dir    = Direction::Up;

	return logic_unit;
}

Rect CircuitElementLoader::getExtent(tinyxml2::XMLElement* elem)
{
	auto drawings = elem->FirstChildElement("Appearance")->FirstChildElement();
	float left = 0.f, top = 0.f, right = 0.f, bottom = 0.f;

	do {
		if (!std::strcmp(drawings->Name(), "Rect")) {
			auto size   = parse_vec2(drawings, "size");
			auto pos    = parse_vec2(drawings, "pos");
			auto origin = parse_vec2(drawings, "origin");

			left   = std::min(pos.x - origin.x, left);
			top    = std::min(pos.y - origin.y, top);
			right  = std::max(pos.x - origin.x + size.x, right);
			bottom = std::max(pos.y - origin.y + size.y, bottom);
		}
	} while (drawings = drawings->NextSiblingElement());

	return Rect(left, top, right - left, bottom - top);	
}
