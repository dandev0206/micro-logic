#pragma once

#include <vk2d/graphics/render_texture.h>
#include "circuit_element.h"
#include "serialize.h"
#include "bvh.hpp"

#define CMD_ONLY

class SchematicSheet : public Serialrizable, public Unserialrizable {
public:
	using bvh_iterator_t = typename BVH<std::unique_ptr<CircuitElement>>::iterator;

	SchematicSheet();
	SchematicSheet(SchematicSheet&& rhs) noexcept = default;
	~SchematicSheet() = default;

	SchematicSheet& operator=(SchematicSheet&& rhs) noexcept = default;

	void serialize(std::ostream& os) const override;
	void unserialize(std::istream& is) override;

public:
	void setPosition(const vec2& pos);
	void setScale(float scale);

public:
	std::string name;
	std::string guid;

	vec2  position;
	float scale;
	float grid_pixel_size;

	CMD_ONLY BVH<std::unique_ptr<CircuitElement>> bvh;
	CMD_ONLY std::vector<bvh_iterator_t>          selections;
	CMD_ONLY uint32_t                             id_counter;

	vk2d::Texture thumbnail;

	bool file_saved;
};