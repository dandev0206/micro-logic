#include "commands.h"

using bvh_iterator_t = typename BVH<std::unique_ptr<CircuitElement>>::iterator;

static AABB transform_AABB(const AABB& aabb, const vec2& delta, const vec2& origin, Direction dir) {
	AABB result;
	result.min = rotate_vector(aabb.min + delta - origin, dir) + origin;
	result.max = rotate_vector(aabb.max + delta - origin, dir) + origin;

	if (result.min.x > result.max.x) std::swap(result.min.x, result.max.x);
	if (result.min.y > result.max.y) std::swap(result.min.y, result.max.y);

	return result;
}

CommandGroup::CommandGroup() :
	modifying(true)
{}

void CommandGroup::onPush(SchematicSheet& sheet)
{
	for (auto iter = commands.begin(); iter != commands.end(); ++iter)
		(*iter)->onPush(sheet);
}

void CommandGroup::redo(SchematicSheet& sheet)
{
	for (auto iter = commands.begin(); iter != commands.end(); ++iter)
		(*iter)->redo(sheet);
}

void CommandGroup::undo(SchematicSheet& sheet)
{
	for (auto iter = commands.rbegin(); iter != commands.rend(); ++iter)
		(*iter)->undo(sheet);
}

std::string CommandGroup::what() const
{
	return description;
}

bool CommandGroup::isModifying() const
{
	return modifying;
}

void CommandGroup::append(std::unique_ptr<Command>&& cmd)
{
	commands.emplace_back(std::move(cmd));
}

bool CommandGroup::empty() const
{
	return commands.empty();
}

void Command_Add::onPush(SchematicSheet& sheet)
{
	assert(elements.size() > 0);

	item_count = elements.size();
}

void Command_Add::redo(SchematicSheet& sheet)
{
	assert(elements.size() > 0);
	assert(refs.capacity() == 0);

	refs.reserve(item_count);

	for (auto& elem_ptr : elements) {
		auto& elem = *elem_ptr;
		
		auto iter = sheet.bvh.insert(elem.getAABB(), std::move(elem_ptr));
		elem.id   = sheet.id_counter++;
		elem.iter = iter;

		refs.emplace_back(iter);
	}

	elements.clear();
	elements.shrink_to_fit();
}

void Command_Add::undo(SchematicSheet& sheet)
{
	assert(elements.capacity() == 0);
	assert(refs.size() > 0);

	elements.reserve(item_count);

	sheet.id_counter -= (uint32_t)item_count;

	for (auto iter : refs) {
		elements.emplace_back(std::move(iter->second));
		sheet.bvh.erase(iter);
	}

	refs.clear();
	refs.shrink_to_fit();
}

std::string Command_Add::what() const
{
	return "Add " + std::to_string(item_count) + " Item(s)";
}

void Command_Select::onPush(SchematicSheet& sheet)
{
	assert(type != Clear || selections.empty() && aabb == AABB());
	assert(type != SelectAll || selections.empty() && aabb == AABB());
	assert(type != SelectAppend || !selections.empty() && aabb == AABB());
	assert(type != SelectInvert || selections.empty() && aabb != AABB());
	assert(type != Unselect || !selections.empty() && aabb == AABB());
}

void Command_Select::redo(SchematicSheet& sheet)
{
	if (type == Clear) {
		assert(sheet.selections.size() > 0);
		assert(selections.capacity() == 0);

		for (auto iter : sheet.selections) {
			auto& elem = *iter->second;
			
			selections.emplace_back(&elem, elem.unselect());
		}

		sheet.selections.clear();
	} else if (type == SelectAll) {
		assert(sheet.selections.size() != sheet.bvh.size());
		assert(selections.capacity() == 0);

		for (auto iter : sheet.selections) {
			auto& elem = *iter->second;

			selections.emplace_back(&elem, elem.getCurrSelectFlags());
		}

		for (auto iter = sheet.bvh.begin(); iter != sheet.bvh.end(); ++iter) {
			auto& elem = *iter->second;

			bool already_selected = elem.isSelected();
			elem.select();

			if (!already_selected)
				sheet.selections.emplace_back(iter);
		}
	} else if (type == SelectAppend) {
		assert(selections.size() > 0);

		for (auto selection : selections) {
			auto& elem = *selection.first;

			assert(!(elem.getCurrSelectFlags() & selection.second));

			bool already_selected = elem.isSelected();
			elem.select(selection.second);

			if (!already_selected)
				sheet.selections.emplace_back(elem.iter);
		}
	} else if (type == SelectInvert) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = *iter->second;

			auto old_flags = elem.getCurrSelectFlags();
			elem.unselect();
			elem.select(~old_flags);
			auto new_flags = elem.getCurrSelectFlags();

			if (!old_flags && new_flags)
				sheet.selections.emplace_back(iter);
			else if (old_flags && !new_flags) {
				auto item = std::find(sheet.selections.begin(), sheet.selections.end(), iter);
				sheet.selections.erase(item);
			}

			BVH_CONTINUE;
		});
	} else { // Unselect
		assert(selections.size() > 0);

		for (auto selection : selections) {
			auto& elem = *selection.first;

			assert(!(elem.getCurrSelectFlags() & selection.second));

			elem.unselect(selection.second);

			if (!elem.isSelected()) {
				auto item = std::find(sheet.selections.begin(), sheet.selections.end(), elem.iter);
				sheet.selections.erase(item);
			}
		}
	}
}

void Command_Select::undo(SchematicSheet& sheet)
{
	if (type == Clear) {
		assert(sheet.selections.empty());
		assert(selections.size() > 0);

		for (auto [elem, flags] : selections) {
			elem->select(flags);
			sheet.selections.emplace_back(elem->iter);
		}

		selections.clear();
		selections.shrink_to_fit();
	} else if (type == SelectAll) {
		for (auto iter : sheet.selections) {
			auto& elem = *iter->second;

			elem.unselect();
		}

		sheet.selections.clear();

		for (auto [elem, flags] : selections) {
			elem->select(flags);
			sheet.selections.emplace_back(elem->iter);
		}

		selections.clear();
		selections.shrink_to_fit();
	} else if (type == SelectAppend) {
		for (auto selection : selections) {
			auto& elem = *selection.first;

			elem.unselect(selection.second);

			if (!elem.isSelected()) {
				auto item = std::find(sheet.selections.begin(), sheet.selections.end(), elem.iter);
				sheet.selections.erase(item);
			}
		}
	} else if (type == SelectInvert) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			auto old_flags = elem.getCurrSelectFlags();
			elem.unselect();
			elem.select(~old_flags);
			auto new_flags = elem.getCurrSelectFlags();

			if (!old_flags && new_flags)
				sheet.selections.emplace_back(iter);
			else if (old_flags && !new_flags) {
				auto item = std::find(sheet.selections.begin(), sheet.selections.end(), iter);
				sheet.selections.erase(item);
			}

			BVH_CONTINUE;
		});
	} else { // Unselect
		for (auto selection : selections) {
			auto& elem = *selection.first;

			bool already_selected = elem.isSelected();
			elem.select(selection.second);

			if (!already_selected)
				sheet.selections.emplace_back(elem.iter);
		}
	}
}

std::string Command_Select::what() const
{
	switch (type) {
	case Clear:
		return "Clear Select";
	case SelectAll:
		return "Select All";
	case SelectAppend: 
		return "Select " + std::to_string(selections.size()) + " item(s)";
	case SelectInvert:
		return "Invert Select";
	case Unselect:
		return "Unselect " + std::to_string(selections.size()) + " item(s)";
	default:
		return "Unkown Selection Type";
	}
}

bool Command_Select::isModifying() const
{
	return false;
}

void Command_Move::onPush(SchematicSheet& sheet)
{
	item_count = sheet.selections.size();
}

void Command_Move::redo(SchematicSheet& sheet)
{
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		elem.transform(delta, origin, dir);
		sheet.bvh.update_element(selection, elem.getAABB());
	}
}

void Command_Move::undo(SchematicSheet& sheet)
{
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		elem.transform({}, origin, invert_dir(dir));
		elem.transform(-delta, {}, Direction::Up);
		sheet.bvh.update_element(selection, elem.getAABB());
	}
}

std::string Command_Move::what() const
{
	return "Move " + std::to_string(item_count) + " Item(s)";
}

void Command_Copy::onPush(SchematicSheet& sheet)
{
	item_count = sheet.selections.size();
}

void Command_Copy::redo(SchematicSheet& sheet)
{
	if (elements.empty()) { // initial
		refs.reserve(item_count);

		for (auto selection : sheet.selections) {
			auto new_elem = selection->second->clone();
			auto& elem    = *new_elem;

			elem.select();
			elem.transform(delta, origin, dir);
			elem.unselect();

			auto iter = sheet.bvh.insert(elem.getAABB(), std::move(new_elem));

			elem.id   = sheet.id_counter++;
			elem.iter = iter;

			refs.emplace_back(iter);
		}
	} else {
		assert(elements.size() > 0);
		assert(refs.capacity() == 0);

		refs.reserve(item_count);

		for (auto& new_elem : elements) {
			auto& elem = *new_elem;

			auto iter = sheet.bvh.insert(elem.getAABB(), std::move(new_elem));

			elem.id   = sheet.id_counter++;
			elem.iter = iter;

			refs.emplace_back(iter);
		}

		elements.clear();
		elements.shrink_to_fit();
	}
}

void Command_Copy::undo(SchematicSheet& sheet)
{
	assert(elements.capacity() == 0);
	assert(refs.size() > 0);

	elements.reserve(item_count);

	sheet.id_counter -= (uint32_t)item_count;

	for (auto iter : refs) {
		elements.emplace_back(std::move(iter->second));
		sheet.bvh.erase(iter);
	}

	refs.clear();
	refs.shrink_to_fit();
}

std::string Command_Copy::what() const
{
	return "Copy " + std::to_string(item_count) + " Item(s)";
}

void Command_Cut::onPush(SchematicSheet& sheet)
{
	item_count = sheet.selections.size();
}

void Command_Cut::redo(SchematicSheet& sheet)
{
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		elem.select();
		elem.transform(delta, origin, dir);
		elem.unselect();

		sheet.bvh.update_element(selection, elem.getAABB());
	}
}

void Command_Cut::undo(SchematicSheet& sheet)
{
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		elem.select();
		elem.transform({}, origin, invert_dir(dir));
		elem.transform(-delta, {}, Direction::Up);
		elem.unselect();

		sheet.bvh.update_element(selection, elem.getAABB());
	}
}

std::string Command_Cut::what() const
{
	return "Copy " + std::to_string(item_count) + " Item(s)";
}

void Command_Delete::onPush(SchematicSheet& sheet)
{
	item_count = sheet.selections.size();
}

void Command_Delete::redo(SchematicSheet& sheet)
{
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		elements.emplace_back(std::move(selection->second));
		sheet.bvh.erase(selection);
	}

	sheet.selections.clear();
}

void Command_Delete::undo(SchematicSheet& sheet)
{
	for (auto& elem : elements) {
		auto iter = sheet.bvh.insert(elem->getAABB(), std::move(elem));
		sheet.selections.emplace_back(iter);
	}

	elements.clear();
}

std::string Command_Delete::what() const
{
	return "Delete " + std::to_string(item_count) + " Item(s)";
}