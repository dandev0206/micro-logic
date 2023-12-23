#include "circuit_element_loader.h"

#include "vk2d/graphics/vertex_buffer.h"
#include <tinyxml2.h>
#include <regex>

#define RAD(degree)(3.14159265358979f / 180.f * (degree))

static vec2 parse_vec2(const tinyxml2::XMLElement* element, const char* name)
{
	static const std::regex re("", std::regex::optimize);

	auto* str = element->Attribute(name);
	size_t size;

	return { std::stof(str, &size), std::stof(str + size + 1) };
}

static int hex_to_int(char ch)
{
	if ('0' <= ch && ch <= '9') return ch - '0';
	if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
	if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
	return -1;
}

static int parse_int(const tinyxml2::XMLElement* element, const char* name)
{
	return element->IntAttribute(name);
}

static float parse_float(const tinyxml2::XMLElement* element, const char* name)
{
	return element->FloatAttribute(name);
}

static vk2d::Color parse_color(const tinyxml2::XMLElement* element, const char* name)
{
	static const std::regex re("^#(?:[0-9a-fA-F]{3,4}){2}$", std::regex::optimize);

	std::string str = element->Attribute(name);
	
	if (!std::regex_match(str, re))
		return vk2d::Colors::Red;
	
	vk2d::Color color;

	color.r = 16 * hex_to_int(str[1]) + hex_to_int(str[2]);
	color.g = 16 * hex_to_int(str[3]) + hex_to_int(str[4]);
	color.b = 16 * hex_to_int(str[5]) + hex_to_int(str[6]);
	if (str.size() > 7)
		color.a = 16 * hex_to_int(str[7]) + hex_to_int(str[8]);
	else
		color.a = 255;

	return color;
}

static std::string parse_string(const tinyxml2::XMLElement* element, const char* name) {
	const char* str = element->Attribute(name);
	return str ? std::string(str) : std::string();
}

static std::string to_lower(const char* c_str) {
	std::string result;

	result.reserve(strlen(c_str) + 1);
	while (*c_str) 
		result += std::tolower(*(c_str)++);

	return result;
}

void CircuitElementLoader::load(const char* dir, const vk2d::Font& font)
{
	logic_gates.clear();
	textures.clear();
	texture_datas.clear();

	this->font = &font;

	tinyxml2::XMLDocument doc;
	doc.LoadFile(dir);

	auto* root = doc.RootElement();

	grid_pixel_size = parse_float(root, "grid_pixel_size");
	grid_scale      = DEFAULT_GRID_SIZE / grid_pixel_size;
	scailing        = parse_float(root, "scailing");

	uint32_t id = 0;

	{ // create gates and get extent
		auto* elem = root->FirstChildElement("LogicGate");
		for (; elem; elem = elem->NextSiblingElement("LogicGate"), id++) {
			auto& gate  = logic_gates.emplace_back();
			auto extent = getExtent(elem);
			
			gate.shared                 = std::make_shared<LogicGate::Shared>();
			gate.shared->name           = parse_string(elem, "name");
			gate.shared->category       = parse_string(elem, "category");
			gate.shared->description    = parse_string(elem, "description");
			gate.shared->element_id     = id;
			gate.shared->shared_id      = id;
			gate.shared->extent         = scale_rect(extent, 1.f / grid_pixel_size);
			gate.shared->texture_coord  = scale_rect(extent, scailing);
			gate.shared->texture_extent = gate.shared->texture_coord.getSize();

			getPinLayouts(elem, gate.shared->pin_layouts);

			gate.pos = {};
			gate.dir = Direction::Up;
			gate.pins.resize(gate.shared->pin_layouts.size(), {});
		}
	}

	calculatePacking();

	{ // load elements
		auto* elem = root->FirstChildElement("LogicGate");
		for (id = 0; elem; elem = elem->NextSiblingElement("LogicGate"), id++)
			renderTexture(elem->FirstChildElement("Appearance")->FirstChildElement(), id);

		for (auto& data : texture_datas) {
			data.texture.draw(data.drawlist);
			data.texture.display();

			data.image_mask = data.texture.getTexture().getImage();
			data.image_mask = data.image_mask.createMask(vk2d::Colors::Transparent, vk2d::Colors::White, false);

			textures.emplace_back(data.texture.release());
			
			auto& texture_mask = textures.emplace_back();
			texture_mask.resize(data.image_mask.size());
			texture_mask.update(data.image_mask);
		}

		elem = root->FirstChildElement("LogicGate");
		for (id = 0; elem; elem = elem->NextSiblingElement("LogicGate"), id++) {
			auto& shared     = *logic_gates[id].shared;
			auto& draw_data  = texture_datas[shared.texture_id / 2];

			shared.texture      = &textures[shared.texture_id];
			shared.texture_mask = &textures[shared.texture_mask_id];
			shared.image_mask   = draw_data.image_mask.blit(shared.texture_coord);
		}
	}
}

void CircuitElementLoader::calculatePacking()
{
	const auto max_size = vk2d::Texture::getMaximumSize();
	const auto padding  = 5.f;
	
	uint32_t texture_id = 0;
	uint32_t gate_idx   = 0;

	while (gate_idx < logic_gates.size()) {
		uint32_t max_height = 0;
		uvec2 cursor(0, 0);

		for (; gate_idx < logic_gates.size(); ++gate_idx) {
			auto& shared = *logic_gates[gate_idx].shared;
			auto& rect   = shared.texture_coord;
			auto bb      = rect.getSize();
			
			if (max_size.x < cursor.x + bb.x + padding) {
				cursor.x  = 0;
				cursor.y += max_height;
			}

			if (max_size.y < cursor.y + bb.y + padding) {
				break;
			}

			shared.texture_id      = texture_id;
			shared.texture_mask_id = texture_id + 1;
			shared.texture_coord.setPosition(cursor);

			cursor.x  += rect.width + padding;
			max_height = std::max(max_height, (uint32_t)(rect.height + padding));
		}

		uvec2 texture_size;
		texture_size.x = cursor.y == 0 ? cursor.x : max_size.x;
		texture_size.y = cursor.y + max_height;

		texture_datas.emplace_back().texture.resize(texture_size);
		texture_id += 2;
	}
}

void CircuitElementLoader::renderTexture(tinyxml2::XMLElement* drawings, uint64_t id)
{
	auto& shared      = *logic_gates[id].shared;
	auto& drawlist    = texture_datas[shared.texture_id / 2].drawlist;
	auto texture_pos  = shared.texture_coord.getPosition();
	auto texture_size = shared.texture_coord.getSize();
	auto translate    = texture_pos - scailing * grid_pixel_size * shared.extent.getPosition();

	for (; drawings; drawings = drawings->NextSiblingElement()) {
		auto& cmd  = drawlist.getCommand();
		auto shape = to_lower(drawings->Name());

		if (shape == "rect") {
			auto size   = scailing * grid_scale * parse_vec2(drawings, "size");
			auto pos    = scailing * grid_scale * parse_vec2(drawings, "pos");
			auto origin = scailing * grid_scale * parse_vec2(drawings, "origin");
			auto color  = parse_color(drawings, "col");

			cmd.addFilledRect(pos + translate - origin, size, color);
		} else if (shape == "triangle") {
			auto p0    = scailing * grid_scale * parse_vec2(drawings, "p0");
			auto p1    = scailing * grid_scale * parse_vec2(drawings, "p1");
			auto p2    = scailing * grid_scale * parse_vec2(drawings, "p2");
			auto color = parse_color(drawings, "col");

			cmd.addFilledTriangle(p0, p1, p2, color);
		} else if (shape == "circlefan") {
			auto pos    = scailing * grid_scale * parse_vec2(drawings, "pos");
			auto origin = scailing * grid_scale * parse_vec2(drawings, "origin");
			auto radius = scailing * grid_scale * parse_float(drawings, "radius");
			auto t_min  = RAD(parse_float(drawings, "t_min"));
			auto t_max  = RAD(parse_float(drawings, "t_max"));
			auto color  = parse_color(drawings, "col");

			cmd.addFilledCircleFan(pos + translate - origin, radius, t_min, t_max, color);
		} else if (shape == "text") {
			vk2d::TextStyle style;

			std::string text = drawings->GetText();
			vec2 pos         = scailing * grid_scale * parse_vec2(drawings, "pos");
			
			style.font       = font;
			style.size       = scailing * parse_int(drawings, "size");
			style.align      = parse_vec2(drawings, "align");

			drawlist.addText(text, pos + translate, style);
		}
	}
}

void CircuitElementLoader::getPinLayouts(tinyxml2::XMLElement* elem, std::vector<PinLayout>& pin_layouts)
{
	elem = elem->FirstChildElement("Pins")->FirstChildElement("Pin");

	for (; elem; elem = elem->NextSiblingElement("Pin")) {
		auto& layout = pin_layouts.emplace_back();

		auto io = to_lower(elem->Attribute("io"));
		if (io == "input")
			layout.io = PinLayout::Input;
		else if (io == "output")
			layout.io = PinLayout::Output;
		else if (io == "vcc" || io == "vdd")
			layout.io = PinLayout::VCC;
		else if (io == "gnd")
			layout.io = PinLayout::GND;
		else {
			// TODO: show error dialog
		}

		layout.pinout = atoi(elem->Attribute("pinout"));
		layout.pos    = parse_vec2(elem, "pos");
	}
}

Rect CircuitElementLoader::getExtent(tinyxml2::XMLElement* elem)
{
	auto drawings = elem->FirstChildElement("Appearance")->FirstChildElement();
	float left = 0.f, top = 0.f, right = 0.f, bottom = 0.f;

	do {
		if (!std::strcmp(drawings->Name(), "Rect")) {
			auto size   = grid_scale * parse_vec2(drawings, "size");
			auto pos    = grid_scale * parse_vec2(drawings, "pos");
			auto origin = grid_scale * parse_vec2(drawings, "origin");

			left   = std::min(pos.x - origin.x, left);
			top    = std::min(pos.y - origin.y, top);
			right  = std::max(pos.x - origin.x + size.x, right);
			bottom = std::max(pos.y - origin.y + size.y, bottom);
		}
	} while (drawings = drawings->NextSiblingElement());

	return Rect(left, top, right - left, bottom - top);	
}
