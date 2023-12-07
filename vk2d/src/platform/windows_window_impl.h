#include "window_impl.h"

#include "../../include/vk2d/core/vk2d_context_impl.h"
#include "../../include/vk2d/system/cursor.h"
#include <windowsx.h>

#define WNDCLASSNAME "vk2d_window_class"

#define RECT_WIDTH(rect) (rect.right - rect.left)
#define RECT_HEIGHT(rect) (rect.bottom - rect.top)
#define RECT_POS(rect) rect.left, rect.top
#define RECT_SIZE(rect) RECT_WIDTH(rect), RECT_HEIGHT(rect)

static uint32_t window_counter = 0;

VK2D_BEGIN

static void set_process_dpi_aware()
{
	HINSTANCE shCoreDll = LoadLibrary("Shcore.dll");

	if (shCoreDll) {
		enum ProcessDpiAwareness {
			ProcessDpiUnaware         = 0,
			ProcessSystemDpiAware     = 1,
			ProcessPerMonitorDpiAware = 2
		};

		typedef HRESULT(WINAPI* SetProcessDpiAwarenessFuncType)(ProcessDpiAwareness);
		SetProcessDpiAwarenessFuncType SetProcessDpiAwarenessFunc = reinterpret_cast<SetProcessDpiAwarenessFuncType>(reinterpret_cast<void*>(GetProcAddress(shCoreDll, "SetProcessDpiAwareness")));

		if (SetProcessDpiAwarenessFunc) {
			if (SetProcessDpiAwarenessFunc(ProcessPerMonitorDpiAware) == E_INVALIDARG) {
				VK2D_ERROR("Failed to set process DPI awareness");
			} else {
				FreeLibrary(shCoreDll);
				return;
			}
		}

		FreeLibrary(shCoreDll);
	}

	HINSTANCE user32Dll = LoadLibrary("user32.dll");

	if (user32Dll) {
		typedef BOOL(WINAPI* SetProcessDPIAwareFuncType)(void);
		SetProcessDPIAwareFuncType SetProcessDPIAwareFunc = reinterpret_cast<SetProcessDPIAwareFuncType>(reinterpret_cast<void*>(GetProcAddress(user32Dll, "SetProcessDPIAware")));

		if (SetProcessDPIAwareFunc) {
			if (!SetProcessDPIAwareFunc())
				VK2D_ERROR("Failed to set process DPI awareness");
		}

		FreeLibrary(user32Dll);
	}
}

static DWORD create_style(WindowStyleFlags style)
{
	DWORD dw_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP;
	
	if (style & Window::Style::Resizable)
		dw_style |= WS_THICKFRAME;

	if (style & Window::Style::Minimize)
		dw_style |= WS_CAPTION | WS_MINIMIZEBOX;

	if (style & Window::Style::Maximize)
		dw_style |= WS_CAPTION | WS_MAXIMIZEBOX;
	
	if (style & Window::Style::Close)
		dw_style |= WS_CAPTION | WS_SYSMENU;

	return dw_style;
}

static DWORD create_ex_style(WindowStyleFlags style)
{
	DWORD dw_ex_style = WS_EX_APPWINDOW;

	if (style & Window::Style::TopMost)
		dw_ex_style |= WS_EX_TOPMOST;

	return dw_ex_style;
}

static iRect get_window_rect(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);
	return { RECT_POS(rect), RECT_SIZE(rect)};
}

static iRect get_client_rect(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	return { RECT_POS(rect), RECT_SIZE(rect) };
}

static void setTracking(HWND hwnd, bool track)
{
	TRACKMOUSEEVENT mouseEvent;
	mouseEvent.cbSize      = sizeof(TRACKMOUSEEVENT);
	mouseEvent.dwFlags     = track ? TME_LEAVE : TME_CANCEL;
	mouseEvent.hwndTrack   = hwnd;
	mouseEvent.dwHoverTime = HOVER_DEFAULT;
	TrackMouseEvent(&mouseEvent);
}

static Key virtual_key_code_to_key(WPARAM key, LPARAM flags)
{
	switch (key) {
	case VK_SHIFT: {
		static UINT lShift = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
		UINT scancode = static_cast<UINT>((flags & (0xFF << 16)) >> 16);
		return scancode == lShift ? Key::LShift : Key::RShift;
	}
	case VK_MENU: return (GET_Y_LPARAM(flags) & KF_EXTENDED) ? Key::RAlt : Key::LAlt;
	case VK_CONTROL: return (GET_Y_LPARAM(flags) & KF_EXTENDED) ? Key::RControl : Key::LControl;
	default: return Keyboard::toKey(key);
	}
}

static uint16_t* decode(uint16_t* begin, uint16_t* end, uint32_t& output, uint32_t replacement = 0)
{
	uint16_t first = *begin++;

	if ((first >= 0xd800) && (first <= 0xdbff)) {
		if (begin < end) {
			uint32_t second = *begin++;
			if ((second >= 0xdc00) && (second <= 0xdfff)) {
				output = ((first - 0xd800u) << 10) + (second - 0xdc00) + 0x0010000;
			} else {
				output = replacement;
			}
		} else {
			begin = end;
			output = replacement;
		}
	} else {
		output = first;
	}

	return begin;
}

static uint32_t* to_utf32(uint16_t* begin, uint16_t* end, uint32_t* output)
{
	while (begin < end)
	{
		uint32_t codepoint;
		begin = decode(begin, end, codepoint);
		*output++ = codepoint;
	}

	return output;
}

VK2D_PRIV_BEGIN

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void registerWindowClass() {
	WNDCLASSEX wc;
	wc.cbSize        = sizeof(WNDCLASSEX);
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = GetModuleHandle(NULL);
	wc.hIcon         = nullptr;
	wc.hCursor       = nullptr;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = WNDCLASSNAME;
	wc.hIconSm       = nullptr;

	auto res = RegisterClassEx(&wc);
	if (!res) VK2D_ERROR("error while creating window class");
}

class WindowImpl : public WindowImplBase {
public:
	WindowImpl(uint32_t width, uint32_t height, const char* title, const WindowImpl* parent, WindowStyleFlags style) :
		WindowImplBase(),
		hwnd(nullptr),
		is_mouse_inside(false),
		is_resizing(false),
		surrogate(0),
		key_repeat_enabled(true) {

		if (window_counter++ == 0) {
			registerWindowClass();
			set_process_dpi_aware();
		}

		DWORD dw_style    = create_style(style);
		DWORD dw_ex_style = create_ex_style(style);

		HDC screenDC = GetDC(NULL);
		int32_t left = (GetDeviceCaps(screenDC, HORZRES) - (int32_t)(width)) / 2;
		int32_t top  = (GetDeviceCaps(screenDC, VERTRES) - (int32_t)(height)) / 2;
		ReleaseDC(NULL, screenDC);

		RECT rect = { 0, 0, (LONG)width, (LONG)height };
		AdjustWindowRectEx(&rect, dw_style, false, dw_ex_style);

		hwnd = CreateWindowEx(
			dw_ex_style,
			WNDCLASSNAME,
			title,
			dw_style,
			left,
			top,
			RECT_WIDTH(rect),
			RECT_HEIGHT(rect),
			parent ? parent->hwnd : NULL,
			NULL,
			GetModuleHandle(NULL),
			this);

		if (style & Window::Style::Visible)
			ShowWindow(hwnd, SW_SHOW);

		auto window_rect = get_window_rect(hwnd);

		position    = window_rect.getPosition();
		size        = window_rect.getSize();
		fb_size     = { width, height };
		this->title = title;

		{
			auto& inst = VK2DContext::get();

			vk::Win32SurfaceCreateInfoKHR info(
				{},
				GetModuleHandle(NULL),
				hwnd);

			surface = inst.instance.createWin32SurfaceKHR(info);
			WindowImplBase::init();
		}
	}

	~WindowImpl() {
		DestroyWindow(hwnd);
		WindowImplBase::destroy();

		if (--window_counter == 0)
			UnregisterClass(WNDCLASSNAME, GetModuleHandle(NULL));
	}

	void processEvent(bool block) {
		MSG msg;
		if (block) {
			while (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	bool pollEvent(Event& event) {
		processEvent(false);

		if (events.empty()) return false;

		event = events.front();
		events.pop();
		return true;
	}

	bool waitEvent(Event& event) {
		if (!events.empty()) {
			event = events.front();
			events.pop();
			return true;
		}

		do {
			processEvent(true);
		} while (events.empty());

		event = events.front();
		events.pop();
		return true;
	}

	void setPosition(const glm::ivec2& pos) {
		if (position != pos) {
			position = pos;
			SetWindowPos(hwnd, nullptr, pos.x, pos.y, 0, 0, SWP_NOSIZE);
		}
	}

	void setSize(const glm::uvec2& size) {
		if (this->size != size) {

			SetWindowPos(hwnd, nullptr, 0, 0, size.x, size.y, SWP_NOMOVE);
			update_swapchain = true;
		}
	}

	void setFraneBufferSize(const glm::uvec2& size) {
		if (this->fb_size != size) {
			RECT rect{ 0, 0, (LONG)size.x, (LONG)size.y };
			
			auto style    = GetWindowLong(hwnd, GWL_STYLE);
			auto ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);

			AdjustWindowRectEx(&rect, style, false, ex_style);
			SetWindowPos(hwnd, nullptr, 0, 0, RECT_SIZE(rect), SWP_NOMOVE);
			update_swapchain = true;
		}
	}

	void setMinSizeLimit(const uvec2& size) {
		size_limit_min = size;
	}

	void setMaxSizeLimit(const uvec2& size) {
		size_limit_max = size;
	}

	void setTransparency(float value) {
		value = std::clamp(value, 0.f, 1.f);
		
		if (value == 0.f && transparency != 0.f) {
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);
		} else if (value != 0.f && transparency == 0.f) {
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		}

		SetLayeredWindowAttributes(hwnd, 0, (BYTE)(255.99f * value), LWA_ALPHA);
	}

	void setVisible(bool value) {
		if (visible != value) {
			visible = value;

			if (visible)
				ShowWindow(hwnd, SW_SHOW);
			else
				ShowWindow(hwnd, SW_HIDE);
		}
	}

	bool isResizable() {
		return GetWindowLong(hwnd, GWL_STYLE) & WS_THICKFRAME;
	}

	void setResizable(bool value) {
		auto style = GetWindowLong(hwnd, GWL_STYLE);

		if ((bool)(style & WS_THICKFRAME) != value) {
			if (value) {
				style |= WS_THICKFRAME;
			} else {
				style &= ~WS_THICKFRAME;
			}
			SetWindowLong(hwnd, GWL_STYLE, style);
		}
	}

	void setParent(WindowImpl* impl) {
		if (impl)
			SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)impl->hwnd);
		else
			SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, NULL);
	}

	bool isDisabled() const {
		return !IsWindowEnabled(hwnd);
	}

	void setDisabled(bool value) {
		EnableWindow(hwnd, !value);
	}

	void setTitle(const char* title) {
		this->title = title;
		SetWindowText(hwnd, title);
	}

	void minimize() {
		ShowWindow(hwnd, SW_MINIMIZE);
	}

	void maximize() {
		ShowWindow(hwnd, SW_MAXIMIZE);
	}

	void setFocus() {
		SetFocus(hwnd);
	}

	void restore() {
		ShowWindow(hwnd, SW_RESTORE);
	}

public:
	HWND hwnd;
	bool is_mouse_inside;
	bool is_resizing;
	uint16_t surrogate;
	bool key_repeat_enabled;
};

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	auto* window = reinterpret_cast<WindowImpl*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	switch (msg) {
	case WM_CREATE: {
		LONG_PTR window_ptr = (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams;
		SetWindowLongPtr(hwnd, GWLP_USERDATA, window_ptr);
	} break;
	case WM_CLOSE: {
		Event e{ Event::Close };
		window->events.push(e);
		return 0;
	}
	case WM_SIZE: {
		uvec2 size    = get_window_rect(hwnd).getSize();
		uvec2 fb_size = get_client_rect(hwnd).getSize();

		if (wParam == SIZE_MINIMIZED) {
			window->minimized = true;
			window->maximized = false;
		} else if (wParam == SIZE_MAXIMIZED) {
			window->minimized = false;
			window->maximized = true;
		} else if (wParam == SIZE_RESTORED) {
			window->minimized = false;
			window->maximized = false;
		}

		if (window->size != size || window->fb_size != fb_size) {
			window->size             = size;
			window->fb_size          = fb_size;
			window->update_swapchain = true;

			if (wParam == SIZE_MINIMIZED || window->is_resizing) break;

			Event e{ Event::Resize };
			e.resize.size    = window->size;
			e.resize.fb_size = window->fb_size;
			window->events.push(e);

		}
	} break;
	case WM_MOVE: {
		window->position = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

		Event e{ Event::Move };
		e.move.pos = window->position;
		window->events.push(e);
	} break;
	case WM_ENTERSIZEMOVE: {
		window->is_resizing = true;
	} break;
	case WM_EXITSIZEMOVE: {
		window->is_resizing = false;

		uvec2 size    = get_window_rect(hwnd).getSize();
		uvec2 fb_size = get_client_rect(hwnd).getSize();

		if (window->size != size || window->fb_size != fb_size) {
			window->size    = size;
			window->fb_size = fb_size;

			Event e{ Event::Resize };
			e.resize.size    = window->size;
			e.resize.fb_size = window->fb_size;
			window->events.push(e);

			window->update_swapchain = true;
		}
	} break;
	case WM_GETMINMAXINFO: {
		if (!window) break;

		LPMINMAXINFO info = (LPMINMAXINFO)lParam;
		
		if (window->size_limit_min.x)
			info->ptMinTrackSize.x = (LONG)window->size_limit_min.x;
		if (window->size_limit_min.y)
			info->ptMinTrackSize.y = (LONG)window->size_limit_min.y;
		if (window->size_limit_max.x)
			info->ptMaxTrackSize.x = (LONG)window->size_limit_max.x;
		if (window->size_limit_max.y)
			info->ptMaxTrackSize.y = (LONG)window->size_limit_max.y;
		return 0;
	} break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN: {
		Event e{ Event::KeyPressed };
		e.keyboard.key  = virtual_key_code_to_key(wParam, lParam);
		e.keyboard.alt  = GET_Y_LPARAM(GetKeyState(VK_MENU)) != 0;
		e.keyboard.ctrl = GET_Y_LPARAM(GetKeyState(VK_CONTROL)) != 0;
		e.keyboard.shft = GET_Y_LPARAM(GetKeyState(VK_SHIFT)) != 0;
		e.keyboard.sys  = GET_Y_LPARAM(GetKeyState(VK_LWIN)) || GET_Y_LPARAM(GetKeyState(VK_RWIN));
		window->events.push(e);
		break;
	}
	case WM_KEYUP: 
	case WM_SYSKEYUP: {
		Event e{ Event::KeyReleased };
		e.keyboard.key  = virtual_key_code_to_key(wParam, lParam);
		e.keyboard.alt  = GET_Y_LPARAM(GetKeyState(VK_MENU)) != 0;
		e.keyboard.ctrl = GET_Y_LPARAM(GetKeyState(VK_CONTROL)) != 0;
		e.keyboard.shft = GET_Y_LPARAM(GetKeyState(VK_SHIFT)) != 0;
		e.keyboard.sys  = GET_Y_LPARAM(GetKeyState(VK_LWIN)) || GET_Y_LPARAM(GetKeyState(VK_RWIN));
		window->events.push(e);
		break;
	}
	case WM_CHAR: {
		if (window->key_repeat_enabled || ((lParam & (1 << 30)) == 0)) {
			uint32_t character = static_cast<uint32_t>(wParam);

			if ((character >= 0xd800) && (character <= 0xdbff)) {
				window->surrogate = static_cast<uint16_t>(character);
			} else {
				if ((character >= 0xdc00) && (character <= 0xdfff)) {
					uint16_t utf16[] = { window->surrogate, static_cast<uint16_t>(character) };
					to_utf32(utf16, utf16 + 2, &character);
					window->surrogate = 0;
				}

				Event e{ Event::TextEntered };
				e.text.unicode = character;
				window->events.push(e);
			}
		}
	} break;
	case WM_MOUSEWHEEL: {
		POINT position;
		position.x = (int32_t)GET_X_LPARAM(lParam);
		position.y = (int32_t)GET_Y_LPARAM(lParam);
		ScreenToClient(hwnd, &position);

		Event e{ Event::WheelScrolled };
		e.wheel.delta = { 0.f, (float)GET_Y_LPARAM(wParam) / 120.f };
		e.wheel.pos   = { position.x, position.y };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_MOUSEHWHEEL: {
		POINT position;
		position.x = (int32_t)GET_X_LPARAM(lParam);
		position.y = (int32_t)GET_Y_LPARAM(lParam);
		ScreenToClient(hwnd, &position);

		Event e{ Event::WheelScrolled };
		e.wheel.delta = { (float)GET_Y_LPARAM(wParam) / 120.f, 0.f };
		e.wheel.pos = { position.x, position.y };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_LBUTTONDOWN: {
		Event e{ Event::MousePressed };
		e.mouseButton.button = Mouse::Left;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_LBUTTONUP: {
		Event e{ Event::MouseReleased };
		e.mouseButton.button = Mouse::Left;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
	} break;
	case WM_RBUTTONDOWN: {
		Event e{ Event::MousePressed };
		e.mouseButton.button = Mouse::Right;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_RBUTTONUP: {
		Event e{ Event::MouseReleased };
		e.mouseButton.button = Mouse::Right;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
	} break;
	case WM_MBUTTONDOWN: {
		Event e{ Event::MousePressed };
		e.mouseButton.button = Mouse::Middle;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_MBUTTONUP: {
		Event e{ Event::MouseReleased };
		e.mouseButton.button = Mouse::Middle;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
	} break;
	case WM_XBUTTONDOWN: {
		Event e{ Event::MousePressed };
		e.mouseButton.button = GET_Y_LPARAM(wParam) == XBUTTON1 ? Mouse::XButton1 : Mouse::XButton2;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
		SetFocus(hwnd);
	} break;
	case WM_XBUTTONUP: {
		Event e{ Event::MouseReleased };
		e.mouseButton.button = GET_Y_LPARAM(wParam) == XBUTTON1 ? Mouse::XButton1 : Mouse::XButton2;
		e.mouseButton.pos    = { (int32_t)GET_X_LPARAM(lParam), (int32_t)GET_Y_LPARAM(lParam) };
		window->events.push(e);
	} break;
	case WM_MOUSELEAVE: {
		if (window->is_mouse_inside) {
			window->is_mouse_inside = false;

			Event e{ Event::MouseLeft };
			window->events.push(e);
		}
	} break;
	case WM_MOUSEMOVE: {
		int32_t x = static_cast<int32_t>(GET_X_LPARAM(lParam));
		int32_t y = static_cast<int32_t>(GET_Y_LPARAM(lParam));
		ivec2 pos = { x, y };

		RECT area;
		GetClientRect(hwnd, &area);

		if ((wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0) {
			if (GetCapture() == hwnd)
				ReleaseCapture();
		} else if (GetCapture() != hwnd) {
			SetCapture(hwnd);
		}

		if ((x < area.left) || (x > area.right) || (y < area.top) || (y > area.bottom)) {
			if (window->is_mouse_inside) {
				window->is_mouse_inside = false;

				setTracking(hwnd, false);

				Event e{ Event::MouseLeft };
				window->events.push(e);
			}
		} else {
			if (!window->is_mouse_inside) {
				window->is_mouse_inside = true;

				setTracking(hwnd, true);

				window->mouse_prev = pos;

				Event e{ Event::MouseEntered };
				window->events.push(e);
			}
		}

		Event e{ Event::MouseMoved };
		e.mouseMove.pos   = pos;
		e.mouseMove.delta = pos - window->mouse_prev;
		window->events.push(e);

		window->mouse_prev = pos;
	} break;
	case WM_SETFOCUS: {
		Event e{ Event::Focussed };
		window->events.push(e);
		window->focussed = true;
	} return 0;
	case WM_KILLFOCUS: {
		Event e{ Event::LostFocus };
		window->events.push(e);
		window->focussed = false;
	} return 0;
	case WM_SETCURSOR: {
		if (LOWORD(lParam) == HTCLIENT)
			Cursor::updateCursor();
	} break;
	case WM_ERASEBKGND: {
		return 1;
	}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

VK2D_PRIV_END
VK2D_END