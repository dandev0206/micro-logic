#include "command.h"

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
}

void Command_Add::redo(SchematicSheet& sheet)
{
	assert(elements.size());
	assert(!refs.capacity());

	refs.reserve(elements.size());

	for (auto& elem : elements) {
		auto aabb = elem->getAABB();

		elem->id = sheet.id_counter++;

		refs.emplace_back(sheet.bvh.insert(aabb, std::move(elem)));
	}

	elements.clear();
	elements.shrink_to_fit();
}

void Command_Add::undo(SchematicSheet& sheet)
{
	assert(!elements.capacity());
	assert(refs.size());

	sheet.id_counter -= (uint32_t)refs.size();

	for (auto iter : refs)
		sheet.bvh.erase(iter);
}

std::string Command_Add::what() const
{
	return "Add " + std::to_string(elements.size()) + " Item(s)";
}

void Command_Select::onPush(SchematicSheet& sheet)
{
	assert(type != Clear || selections.empty() && aabb == AABB());
	assert(type != SelectAll || selections.empty() && aabb == AABB());
	assert(type != SelectAppend || !selections.empty() && aabb != AABB());
	assert(type != SelectInvert || selections.empty() && aabb != AABB());
	assert(type != Unselect || !selections.empty() && aabb != AABB());
	
	sortSelections();
}

void Command_Select::redo(SchematicSheet& sheet)
{
	if (type == Clear) {
		aabb = sheet.selections.front()->second->getAABB();

		for (auto iter : sheet.selections) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);
			
			auto flags = elem.unselect();

			selections.emplace_back(elem.id, flags);
			aabb = aabb.union_of(elem.getAABB());
		}

		sortSelections();
		sheet.selections.clear();
	} else if (type == SelectAll) {
		aabb = sheet.bvh.begin()->first;

		for (auto iter = sheet.bvh.begin(); iter != sheet.bvh.end(); ++iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			bool already_selected = elem.isSelected();
			auto flags            = elem.select();

			selections.emplace_back(elem.id, flags);
			aabb = aabb.union_of(iter->first);

			if (!already_selected)
				sheet.selections.emplace_back(iter);
		}

		sortSelections();
	} else if (type == SelectAppend) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);
			
			if (auto selection = findSelection(elem.id)) {
				bool already_selected = elem.isSelected();
				
				elem.select(selection->second);

				if (!already_selected) 
					sheet.selections.emplace_back(iter);
			}

			BVH_CONTINUE;
		});
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
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (auto selection = findSelection(elem.id)) {
				elem.unselect(selection->second);
				auto item = std::find(sheet.selections.begin(), sheet.selections.end(), iter);
				sheet.selections.erase(item);
				
				BVH_BREAK;
			}

			BVH_CONTINUE;
		});
	}
}

void Command_Select::undo(SchematicSheet& sheet)
{
	if (type == Clear) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (auto selection = findSelection(elem.id)) {
				elem.select(selection->second);
				sheet.selections.emplace_back(iter);
			}

			BVH_CONTINUE;
		});

		selections.clear();
	} else if (type == SelectAll) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (auto selection = findSelection(elem.id)) {
				elem.unselect(selection->second);

				if (!elem.isSelected()) {
					auto item = std::find(sheet.selections.begin(), sheet.selections.end(), iter);
					sheet.selections.erase(item);
				}
			}

			BVH_CONTINUE;
		});

		selections.clear();
	} else if (type == SelectAppend) {
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (auto selection = findSelection(elem.id)) {
				auto flag = elem.getCurrSelectFlags();
				
				auto delta = elem.unselect(selection->second);

				if (!elem.isSelected()) {
					auto item = std::find(sheet.selections.begin(), sheet.selections.end(), iter);
					sheet.selections.erase(item);
				}
			}

			BVH_CONTINUE;
		});
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
		sheet.bvh.query(aabb, [&](decltype(sheet.bvh)::iterator iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (auto selection = findSelection(elem.id)) {
				elem.select(selection->second);
				sheet.selections.emplace_back(iter);
			}

			BVH_CONTINUE;
		});
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

void Command_Select::sortSelections()
{
	std::sort(selections.begin(), selections.end(), [](const auto& a, const auto& b) {
		return a.first < b.first;
	});
}

Command_Select::selection_t* Command_Select::findSelection(int32_t id)
{
	auto iter = std::lower_bound(selections.begin(), selections.end(), id, [](const auto& item, int32_t id) {
		return item.first < id;
	});

	if (iter == selections.end() || iter->first != id) return nullptr;

	return std::addressof(*iter);
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
	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		auto new_elem = elem.clone(sheet.id_counter++);
		
		new_elem->select();
		new_elem->transform(delta, origin, dir);
		new_elem->unselect();

		sheet.bvh.insert(new_elem->getAABB(), std::move(new_elem));
	}
}

void Command_Copy::undo(SchematicSheet& sheet)
{
	auto id = sheet.id_counter -= (uint32_t)sheet.selections.size();

	for (auto& selection : sheet.selections) {
		auto& elem = static_cast<CircuitElement&>(*selection->second);

		auto aabb = elem.getAABB();

		aabb = transform_AABB(aabb, delta, origin, dir);

		auto result = sheet.bvh.query(aabb, [&](auto iter) {
			auto& elem = static_cast<CircuitElement&>(*iter->second);

			if (elem.id != id) BVH_CONTINUE;

			sheet.bvh.erase(iter);
			BVH_BREAK;
		});

		if (result != true)
			int a = 0;

		id++;
	}
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