#include "schematic_sheet.h"

#include "micro_logic_config.h"
#include <algorithm>

SchematicSheet::SchematicSheet() :
	position(0, 0),
	scale(1.f),
	grid_pixel_size(DEFAULT_GRID_SIZE),
	id_counter(0),
	file_saved(false),
	is_up_to_date(false)
{}

void SchematicSheet::serialize(std::ostream& os) const
{
	write_binary_str(os, name, 256);
	write_binary_str(os, guid, 39);
	write_binary(os, position);
	write_binary(os, scale);
	write_binary(os, id_counter);
	write_binary(os, bvh.size());

	for (const auto& [aabb, elem] : bvh)
		elem->serialize(os);
}

void SchematicSheet::unserialize(std::istream& is)
{
	size_t elem_count = 0;

	read_binary_str(is, name, 256);
	read_binary_str(is, guid, 39);
	read_binary(is, position);
	read_binary(is, scale);
	read_binary(is, id_counter);
	read_binary(is, elem_count);

	for (size_t i = 0; i < elem_count; ++i) {
		auto new_elem = CircuitElement::create(is);
		auto& elem    = *new_elem;

		elem.iter = bvh.insert(elem.getAABB(), std::move(new_elem));
	}
}

bool SchematicSheet::empty() const
{
	return bvh.empty();
}

void SchematicSheet::setPosition(const vec2& pos)
{
	position = pos;
}

void SchematicSheet::setScale(float scale)
{
	scale = std::clamp(scale, MIN_SCALE, MAX_SCALE);

	this->scale = scale;
	grid_pixel_size = DEFAULT_GRID_SIZE * scale;
}