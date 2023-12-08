#pragma once

#include "command.h"

class CommandGroup : public Command {
public:
	CommandGroup();

	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;
	bool isModifying() const override;

	void append(std::unique_ptr<Command>&& cmd);
	bool empty() const;

	std::vector<std::unique_ptr<Command>> commands;
	std::string                           description;
	bool                                  modifying;
};

class Command_Add : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	std::vector<std::unique_ptr<CircuitElement>> elements;

private:
	using bvh_iterator_t = typename BVH<std::unique_ptr<CircuitElement>>::iterator;

	std::vector<bvh_iterator_t> refs;
	size_t                      item_count;
};

class Command_Select : public Command {
public:
	using selection_t  = std::pair<CircuitElement*, uint32_t>;
	using selections_t = std::vector<selection_t>;

	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;
	bool isModifying() const override;

public:
	enum SelectType {   // selections aabb
		Clear,          //     x       x
		SelectAll,      //     x       x
		SelectAppend,	//     o       x
		SelectInvert,	//     x       o
		Unselect		//     o       x
	};

	SelectType   type;
	selections_t selections;
	AABB         aabb;
};

class Command_Move : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	vec2      delta;
	vec2      origin;
	Direction dir;

private:
	size_t item_count;
};

class Command_Copy : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	vec2      delta;
	vec2      origin;
	Direction dir;

private:
	using bvh_iterator_t = typename BVH<std::unique_ptr<CircuitElement>>::iterator;

	std::vector<std::unique_ptr<CircuitElement>> elements;
	std::vector<bvh_iterator_t>                  refs;
	size_t                                       item_count;
};

class Command_Cut : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	vec2      delta;
	vec2      origin;
	Direction dir;

private:
	std::vector<std::unique_ptr<CircuitElement>> elements;
	size_t                                       item_count;
};

class Command_Delete : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

private:
	std::vector<std::unique_ptr<CircuitElement>> elements;
	size_t item_count;
};