#include "platform_impl.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include "platform_impl.h"

#pragma comment(lib, "dwmapi.lib")

struct TitlebarWindowData {
	const vk2d::Window*   window;
	const CustomTitleBar* titlebar;
	LONG_PTR              original_proc;
};

struct ResizingLoopData {
	const vk2d::Window* window;
	ResizingLoop*       loop;
	LONG_PTR            original_proc;
};

static std::map<HWND, TitlebarWindowData> titlebar_datas;
static std::map<HWND, ResizingLoopData>   resizing_loop_datas;

static bool is_window_minimized(HWND hwnd) 
{
	WINDOWPLACEMENT placement = { 0 };
	placement.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(hwnd, &placement)) {
		return placement.showCmd == SW_SHOWMINIMIZED;
	}
	return false;
}

static bool is_window_maximized(HWND hwnd)
{
	WINDOWPLACEMENT placement = { 0 };
	placement.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(hwnd, &placement)) {
		return placement.showCmd == SW_SHOWMAXIMIZED;
	}
	return false;
}

static bool composition_enabled() 
{
	BOOL composition_enabled = FALSE;
	bool success = ::DwmIsCompositionEnabled(&composition_enabled) == S_OK;
	return composition_enabled && success;
}

static void redraw_frame(HWND hwnd) 
{
	SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
}

LRESULT CALLBACK titleBarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{	
	auto& td = titlebar_datas[hwnd];

	switch (msg) {
	case WM_NCCALCSIZE: {
		if (!wParam) break;

		if (is_window_maximized(hwnd)) {
			auto monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
			
			if (!monitor) return 0;

			auto* params                = (NCCALCSIZE_PARAMS*)lParam;
			auto& requested_client_rect = *params->rgrc;

			MONITORINFO monitor_info{};
			monitor_info.cbSize = sizeof(monitor_info);
			if (!GetMonitorInfoW(monitor, &monitor_info)) return 0;

			requested_client_rect = monitor_info.rcWork;
			requested_client_rect.bottom -= 1;

			//UINT dpi    = GetDpiForWindow(hwnd);
			//int frame_x = GetSystemMetricsForDpi(SM_CXFRAME, dpi);
			//int frame_y = GetSystemMetricsForDpi(SM_CYFRAME, dpi);
			//int padding = GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);

			//requested_client_rect.right  -= frame_x + padding - 1;
			//requested_client_rect.left   += frame_x + padding - 1;
			//requested_client_rect.top    += frame_y + padding - 1;
			//requested_client_rect.bottom -= frame_y + padding - 1;
		}

		return 0;
	}
	case WM_NCACTIVATE: {
		return 1;
	} break;
	case WM_NCLBUTTONDOWN: {
		if (td.titlebar->getHoveredButton() != TitleButton::None)
			return 0;
	} break;
	case WM_NCLBUTTONUP: {
		auto button = (int32_t)td.titlebar->getHoveredButton();

		if (button & (int32_t)TitleButton::Minimize) {
			ShowWindow(hwnd, SW_MINIMIZE);
			return 0;
		} else if (button & (int32_t)TitleButton::Maximize) {
			int mode = is_window_maximized(hwnd) ? SW_NORMAL : SW_MAXIMIZE;
			ShowWindow(hwnd, mode);
			return 0;
		} else if (button & (int32_t)TitleButton::Close) {
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
		Rect window_rect = { {}, (vec2)td.window->getSize() };
		Rect border_rect = window_rect;

		border_rect.width  -= 14;
		border_rect.height -= 7;
		border_rect.left   += 7;

		if (window_rect.contain(pos) && !border_rect.contain(pos)) {
			bool bottom = border_rect.height <= pos.y && pos.y <= window_rect.height;
			bool left   = 0 <= pos.x && pos.x < border_rect.left;
			bool right  = border_rect.left + border_rect.width < pos.x && pos.x <= window_rect.width;

			if (left && bottom)   return HTBOTTOMLEFT;
			if (left && !bottom)  return HTLEFT;
			if (right && bottom)  return HTBOTTOMRIGHT;
			if (right && !bottom) return HTRIGHT;
			if (bottom)           return HTBOTTOM;
		}

		auto hovered_button = (int32_t)td.titlebar->getHoveredButton();
		//if (hovered_button & (int32_t)TitleButton::Minimize)
		//	return HTMINBUTTON;
		if (hovered_button & (int32_t)TitleButton::Maximize)
			return HTMAXBUTTON;
		//if (hovered_button & (int32_t)TitleButton::Close)
		//	return HTCLOSE;

		if (td.titlebar->getCaptionRect().contain(pos)) 
			return HTCAPTION;

		return HTCLIENT;
	}
	case WM_SIZE: {
		RECT rect;
		HRGN hrgn;

		if (is_window_maximized(hwnd)) {
			hrgn = NULL;
		} else {
			GetClientRect(hwnd, &rect);

			hrgn = CreateRoundRectRgn(rect.left, rect.top, rect.right + 2, rect.bottom + 2, 20, 20);
		}
		SetWindowRgn(hwnd, hrgn, false);
	} break;
	}

	return CallWindowProc(reinterpret_cast<WNDPROC>(td.original_proc), hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK resizingProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	auto& rld = resizing_loop_datas[hwnd];

	switch (msg) {
	case WM_NCHITTEST: {
		if (rld.loop->resize_tab_hovered)
			return HTBOTTOMRIGHT;
	} break;
	case WM_SIZE: {
		if (!wParam && is_window_maximized(hwnd)) return 0;
	} break;
	case WM_SIZING: {
		auto& rect = *reinterpret_cast<RECT*>(lParam);

		uvec2 size(rect.right - rect.left, rect.bottom - rect.top);

		vk2d::Event e{ vk2d::Event::Resize };
		e.resize.size = size;

		auto dt = rld.loop->delta_time;

		SetWindowPos(hwnd, NULL, 0, 0, size.x, size.y, SWP_NOMOVE | SWP_NOZORDER);
		rld.loop->eventProc(e);
		rld.loop->loop();
		return TRUE;
	} break;
	}
	return CallWindowProc(reinterpret_cast<WNDPROC>(rld.original_proc), hwnd, msg, wParam, lParam);
}

void CreateNewProcess()
{
	STARTUPINFOA si{};
	PROCESS_INFORMATION pi{};

	si.cb = sizeof(si);

	CreateProcessA(
		__argv[0],
		NULL,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&si,
		&pi
	);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

void InjectTitleBar(CustomTitleBar* titlebar)
{
	auto& window = titlebar->getWindow();
	auto hwnd    = (HWND)window.getNativeHandle();

	auto& td = titlebar_datas[hwnd];

	td.window       = &window;
	td.titlebar     = titlebar;
	td.original_proc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);

	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)titleBarProc);

	MARGINS shadow_rect{ 0, 0, 1, 0 };
	DwmExtendFrameIntoClientArea(hwnd, &shadow_rect);
	
	LONG style = GetWindowLong(hwnd, GWL_STYLE);

	style |=
		WS_SYSMENU
		| WS_MAXIMIZEBOX
		| WS_MINIMIZEBOX
		| WS_CAPTION;

	SetWindowLong(hwnd, GWL_STYLE, style);

	redraw_frame(hwnd);
}

void InjectResizingLoop(const vk2d::Window& window, ResizingLoop* resizing_loop)
{
	HWND hwnd = (HWND)window.getNativeHandle();

	auto& rld = resizing_loop_datas[hwnd];

	rld.window        = &window;
	rld.loop          = resizing_loop;
	rld.original_proc = GetWindowLongPtr(hwnd, GWLP_WNDPROC);

	SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)resizingProc);

	redraw_frame(hwnd);
}