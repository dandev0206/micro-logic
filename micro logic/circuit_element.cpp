#include "circuit_element.h"

#include "main_window.h"
#include "math_utils.h"
#include "sdf.h"

#define TEXTURE_ID_OFF 2

static inline bool on_same_line(const vec2& pa0, const vec2& pa1, const vec2& pb0, const vec2& pb1) {
	if (std::abs((pa1.y - pa0.y) * (pb1.x - pb0.x) - (pb1.y - pb0.y) * (pa1.x - pa0.x)) > 1e-7) return false;
	if (std::abs((pa1.y - pa0.y) * (pb1.x - pa0.x) - (pb1.y - pa0.y) * (pa1.x - pa0.x)) > 1e-7) return false;
	return true;
}

static inline uint32_t  get_intersection_code(const AABB& aabb, const vec2& p) {
	uint32_t code = 0;

	if (p.x < aabb.min.x)
		code |= 1 << 0;
	else if (p.x > aabb.max.x)
		code |= 1 << 1;

	if (p.y < aabb.min.y)
		code |= 1 << 2;
	else if (p.y > aabb.max.y)
		code |= 1 << 3;

	return code;
}

Pin::Pin() :
	net(nullptr)
{}

std::unique_ptr<CircuitElement> CircuitElement::create(std::istream& is)
{
	Type type;
	read_binary(is, type);
	
	auto& main_window = MainWindow::get();

	switch (type) {
	case Type::LogicGate: {
		auto elem = std::make_unique<::LogicGate>();

		uint64_t shared_id;

		read_binary(is, elem->id);
		read_binary(is, elem->style);
		read_binary(is, shared_id);
		read_binary(is, elem->pos);
		read_binary(is, elem->dir);

		elem->shared = main_window.logic_gates[shared_id].shared;

		return elem;
	}
	case Type::LogicUnit: {
		auto elem = std::make_unique<::LogicUnit>();

		uint64_t shared_id;

		read_binary(is, elem->id);
		read_binary(is, elem->style);
		read_binary(is, shared_id);
		read_binary(is, elem->pos);
		read_binary(is, elem->dir);

		elem->shared = main_window.logic_units[shared_id].shared;

		return elem;
	}
	case Type::Wire: {
		auto elem = std::make_unique<::Wire>();

		read_binary(is, elem->id);
		read_binary(is, elem->style);
		read_binary(is, elem->p0);
		read_binary(is, elem->p1);
		read_binary(is, elem->dot0);
		read_binary(is, elem->dot1);
		read_binary(is, elem->select_p0);
		read_binary(is, elem->select_p1);
		read_binary(is, elem->hover_p0);
		read_binary(is, elem->hover_p1);

		return elem;
	}
	case Type::Net: {
		//auto elem = std::make_unique<::Net>();

	}
	default:
		return nullptr;
	}
}

CircuitElement::CircuitElement() :
	id(-1),
	style(Style::None)
{}

CircuitElement::~CircuitElement()
{
}

bool CircuitElement::isSelected() const
{
	return style & CircuitElement::Selected;
}

bool CircuitElement::isWireBased() const
{
	auto type = getType();
	return type == Wire || type == Net;
}

RigidElement::RigidElement() :
	pos(0.f, 0.f),
	dir(Direction::Up)
{}

void RigidElement::transform(const vec2& delta, const vec2& origin, Direction rotation)
{
	if (rotation != Direction::Up) {
		pos = rotate_vector(pos + delta - origin, rotation) + origin;
		dir = rotate_dir(dir, rotation);
	} else {
		pos += delta;
	}
}

uint32_t RigidElement::getSelectFlagsMask() const
{
	return 0x00000001;
}

uint32_t RigidElement::getSelectFlags(const AABB& aabb) const
{
	return getAABB().overlap(aabb);
}

uint32_t RigidElement::getCurrSelectFlags() const
{
	return (bool)(style & Style::Selected);
}

uint32_t RigidElement::select(uint32_t flags)
{
	if ((flags & 1) && !(style & Style::Selected)) {
		style |= Style::Selected;
		return 1;
	}

	return 0;
}

uint32_t RigidElement::unselect(uint32_t flags)
{
	if ((flags & 1) && (style & Style::Selected)) {
		style &= ~Style::Selected;
		return 1;
	}

	return 0;
}

void RigidElement::setHover(uint32_t flags)
{
	if (flags) style |= Style::Hovered;
}

void RigidElement::clearHover()
{
	style &= ~Style::Hovered;
}

AABB LogicElement::getAABB() const
{
	AABB aabb = rotate_rect(shared->extent, dir);
	return { aabb.min + pos, aabb.max + pos };
}

bool LogicElement::hit(const AABB& aabb) const
{
	return getAABB().overlap(aabb);
}

bool LogicElement::hit(const vec2& pos) const
{
	auto rect  = shared->extent;
	auto& mask = shared->image_mask;
	auto size  = mask.size();

	auto p = rotate_vector(pos - this->pos, invert_dir(dir)) - rect.getPosition();
	p *= DEFAULT_GRID_SIZE;

	if (p.x < 0 || size.x <= p.x || p.y < 0 || size.y <= p.y) return false;

	return mask.getPixel((uint32_t)p.x, (uint32_t)p.y).a != 0;
}

Pin* LogicElement::getPin(const vec2& pos)
{
	auto point = rotate_vector(pos - this->pos, invert_dir(dir));

	for (const auto& layout : shared->pin_layouts)
		if (equal(point, layout.pos))
			return &pins[layout.pinout - 1];

	return nullptr;
}

void LogicGate::serialize(std::ostream& os) const
{
	write_binary(os, Type::LogicGate);
	write_binary(os, id);
	write_binary(os, style);
	write_binary(os, shared->shared_id);
	write_binary(os, pos);
	write_binary(os, dir);
}

std::unique_ptr<CircuitElement> LogicGate::clone(int32_t new_id) const
{
	auto new_one = std::make_unique<LogicGate>(*this);
	if (new_id != -1) new_one->id = new_id;
	return std::move(new_one);
}

void LogicGate::draw(vk2d::DrawList& draw_list) const
{
	if (style & Style::Hidden) return;

	auto rect = shared->extent;
	auto texture_rect = shared->texture_coord;
	auto x = rect.width;
	auto y = rect.height;
	auto tx = texture_rect.width;
	auto ty = texture_rect.height;

	vk2d::Color color(255, 255, 255, style & Style::Cut ? 128 : 255);

	vec2 p0(rect.left, rect.top);
	vec2 p1(rect.left + x, rect.top);
	vec2 p2(rect.left + x, rect.top + y);
	vec2 p3(rect.left, rect.top + y);

	p0 = rotate_vector(p0, dir) + pos;
	p1 = rotate_vector(p1, dir) + pos;
	p2 = rotate_vector(p2, dir) + pos;
	p3 = rotate_vector(p3, dir) + pos;

	vk2d::Vertex v0(p0, color, { texture_rect.left, texture_rect.top });
	vk2d::Vertex v1(p1, color, { texture_rect.left + tx, texture_rect.top });
	vk2d::Vertex v2(p2, color, { texture_rect.left + tx, texture_rect.top + ty });
	vk2d::Vertex v3(p3, color, { texture_rect.left, texture_rect.top + ty });

	{
		auto& cmd = draw_list[shared->texture_id + TEXTURE_ID_OFF];

		auto idx = cmd.reservePrims(4, 6);

		cmd.vertices.emplace_back(v0);
		cmd.vertices.emplace_back(v1);
		cmd.vertices.emplace_back(v2);
		cmd.vertices.emplace_back(v3);

		cmd.indices.emplace_back(idx + 0);
		cmd.indices.emplace_back(idx + 1);
		cmd.indices.emplace_back(idx + 2);
		cmd.indices.emplace_back(idx + 0);
		cmd.indices.emplace_back(idx + 3);
		cmd.indices.emplace_back(idx + 2);
	}

	if (!(style & (Style::Hovered | Style::Selected | Style::Blocked))) return;

	vk2d::Color mask_color;

	if (style & Style::Blocked) {
		mask_color = vk2d::Color(255, 0, 0, 48);
	} else if (style & Style::Hovered) {
		mask_color = vk2d::Color(255, 255, 255, 32);
	} else if (style & Style::Selected) {
		mask_color = vk2d::Color(0, 255, 0, 16);
	}

	v0.color = mask_color;
	v1.color = mask_color;
	v2.color = mask_color;
	v3.color = mask_color;

	{
		auto& cmd = draw_list[shared->texture_mask_id + TEXTURE_ID_OFF];

		auto idx = cmd.reservePrims(4, 6);

		cmd.vertices.emplace_back(v0);
		cmd.vertices.emplace_back(v1);
		cmd.vertices.emplace_back(v2);
		cmd.vertices.emplace_back(v3);

		cmd.indices.emplace_back(idx + 0);
		cmd.indices.emplace_back(idx + 1);
		cmd.indices.emplace_back(idx + 2);
		cmd.indices.emplace_back(idx + 0);
		cmd.indices.emplace_back(idx + 3);
		cmd.indices.emplace_back(idx + 2);
	}
}

CircuitElement::Type LogicGate::getType() const
{
	return CircuitElement::LogicGate;
}

void LogicUnit::serialize(std::ostream& os) const
{
}

std::unique_ptr<CircuitElement> LogicUnit::clone(int32_t new_id) const
{
	auto new_one = std::make_unique<LogicUnit>(*this);
	if (new_id != -1) new_one->id = new_id;
	return std::move(new_one);
}

void LogicUnit::draw(vk2d::DrawList& draw_list) const
{

}

CircuitElement::Type LogicUnit::getType() const
{
	return CircuitElement::LogicUnit;
}

WireElement::WireElement() :
	p0(0.f, 0.f),
	p1(0.f, 0.f),
	dot0(false),
	dot1(false),
	select_p0(false),
	select_p1(false),
	hover_p0(false),
	hover_p1(false)
{}

WireElement::WireElement(const vec2& p0, const vec2& p1) :
	p0(p0),
	p1(p1),
	dot0(false),
	dot1(false),
	select_p0(false),
	select_p1(false),
	hover_p0(false),
	hover_p1(false)
{}

void WireElement::transform(const vec2& delta, const vec2& origin, Direction rotation)
{
	if (select_p0)
		p0 = rotate_vector(p0 + delta - origin, rotation) + origin;
	if (select_p1)
		p1 = rotate_vector(p1 + delta - origin, rotation) + origin;
}

uint32_t WireElement::getSelectFlagsMask() const
{
	return 0x00000003;
}

uint32_t WireElement::getSelectFlags(const AABB& aabb) const
{
	auto code0 = get_intersection_code(aabb, p0);
	auto code1 = get_intersection_code(aabb, p1);

	if (!code0 && !code1) 
		return 3;
	if (!code0 != !code1)
		return (!code0 << 0) | (!code1 << 1);
	if (!(code0 & code1)) { // might intersect
		if (is_vertical(p0, p1) && in_range(p0.x, aabb.min.x, aabb.max.x))
			return 3;

		float m = slope(p0, p1);

		float y = m * (aabb.min.x - p1.x) + p1.y;
		if (in_range(y, aabb.min.y, aabb.max.y)) return 3;

		y = m * (aabb.max.x - p1.x) + p1.y;
		if (in_range(y, aabb.min.y, aabb.max.y)) return 3;

		float x = (aabb.min.y - p1.y) / m + p1.x;
		if (in_range(x, aabb.min.x, aabb.max.x)) return 3;

		x = (aabb.max.y - p1.y) / m + p1.x;
		if (in_range(x, aabb.min.x, aabb.max.x)) return 3;
	}

	return 0;
}

uint32_t WireElement::getCurrSelectFlags() const
{
	return style & Style::Selected ? (select_p0 << 0) | (select_p1 << 1) : 0;
}

uint32_t WireElement::select(uint32_t flags)
{
	uint32_t delta = 0;

	if ((flags >> 0) & 1) {
		delta |= !select_p0 << 0;
		select_p0 = true;
	}
	if ((flags >> 1) & 1) {
		delta |= !select_p1 << 1;
		select_p1 = true;
	}

	if (select_p0 || select_p1)
		style |= Style::Selected;
	else
		style &= ~Style::Selected;

	return delta;
}

uint32_t WireElement::unselect(uint32_t flags)
{
	uint32_t delta = 0;

	if ((flags >> 0) & 1) {
		delta |= select_p0 << 0;
		select_p0 = false;
	}
	if ((flags >> 1) & 1) {
		delta |= select_p1 << 1;
		select_p1 = false;
	}

	if (select_p0 || select_p1)
		style |= Style::Selected;
	else
		style &= ~Style::Selected;

	return delta;
}

void WireElement::setHover(uint32_t flags)
{
	hover_p0 = (flags >> 0) & 1;
	hover_p1 = (flags >> 1) & 1;
	style |= Style::Hovered;
}

void WireElement::clearHover()
{
	hover_p0 = false;
	hover_p1 = false;
	style &= ~Style::Hovered;
}

bool WireElement::overlap(const WireElement& wire, float& t_min, float& t_max) const
{
	if (!on_same_line(p0, p1, wire.p0, wire.p1)) return false;

	float t0, t1;

	if (is_vertical(p0, p1)) {
		t0 = unlerp(wire.p0.y, wire.p1.y, p0.y);
		t1 = unlerp(wire.p0.y, wire.p1.y, p1.y);
	} else {
		t0 = unlerp(wire.p0.x, wire.p1.x, p0.x);
		t1 = unlerp(wire.p0.x, wire.p1.x, p1.x);
	}

	if (!(in_range(t0, 0.f, 1.f) || in_range(t1, 0.f, 1.f) || t0 * t1 < 0)) return false;

	t0 = std::clamp(t0, 0.f, 1.f);
	t1 = std::clamp(t1, 0.f, 1.f);

	t_min = std::min(t0, t1);
	t_max = std::max(t0, t1);

	return true;
}

Wire::Wire() :
	WireElement()
{}

Wire::Wire(const vec2& p0, const vec2& p1) :
	WireElement(p0, p1)
{}

void Wire::serialize(std::ostream & os) const
{
	write_binary(os, Type::Wire);
	write_binary(os, id);
	write_binary(os, style);
	write_binary(os, p0);
	write_binary(os, p1);
	write_binary(os, dot0);
	write_binary(os, dot1);
	write_binary(os, select_p0);
	write_binary(os, select_p1);
	write_binary(os, hover_p0);
	write_binary(os, hover_p1);
}

void Wire::draw(vk2d::DrawList& draw_list) const
{
	if (style & Style::Hidden) return;

	auto& cmd = draw_list[1];

	vk2d::Color color(50, 177, 108, style & Style::Cut ? 128 : 255);

	if ((style & Style::Selected)) {
		vk2d::Color color_select(188, 255, 163);

		if (select_p0 && select_p1)
			cmd.addFilledCapsule(p0, p1, 4 / DEFAULT_GRID_SIZE, color_select);
		else {
			vec2 mid = (p0 + p1) / 2.f;

			if (select_p0) {
				cmd.addFilledHalfCapsule(p0, mid, 4 / DEFAULT_GRID_SIZE, color_select, color);
				cmd.addFilledHalfCapsule(p1, mid, 4 / DEFAULT_GRID_SIZE, color, color);
			} else {
				cmd.addFilledHalfCapsule(p0, mid, 4 / DEFAULT_GRID_SIZE, color, color);
				cmd.addFilledHalfCapsule(p1, mid, 4 / DEFAULT_GRID_SIZE, color_select, color);
			}
		}
	} else if (style & Style::Hovered) {
		vk2d::Color color_hovered(143, 241, 124);

		if (hover_p0) 
			cmd.addFilledCircle(p0, 8 / DEFAULT_GRID_SIZE, color_hovered);
		if (hover_p1) 
			cmd.addFilledCircle(p1, 8 / DEFAULT_GRID_SIZE, color_hovered);

		if (hover_p0 && hover_p1)
			cmd.addFilledCapsule(p0, p1, 4 / DEFAULT_GRID_SIZE, color_hovered);
		else 
			cmd.addFilledCapsule(p0, p1, 4 / DEFAULT_GRID_SIZE, color);
	} else {
		cmd.addFilledCapsule(p0, p1, 4 / DEFAULT_GRID_SIZE, color);
	}

	if (dot0) cmd.addFilledCircle(p0, 6 / DEFAULT_GRID_SIZE, color);
	if (dot1) cmd.addFilledCircle(p1, 6 / DEFAULT_GRID_SIZE, color);
}

std::unique_ptr<CircuitElement> Wire::clone(int32_t new_id) const
{
	auto new_one = std::make_unique<Wire>(*this);
	if (new_id != -1) new_one->id = new_id;
	return std::move(new_one);
}

AABB Wire::getAABB() const
{
	float thickness = 0.4f;

	AABB aabb;
	aabb.min.x = std::min(p0.x - thickness, p1.x - thickness);
	aabb.min.y = std::min(p0.y - thickness, p1.y - thickness);
	aabb.max.x = std::max(p0.x + thickness, p1.x + thickness);
	aabb.max.y = std::max(p0.y + thickness, p1.y + thickness);
	return aabb;
}

bool Wire::hit(const AABB& aabb) const
{
	auto code0 = get_intersection_code(aabb, p0);
	auto code1 = get_intersection_code(aabb, p1);

	return !(code0 & code1);
}

bool Wire::hit(const vec2& pos) const
{
	float sdf0 = SDF_circle(pos, p0, 0.4f);
	float sdf1 = SDF_circle(pos, p1, 0.4f);

	if (sdf0 < 0.f || sdf1 < 0.f) return true;

	vec2 v0 = pos - p0;
	vec2 v1 = pos - p1;
	vec2 v2 = p1 - p0;

	if (dot(v0, v2) < 0 || dot(v1, -v2) < 0) return false;

	float theta = acosf(dot(v0, v2) / (glm::length(v0) * glm::length(v2)));

	return glm::length(v0) * sinf(theta) < (6 / DEFAULT_GRID_SIZE);
}

CircuitElement::Type Wire::getType() const
{
	return CircuitElement::Wire;
}