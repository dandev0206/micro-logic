#pragma once

#include "../../include/vk2d/system/cursor.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

VK2D_BEGIN

class CursorImpl {
private:
	static HCURSOR last_cursor;
	static bool    cursor_visible;

public:
	static void setVisible(bool visible) {
		cursor_visible = visible;
		SetCursor(cursor_visible ? last_cursor : NULL);
	}

	static void setCursor(const Cursor& cursor) {
		last_cursor = cursor.impl->cursor;
		SetCursor(cursor_visible ? last_cursor : NULL);
	}

	static void updateCursor() {
		SetCursor(cursor_visible ? last_cursor : NULL);
	}

    CursorImpl() :
        is_system_cursor(false),
        cursor(NULL) {}

    void loadFromPixels(const uint8_t* pixels, const uvec2& size, const uvec2& hotspot) {
		BITMAPV5HEADER bitmapHeader{};

		bitmapHeader.bV5Size        = sizeof(BITMAPV5HEADER);
		bitmapHeader.bV5Width       = static_cast<LONG>(size.x);
		bitmapHeader.bV5Height      = -static_cast<LONG>(size.y); // Negative indicates origin is in upper-left corner
		bitmapHeader.bV5Planes      = 1;
		bitmapHeader.bV5BitCount    = 32;
		bitmapHeader.bV5Compression = BI_BITFIELDS;
		bitmapHeader.bV5RedMask     = 0x00ff0000;
		bitmapHeader.bV5GreenMask   = 0x0000ff00;
		bitmapHeader.bV5BlueMask    = 0x000000ff;
		bitmapHeader.bV5AlphaMask   = 0xff000000;

		uint32_t* bitmapData = NULL;

		HDC screenDC = GetDC(NULL);
		HBITMAP color = CreateDIBSection(
			screenDC,
			reinterpret_cast<const BITMAPINFO*>(&bitmapHeader),
			DIB_RGB_COLORS,
			reinterpret_cast<void**>(&bitmapData),
			NULL,
			0
		);
		ReleaseDC(NULL, screenDC);

		VK2D_ASSERT(color && "Failed to create cursor color bitmap");

		uint32_t* bitmapOffset = bitmapData;
		for (std::size_t remaining = size.x * size.y; remaining > 0; --remaining, pixels += 4) {
			*bitmapOffset++ = static_cast<uint32_t>((pixels[3] << 24) | (pixels[0] << 16) | (pixels[1] << 8) | pixels[2]);
		}

		HBITMAP mask = CreateBitmap(static_cast<int>(size.x), static_cast<int>(size.y), 1, 1, NULL);

		VK2D_ASSERT(mask && "Failed to create cursor mask bitmap");


		ICONINFO cursorInfo;
		std::memset(&cursorInfo, 0, sizeof(ICONINFO));

		cursorInfo.fIcon    = FALSE;
		cursorInfo.xHotspot = hotspot.x;
		cursorInfo.yHotspot = hotspot.y;
		cursorInfo.hbmColor = color;
		cursorInfo.hbmMask  = mask;

		cursor = reinterpret_cast<HCURSOR>(CreateIconIndirect(&cursorInfo));
		is_system_cursor = false;

		DeleteObject(color);
		DeleteObject(mask);

		VK2D_ASSERT(cursor && "Failed to create cursor from bitmaps");
    }

    void loadFromSystem(Cursor::Type type) {
		LPCTSTR shape = nullptr;
		
		switch (type) {
		case Cursor::Arrow:                  shape = IDC_ARROW;       break;
		case Cursor::ArrowWait:              shape = IDC_APPSTARTING; break;
		case Cursor::Wait:                   shape = IDC_WAIT;        break;
		case Cursor::Text:                   shape = IDC_IBEAM;       break;
		case Cursor::Hand:                   shape = IDC_HAND;        break;
		case Cursor::SizeHorizontal:         shape = IDC_SIZEWE;      break;
		case Cursor::SizeVertical:           shape = IDC_SIZENS;      break;
		case Cursor::SizeTopLeftBottomRight: shape = IDC_SIZENWSE;    break;
		case Cursor::SizeBottomLeftTopRight: shape = IDC_SIZENESW;    break;
		case Cursor::SizeLeft:               shape = IDC_SIZEWE;      break;
		case Cursor::SizeRight:              shape = IDC_SIZEWE;      break;
		case Cursor::SizeTop:                shape = IDC_SIZENS;      break;
		case Cursor::SizeBottom:             shape = IDC_SIZENS;      break;
		case Cursor::SizeTopLeft:            shape = IDC_SIZENWSE;    break;
		case Cursor::SizeBottomRight:        shape = IDC_SIZENWSE;    break;
		case Cursor::SizeBottomLeft:         shape = IDC_SIZENESW;    break;
		case Cursor::SizeTopRight:           shape = IDC_SIZENESW;    break;
		case Cursor::SizeAll:                shape = IDC_SIZEALL;     break;
		case Cursor::Cross:                  shape = IDC_CROSS;       break;
		case Cursor::Help:                   shape = IDC_HELP;        break;
		case Cursor::NotAllowed:             shape = IDC_NO;          break;
		default: VK2D_ERROR("unsupported system cursor");
		}

		cursor           = LoadCursor(NULL, shape);
		is_system_cursor = true;

		VK2D_ASSERT(cursor && "Could not create copy of a system cursor");
    }

    void destroy() {
		if (cursor && !is_system_cursor) {
			DestroyCursor(cursor);
			cursor = NULL;
		}
    }

	bool valid() const {
		return cursor;
	}

    bool    is_system_cursor;
    HCURSOR cursor;
};

VK2D_END