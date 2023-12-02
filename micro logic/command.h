#pragma once

#include "schematic_sheet.h"

class Command {
public:
	virtual void onPush(SchematicSheet& sheet) {};
	virtual void redo(SchematicSheet& sheet) = 0;
	virtual void undo(SchematicSheet& sheet) = 0;
	virtual std::string what() const = 0;
	virtual bool isModifying() const { return true; }
};

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
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	std::vector<std::unique_ptr<CircuitElement>> elements;
};

class Command_Select : public Command {
public:
	using selection_t  = std::pair<int32_t, uint32_t>;
	using selections_t = std::vector<std::pair<int32_t, uint32_t>>;

	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;
	bool isModifying() const override;

private:
	void sortSelections();
	selection_t* findSelection(int32_t id);

public:
	enum SelectType {   // selections aabb
		Clear,          //     x       x
		SelectAppend,	//     o       o
		SelectInvert,	//     x       o
		Unselect		//     o       o
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
	Direction path;

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
	Direction path;

private:
	size_t item_count;
};

class Command_Cut : public Command {
public:
	void onPush(SchematicSheet& sheet) override;
	void redo(SchematicSheet& sheet) override;
	void undo(SchematicSheet& sheet) override;
	std::string what() const override;

	vec2      delta;
	vec2      origin;
	Direction path;

private:
	size_t item_count;
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