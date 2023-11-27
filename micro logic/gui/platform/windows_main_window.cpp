#include "windows_main_window.h"

#include "../../vector_type.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

static MainWindow* window    = nullptr;
static LONG_PTR originalProc = NULL;


static bool win32_window_is_maximized(HWND hwnd) 
{
	WINDOWPLACEMENT placement = { 0 };
	placement.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(hwnd, &placement)) {
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}
	return false;
}

LRESULT CALLBACK newProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_NCCALCSIZE: {
		if (!wParam) return DefWindowProc(hwnd, msg, wParam, lParam);

		if (win32_window_is_maximized(hwnd)) {
			UINT dpi = GetDpiForWindow(hwnd);
			int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
			int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
			int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

			NCCALCSIZE_PARAMS* params = (NCCALCSIZE_PARAMS*)lParam;
			RECT* requested_client_rect = params->rgrc;

			requested_client_rect->right  -= frame_x + padding - 2;
			requested_client_rect->left   += frame_x + padding - 2;
			requested_client_rect->top    += frame_y + padding - 2;
			requested_client_rect->bottom -= frame_y + padding - 2;
		}
		
		return 0;
	}
	case WM_NCACTIVATE: {
		return 1;
	}
	case WM_NCLBUTTONDOWN: {
		if (window->hovered_title_button != TitleButton::None) {
			return 0;
		}
	} break;
	case WM_NCLBUTTONUP: {
		if (wParam == HTMINBUTTON) {
			ShowWindow(hwnd, SW_MINIMIZE);
			return 0;
		} else if (wParam == HTMAXBUTTON) {
			int mode = win32_window_is_maximized(hwnd) ? SW_NORMAL : SW_MAXIMIZE;
			ShowWindow(hwnd, mode);
			return 0;
		} else if (wParam == HTCLOSE) {
			PostMessageW(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
	} break;
	case WM_NCHITTEST: {
		POINT p;
		p.x = GET_X_LPARAM(lParam);
		p.y = GET_Y_LPARAM(lParam);
		ScreenToClient(hwnd, &p);

		vec2 pos(p.x, p.y);

		vk2d::Rect rect = { {}, (vec2)window->window.getSize() };
		vk2d::Rect frame_rect = rect;
		frame_rect.width -= 14;
		frame_rect.height -= 7;
		frame_rect.left += 7;

		if (rect.contain(pos) && !frame_rect.contain(pos)) {
			bool bottom = frame_rect.height <= pos.y && pos.y <= rect.height;
			bool left   = 0 <= pos.x && pos.x < frame_rect.left;
			bool right = frame_rect.left + frame_rect.width < pos.x && pos.x <= rect.width;

			if (left && bottom)   return HTBOTTOMLEFT;
			if (left && !bottom)  return HTLEFT;
			if (right && bottom)  return HTBOTTOMRIGHT;
			if (right && !bottom) return HTRIGHT;
			if (bottom)           return HTBOTTOM;
		}

		if (window->hovered_title_button == TitleButton::Minimize) 
			return HTMINBUTTON;
		if (window->hovered_title_button == TitleButton::Maximize) 
			return HTMAXBUTTON;
		if (window->hovered_title_button == TitleButton::Close)    
			return HTCLOSE;

		float width = (float)window->window.getSize().x;
		vk2d::Rect caption_rect{ 370.f, 0.f, width - 535.f, 43.f };
		if (caption_rect.contain(pos)) return HTCAPTION;

		return HTCLIENT;
	}
	case WM_SIZE: {
		if (!wParam && win32_window_is_maximized(hwnd)) return 0;
		break;
	}
	case WM_SIZING: {
		if (!window->initialized) break;

		auto& rect = *reinterpret_cast<RECT*>(lParam);
		
		uvec2 size(rect.right - rect.left, rect.bottom - rect.top);

		vk2d::Event e{ vk2d::Event::Resize };
		e.resize.size = size;

		auto dt = window->getDeltaTime();

		SetWindowPos(hwnd, NULL, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER);
		window->eventProc(e, dt);
		window->loop(dt);

	return TRUE;
	} break;
	case WM_ERASEBKGND: {
		return 0;
	} break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(originalProc), hwnd, msg, wParam, lParam);
}

void InjectWndProc(MainWindow* main_window)
{
	window = main_window;

	HWND hwnd = (HWND)window->window.getNativeHandle();

	originalProc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)newProc);

	int window_style = 
		WS_THICKFRAME
		| WS_SYSMENU
		| WS_POPUP
		| WS_MAXIMIZEBOX
		| WS_MINIMIZEBOX
		| WS_VISIBLE;

	SetWindowLong(hwnd, GWL_STYLE, window_style);

	MARGINS shadow_rect{ 1, 1, 0, 1 };
	DwmExtendFrameIntoClientArea(hwnd, &shadow_rect);

	// redraw frame
	SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hwnd, SW_SHOW);
}
