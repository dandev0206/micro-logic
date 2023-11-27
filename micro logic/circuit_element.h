#pragma once

#include "direction.h"
#include "aabb.hpp"

#include <vk2d/graphics/image.h>
#include <vk2d/graphics/draw_list.h>
#include <memory>

#include "serialize.h"

#define DEFAULT_GRID_SIZE (30.f)

class CircuitElement : public Serialrizable {
public:
	enum Style {
		None     = 0,
		Hidden   = 1 << 0,
		Hovered  = 1 << 1,
		Selected = 1 << 2,
		Cut      = 1 << 3,
		Blocked  = 1 << 4,
	};

	enum Type {
		LogicGate,
		LogicUnit,
		Wire,
		Net
	};

	using style_t = uint32_t;

	static std::unique_ptr<CircuitElement> create(std::istream& is);

	CircuitElement();
	virtual ~CircuitElement();

	virtual void draw(vk2d::DrawList& draw_list) const = 0;
	virtual std::unique_ptr<CircuitElement> clone(int32_t new_id = -1) const = 0;
	virtual AABB getAABB() const = 0;
	virtual bool hit(const AABB& aabb) const = 0;
	virtual bool hit(const vec2& pos) const = 0;
	virtual void transform(const vec2& delta, const vec2& origin, Direction rotation) = 0;
	virtual uint32_t getSelectFlagsMask() const = 0;
	virtual uint32_t getSelectFlags(const AABB& aabb) const = 0;
	virtual uint32_t getCurrSelectFlags() const = 0;
	virtual uint32_t select(uint32_t flags = UINT_MAX) = 0;
	virtual uint32_t unselect(uint32_t flags = UINT_MAX) = 0;
	virtual void setHover(uint32_t flags = UINT_MAX) = 0;
	virtual void clearHover() = 0;
	virtual Type getType() const = 0;

	bool isSelected() const;
	bool isWireBased() const;

	int32_t id;
	style_t style;
};

class RigidElement abstract : public CircuitElement {
public:
	AABB getAABB() const override;
	bool hit(const AABB& aabb) const override;
	bool hit(const vec2& pos) const override;
	void transform(const vec2& delta, const vec2& origin, Direction rotation) override;

	uint32_t getSelectFlagsMask() const;
	uint32_t getSelectFlags(const AABB& aabb) const override;
	uint32_t getCurrSelectFlags() const override;
	uint32_t select(uint32_t flags = UINT_MAX) override;
	uint32_t unselect(uint32_t flags = UINT_MAX) override;
	void setHover(uint32_t flags = UINT_MAX) override;
	void clearHover() override;


	struct Shared {
		std::string name;
		std::string description;
		uint64_t    shared_id;
		uint64_t    element_id;
		uint64_t    texture_id;
		uint64_t    mask_id;
		Rect        texture_coord;
		uvec2       texture_extent;
		Rect        extent;
		vk2d::Image image_mask;
	};

	std::shared_ptr<Shared> shared;

	vec2      pos;
	Direction dir;
};

class LogicGate : public RigidElement {
public:
	void serialize(std::ostream& os) const override;

	std::unique_ptr<CircuitElement> clone(int32_t new_id = -1) const override;
	void draw(vk2d::DrawList& draw_list) const override;
	Type getType() const override;
};

class LogicUnit : public RigidElement {
public:
	void serialize(std::ostream& os) const override;

	std::unique_ptr<CircuitElement> clone(int32_t new_id = -1) const override;
	void draw(vk2d::DrawList& draw_list) const override;
	Type getType() const override;

};

class WireElement abstract : public CircuitElement {
public:
	WireElement();
	WireElement(const vec2& p0, const vec2& p1);

	void transform(const vec2& delta, const vec2& origin, Direction rotation) override;
	uint32_t getSelectFlagsMask() const override;
	uint32_t getSelectFlags(const AABB& aabb) const override;
	uint32_t getCurrSelectFlags() const override;
	uint32_t select(uint32_t flags = UINT_MAX) override;
	uint32_t unselect(uint32_t flags = UINT_MAX) override;
	void setHover(uint32_t flags = UINT_MAX) override;
	void clearHover() override;

	bool overlap(const WireElement& wire, float& t_min, float& t_max) const;

	vec2 p0;
	vec2 p1;
	bool dot0;
	bool dot1;
	bool select_p0;
	bool select_p1;
	bool hover_p0;
	bool hover_p1;
};

class Wire : public WireElement {
public:
	Wire();
	Wire(const vec2& p0, const vec2& p1);

	void serialize(std::ostream& os) const override;

	void draw(vk2d::DrawList& draw_list) const override;
	std::unique_ptr<CircuitElement> clone(int32_t new_id = -1) const override;
	AABB getAABB() const override;
	bool hit(const AABB& aabb) const override;
	bool hit(const vec2& pos) const override;
	Type getType() const override;
};