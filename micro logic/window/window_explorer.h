#pragma once

#include "../gui/docking_window.h"

class SchematicSheet;

class Window_Explorer : public DockingWindow {
public:
	Window_Explorer();

	void showUI();

	SchematicSheet* delete_sheet = nullptr;
};