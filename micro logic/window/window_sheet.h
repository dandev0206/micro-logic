#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/draw_list.h>

#include "../gui/docking_window.h"
#include "../schematic_sheet.h"
#include "../command.h"

enum class GridStyle {
	line,
	dot
};

class Window_Sheet : public DockingWindow {
public:
	using bvh_iterator_t = typename BVH<std::unique_ptr<CircuitElement>>::iterator;
	using CommandStack_t = std::vector<std::unique_ptr<Command>>;

	Window_Sheet();
	Window_Sheet(SchematicSheet& sheet);
	Window_Sheet(Window_Sheet&&) = default;
	~Window_Sheet();

	Window_Sheet& operator=(Window_Sheet&& rhs) noexcept = default;

	void showUI();
	void EventProc(const vk2d::Event& e, float dt) override;

	bool isCommandInSavedRange(int64_t cmd_idx) const;
	void sheetSaved();
	void SheetRenamed();

	void bindSchematicSheet(SchematicSheet& sheet);

	void copySelectedToClipboard(bool is_copy = true);
	void cutSelectedToClipboard();
	void pasteFromClipboard();

	void addToHoverList(bvh_iterator_t iter, const AABB& aabb);
	void clearHoverList();

	void pushCommand(std::unique_ptr<Command>&& cmd, bool skip_redo = false);
	void setCurrCommandTo(int64_t next_cmd);
	void redo();
	void undo();
	bool isRedoable() const;
	bool isUndoable() const;
	void clearCommand();

	void deleteElement(const AABB& aabb);

public: // drawing
	void draw();
	void showGrid();
	void showBVH();

	void showDragRect(vk2d::Mouse::Button button);

public:
	void setPosition(const vec2& pos);
	void setScale(float scale);
	void setScaleTo(float scale, vec2 center);

	vec2 toScreen(const vec2& pos) const;
	vec2 toPlane(const vec2& pos) const;
	Rect toPlane(const Rect& rect) const;

	Rect getDragRect(vk2d::Mouse::Button button);

	vec2 getCursorPlanePos() const;
	vec2 getClampedCursorPlanePos() const;
	vec2 getClampedCursorPos() const;

	BVH<std::unique_ptr<CircuitElement>>& getBVH();

public:
	std::string     window_name;
	SchematicSheet* sheet;

	vk2d::DrawList draw_list;
	CommandStack_t command_stack;
	int64_t        curr_command;
	int64_t        last_saved_command_min;
	int64_t        last_saved_command_max;

	std::vector<bvh_iterator_t> hover_list;

	vec2  content_center;
	vec2  prev_position;
	float prev_scale;

	bool capturing_mouse;
	bool update_grid;
};