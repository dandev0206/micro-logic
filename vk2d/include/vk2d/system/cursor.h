#pragma once

#include "../vector_type.h"

VK2D_BEGIN

class CursorImpl;
class Window;

class Cursor {
	friend class CursorImpl;

public:
	enum Type
	{
		Arrow,                  // Arrow cursor (default)
		ArrowWait,              // Busy arrow cursor
		Wait,                   // Busy cursor
		Text,                   // I-beam, cursor when hovering over a field allowing text entry
		Hand,                   // Pointing hand cursor
		SizeHorizontal,         // Horizontal double arrow cursor
		SizeVertical,           // Vertical double arrow cursor
		SizeTopLeftBottomRight, // Double arrow cursor going from top-left to bottom-right
		SizeBottomLeftTopRight, // Double arrow cursor going from bottom-left to top-right
		SizeLeft,               // Left arrow cursor on Linux, same as SizeHorizontal on other platforms
		SizeRight,              // Right arrow cursor on Linux, same as SizeHorizontal on other platforms
		SizeTop,                // Up arrow cursor on Linux, same as SizeVertical on other platforms
		SizeBottom,             // Down arrow cursor on Linux, same as SizeVertical on other platforms
		SizeTopLeft,            // Top-left arrow cursor on Linux, same as SizeTopLeftBottomRight on other platforms
		SizeBottomRight,        // Bottom-right arrow cursor on Linux, same as SizeTopLeftBottomRight on other platforms
		SizeBottomLeft,         // Bottom-left arrow cursor on Linux, same as SizeBottomLeftTopRight on other platforms
		SizeTopRight,           // Top-right arrow cursor on Linux, same as SizeBottomLeftTopRight on other platforms
		SizeAll,                // Combination of SizeHorizontal and SizeVertical
		Cross,                  // Crosshair cursor
		Help,                   // Help cursor
		NotAllowed              // Action not allowed cursor
	};

public:
	static void setVisible(bool visible);
	static void setCursor(const Cursor& cursor);
	static void updateCursor();

public:
	Cursor();
	~Cursor();

	void loadFromPixels(const uint8_t* pixels, const uvec2& size, const uvec2& hotspot);
	void loadFromSystem(Type type);

	void destroy();

	bool valid() const;

private:
	CursorImpl* impl;
};

VK2D_END