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