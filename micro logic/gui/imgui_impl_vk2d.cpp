#include "imgui_impl_vk2d.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <vk2d/core/vk2d_context_impl.h>
#include <vk2d/system/clipboard.h>
#include <vk2d/system/cursor.h>
#include <vk2d/system/keyboard.h>
#include <vk2d/system/mouse.h>
#include <vk2d/graphics/vertex_buffer.h>
#include <vk2d/graphics/buffer.h>
#include <vector>
#include <map>

#ifdef VK2D_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

IMGUI_VK2D_BEGIN

struct WindowData {
	WindowData(EventProc event_proc, void* event_user_data) :
		event_proc(event_proc),
		event_user_data(event_user_data) {}

	EventProc event_proc;
	void*     event_user_data;
};

struct BackendData {
	BackendData() : 
		context(nullptr),
		window(nullptr),
		want_update_monitors(true),
		last_cursor(ImGuiMouseCursor_None) {}

	ImGuiContext* context;
	vk2d::Window* window;

	vk2d::Pipeline pipeline;
	vk2d::Texture  font_texture;

	vk2d::Cursor     cursors[ImGuiMouseCursor_COUNT];
	ImGuiMouseCursor last_cursor;

	bool want_update_monitors;
};

struct ViewportWindowData {
	ViewportWindowData() :
		window(nullptr),
		window_owned(false) {}

	struct FrameData {
		vk2d::VertexBuffer vertex_buffer;
		vk2d::Buffer       index_buffer;
	};

	vk2d::Window*           window;
	std::vector<FrameData>  frames;
	std::vector<WindowData> window_datas;
	BackendData*            backend;
	bool                    window_owned;
};

// glsl_shader.vert, compiled with:
// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
/*
#version 450 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;
layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;

out gl_PerVertex { vec4 gl_Position; };
layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;

void main()
{
	Out.Color = aColor;
	Out.UV = aUV;
	gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
}
*/
static uint32_t __glsl_shader_vert_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
	0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
	0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
	0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
	0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
	0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
	0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
	0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
	0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
	0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
	0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
	0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
	0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
	0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
	0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
	0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
	0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
	0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
	0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
	0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
	0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
	0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
	0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
	0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
	0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
	0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
	0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
	0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
	0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
	0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
	0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
	0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
	0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
	0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
	0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
	0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
	0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
	0x0000002d,0x0000002c,0x000100fd,0x00010038
};

// glsl_shader.frag, compiled with:
// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
/*
#version 450 core
layout(location = 0) out vec4 fColor;
layout(set=0, binding=0) uniform sampler2D sTexture;
layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
void main()
{
	fColor = In.Color * texture(sTexture, In.UV.st);
}
*/
static uint32_t __glsl_shader_frag_spv[] =
{
	0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
	0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
	0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
	0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
	0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
	0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
	0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
	0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
	0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
	0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
	0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
	0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
	0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
	0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
	0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
	0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
	0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
	0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
	0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
	0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
	0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
	0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
	0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
	0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
	0x00010038
};

static std::map<std::intptr_t, BackendData*> backends;

static void bind_key_map(ImGuiIO& io) 
{
	io.KeyMap[ImGuiKey_Tab]        = (int)vk2d::Key::Tab;
	io.KeyMap[ImGuiKey_LeftArrow]  = (int)vk2d::Key::Left;
	io.KeyMap[ImGuiKey_RightArrow] = (int)vk2d::Key::Right;
	io.KeyMap[ImGuiKey_UpArrow]    = (int)vk2d::Key::Up;
	io.KeyMap[ImGuiKey_DownArrow]  = (int)vk2d::Key::Down;
	io.KeyMap[ImGuiKey_PageUp]     = (int)vk2d::Key::PageUp;
	io.KeyMap[ImGuiKey_PageDown]   = (int)vk2d::Key::PageDown;
	io.KeyMap[ImGuiKey_Home]       = (int)vk2d::Key::Home;
	io.KeyMap[ImGuiKey_End]        = (int)vk2d::Key::End;
	io.KeyMap[ImGuiKey_Insert]     = (int)vk2d::Key::Insert;
	io.KeyMap[ImGuiKey_Delete]     = (int)vk2d::Key::Delete;
	io.KeyMap[ImGuiKey_Backspace]  = (int)vk2d::Key::Backspace;
	io.KeyMap[ImGuiKey_Space]      = (int)vk2d::Key::Space;
	io.KeyMap[ImGuiKey_Enter]      = (int)vk2d::Key::Enter;
	io.KeyMap[ImGuiKey_Escape]     = (int)vk2d::Key::Escape;
	io.KeyMap[ImGuiKey_A]          = (int)vk2d::Key::A;
	io.KeyMap[ImGuiKey_B]          = (int)vk2d::Key::B;
	io.KeyMap[ImGuiKey_C]          = (int)vk2d::Key::C;
	io.KeyMap[ImGuiKey_D]          = (int)vk2d::Key::D;
	io.KeyMap[ImGuiKey_E]          = (int)vk2d::Key::E;
	io.KeyMap[ImGuiKey_F]          = (int)vk2d::Key::F;
	io.KeyMap[ImGuiKey_G]          = (int)vk2d::Key::G;
	io.KeyMap[ImGuiKey_H]          = (int)vk2d::Key::H;
	io.KeyMap[ImGuiKey_I]          = (int)vk2d::Key::I;
	io.KeyMap[ImGuiKey_J]          = (int)vk2d::Key::J;
	io.KeyMap[ImGuiKey_K]          = (int)vk2d::Key::K;
	io.KeyMap[ImGuiKey_L]          = (int)vk2d::Key::L;
	io.KeyMap[ImGuiKey_M]          = (int)vk2d::Key::M;
	io.KeyMap[ImGuiKey_N]          = (int)vk2d::Key::N;
	io.KeyMap[ImGuiKey_O]          = (int)vk2d::Key::O;
	io.KeyMap[ImGuiKey_P]          = (int)vk2d::Key::P;
	io.KeyMap[ImGuiKey_Q]          = (int)vk2d::Key::Q;
	io.KeyMap[ImGuiKey_R]          = (int)vk2d::Key::R;
	io.KeyMap[ImGuiKey_S]          = (int)vk2d::Key::S;
	io.KeyMap[ImGuiKey_T]          = (int)vk2d::Key::T;
	io.KeyMap[ImGuiKey_U]          = (int)vk2d::Key::U;
	io.KeyMap[ImGuiKey_V]          = (int)vk2d::Key::V;
	io.KeyMap[ImGuiKey_W]          = (int)vk2d::Key::W;
	io.KeyMap[ImGuiKey_X]          = (int)vk2d::Key::X;
	io.KeyMap[ImGuiKey_Y]          = (int)vk2d::Key::Y;
	io.KeyMap[ImGuiKey_Z]          = (int)vk2d::Key::Z;
}

static BackendData& get_backend_data(const vk2d::Window& window)
{
	auto& bd = *backends[(std::intptr_t)window.getNativeHandle()];
	ImGui::SetCurrentContext(bd.context);

	return bd;
}

void set_clipboard_text(void* user_data, const char* text) 
{
	vk2d::Clipboard::setString(text);
}

const char* get_clipboard_text(void* user_data) 
{
	std::string str = vk2d::Clipboard::getString();

	char* c_str = new char[str.size()];
	memcpy(c_str, str.data(), str.size());
	return c_str;
}

static vk2d::Pipeline build_pipeline() 
{
	vk2d::PipelineBuilder builder;

	builder.addShader(__glsl_shader_vert_spv, sizeof(__glsl_shader_vert_spv), vk::ShaderStageFlagBits::eVertex);
	builder.addShader(__glsl_shader_frag_spv, sizeof(__glsl_shader_frag_spv), vk::ShaderStageFlagBits::eFragment);

	builder.addVertexInputBinding(0, sizeof(ImDrawVert));
	builder.setVertexInputAttribute(0, 0, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, pos));
	builder.setVertexInputAttribute(0, 1, vk::Format::eR32G32Sfloat, offsetof(ImDrawVert, uv));
	builder.setVertexInputAttribute(0, 2, vk::Format::eR8G8B8A8Unorm, offsetof(ImDrawVert, col));

	builder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleList);

	builder.addPushConstant(vk::ShaderStageFlagBits::eVertex, sizeof(vec2[2]));
	builder.addDescriptorSetLayoutBinding(vk::ShaderStageFlagBits::eFragment, 0, vk::DescriptorType::eCombinedImageSampler);

	return builder.build();
}

static void update_monitors(BackendData& bd)
{
	bd.want_update_monitors = false;

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

#ifdef  _WIN32
	struct ScreenArray {
		static BOOL CALLBACK MonitorEnumProc(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data) {
			MONITORINFOEX mi = { 0 };

			mi.cbSize = sizeof(MONITORINFOEX);
			GetMonitorInfo(monitor, &mi);

			if (mi.dwFlags != DISPLAY_DEVICE_MIRRORING_DRIVER) {
				auto p = reinterpret_cast<ScreenArray*>(data);
				p->Monitors.push_back(*rect);
			}
			return TRUE;
		}

		ScreenArray() {
			Monitors.clear();
			EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)this);
		}

		std::vector<RECT> Monitors;
	};

	ScreenArray monitors;

	for (auto& rect : monitors.Monitors) {
		ImGuiPlatformMonitor monitor;
		ImVec2 pos((float)rect.left, (float)rect.top);
		ImVec2 size((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

		monitor.MainPos  = monitor.WorkPos  = pos;
		monitor.MainSize = monitor.WorkSize = size;
		platform_io.Monitors.push_back(monitor);
	}
#endif //  _WIN32
}

static void process_viewport_event(vk2d::Window& window, vk2d::Event& event)
{
	if (window.hasFocus()) {
		auto& io = ImGui::GetIO();

		switch (event.type) {
		case vk2d::Event::MouseMoved:
			auto pos = event.mouseMove.pos + window.getPosition();
			io.AddMousePosEvent((float)pos.x, (float)pos.y);
			break;
		}
	}
}

static void ImGui_impl_vk2d_CreateWindow(ImGuiViewport* viewport)
{
	auto& vd = *(new ViewportWindowData());

	uint32_t width  = (uint32_t)viewport->Size.x;
	uint32_t height = (uint32_t)viewport->Size.y;
	int32_t  style  = vk2d::Window::Style::None;

	auto& main_vd = *(ViewportWindowData*)ImGui::GetMainViewport()->PlatformUserData;

	vd.window = new vk2d::Window;
	vd.window->create(width, height, "untitled", *main_vd.window, style);
	vd.window->setPosition(to_ivec2(viewport->Pos));
	vd.backend      = main_vd.backend;
	vd.window_owned = true;

	viewport->PlatformUserData  = &vd;
	viewport->PlatformHandle    = (void*)vd.window;
	viewport->PlatformHandleRaw = vd.window->getNativeHandle();
}

static void ImGui_impl_vk2d_DestroyWindow(ImGuiViewport* viewport)
{
	if (!viewport->PlatformUserData) return;

	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	viewport->PlatformUserData = nullptr;
	
	if (vd.window_owned) 
		delete vd.window;
	delete &vd;
}

static void ImGui_impl_vk2d_ShowWindow(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setVisible(true);
}

static void ImGui_impl_vk2d_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setPosition(to_ivec2(pos));
}

static ImVec2 ImGui_impl_vk2d_GetWindowPos(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	return to_ImVec2(vd.window->getPosition());
}

static void ImGui_impl_vk2d_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setSize(to_uvec2(size));
}

static ImVec2 ImGui_impl_vk2d_GetWindowSize(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	return to_ImVec2(vd.window->getSize());
}

static void ImGui_impl_vk2d_SetWindowFocus(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setFocus();
}

static bool ImGui_impl_vk2d_GetWindowFocus(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	return vd.window->hasFocus();
}

static bool ImGui_impl_vk2d_GetWindowMinimized(ImGuiViewport* viewport)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	return vd.window->isMinimized();
}

static void ImGui_impl_vk2d_SetWindowTitle(ImGuiViewport* viewport, const char* str)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setTitle(str);
}

static void ImGui_impl_vk2d_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window->setTransparency(alpha);
}

static void ImGui_impl_vk2d_RenderWindow(ImGuiViewport* viewport, void* render_arg)
{
	Render(viewport->DrawData);
}

static void ImGui_impl_vk2d_SwapBuffers(ImGuiViewport* viewport, void* render_arg)
{
	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);

	vd.window->display();
}

IMGUI_VK2D_API void Init(vk2d::Window& window, bool load_default_font)
{
	backends[(std::intptr_t)window.getNativeHandle()] = new BackendData();
	
	auto& bd   = get_backend_data(window);
	bd.context = ImGui::CreateContext();
	bd.window  = &window;

	// pipeline
	bd.pipeline = build_pipeline();

	// load cursors
	bd.cursors[ImGuiMouseCursor_Arrow].loadFromSystem(vk2d::Cursor::Arrow);
	bd.cursors[ImGuiMouseCursor_TextInput].loadFromSystem(vk2d::Cursor::Text);
	bd.cursors[ImGuiMouseCursor_ResizeAll].loadFromSystem(vk2d::Cursor::SizeAll);
	bd.cursors[ImGuiMouseCursor_ResizeNS].loadFromSystem(vk2d::Cursor::SizeVertical);
	bd.cursors[ImGuiMouseCursor_ResizeEW].loadFromSystem(vk2d::Cursor::SizeHorizontal);
	bd.cursors[ImGuiMouseCursor_ResizeNESW].loadFromSystem(vk2d::Cursor::SizeBottomLeftTopRight);
	bd.cursors[ImGuiMouseCursor_ResizeNWSE].loadFromSystem(vk2d::Cursor::SizeTopLeftBottomRight);
	bd.cursors[ImGuiMouseCursor_Hand].loadFromSystem(vk2d::Cursor::Hand);

	bd.want_update_monitors = false;

	auto& io = ImGui::GetIO();
	
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
	io.BackendPlatformName = "imgui_impl_vk2d";

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// frame buffer size
	io.DisplaySize = to_ImVec2((vec2)window.getFrameBufferSize());

	// bind keys
	bind_key_map(io);

	// clipboard
	io.SetClipboardTextFn = set_clipboard_text;
	io.GetClipboardTextFn = get_clipboard_text;

	// platform window
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

	platform_io.Platform_CreateWindow       = ImGui_impl_vk2d_CreateWindow;
	platform_io.Platform_DestroyWindow      = ImGui_impl_vk2d_DestroyWindow;
	platform_io.Platform_ShowWindow         = ImGui_impl_vk2d_ShowWindow;
	platform_io.Platform_SetWindowPos       = ImGui_impl_vk2d_SetWindowPos;
	platform_io.Platform_GetWindowPos       = ImGui_impl_vk2d_GetWindowPos;
	platform_io.Platform_SetWindowSize      = ImGui_impl_vk2d_SetWindowSize;
	platform_io.Platform_GetWindowSize      = ImGui_impl_vk2d_GetWindowSize;
	platform_io.Platform_SetWindowFocus     = ImGui_impl_vk2d_SetWindowFocus;
	platform_io.Platform_GetWindowFocus     = ImGui_impl_vk2d_GetWindowFocus;
	platform_io.Platform_GetWindowMinimized = ImGui_impl_vk2d_GetWindowMinimized;
	platform_io.Platform_SetWindowTitle     = ImGui_impl_vk2d_SetWindowTitle;
	platform_io.Platform_SetWindowAlpha     = ImGui_impl_vk2d_SetWindowAlpha;
	platform_io.Platform_RenderWindow       = ImGui_impl_vk2d_RenderWindow;
	platform_io.Platform_SwapBuffers        = ImGui_impl_vk2d_SwapBuffers;

	if (load_default_font)
		UpdateFontTexture(window);

	auto* viewport = ImGui::GetMainViewport();
	auto& vd            = *(new ViewportWindowData);
	vd.window           = &window;
	vd.backend          = &bd;
	vd.window_owned     = false;

	viewport->PlatformUserData  = &vd;
	viewport->PlatformHandle    = (void*)vd.window;
	viewport->PlatformHandleRaw = vd.window->getNativeHandle();

	update_monitors(bd);
}

IMGUI_VK2D_API void SetCurrentContext(const vk2d::Window& window)
{
	get_backend_data(window);
}

IMGUI_VK2D_API void ProcessEvent(const vk2d::Window& window, const vk2d::Event& event)
{
	auto& io = ImGui::GetIO();

	switch (event.type) {
	case vk2d::Event::Resize: {
		if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle((void*)&window)) {
			viewport->PlatformRequestResize = true;
		}
	} break;
	case vk2d::Event::MouseMoved: {
		auto pos = event.mouseMove.pos;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			pos += window.getPosition();
		}
		io.AddMousePosEvent((float)pos.x, (float)pos.y);
	} break;
	case vk2d::Event::MousePressed:
		io.AddMouseButtonEvent((int)event.mouseButton.button, true);
		break;
	case vk2d::Event::MouseReleased:
		io.AddMouseButtonEvent((int)event.mouseButton.button, false);
		break;
	case vk2d::Event::WheelScrolled:
		io.AddMouseWheelEvent(event.wheel.delta.x, event.wheel.delta.y);
		break;
	case vk2d::Event::KeyPressed:
	case vk2d::Event::KeyReleased: {
		int key = (int)event.keyboard.key;
		if (key >= 0 && key < IM_ARRAYSIZE(io.KeysDown)) {
			io.KeysDown[key] = (event.type == vk2d::Event::KeyPressed);
		}

		io.KeyCtrl  = event.keyboard.ctrl;
		io.KeyAlt   = event.keyboard.alt;
		io.KeyShift = event.keyboard.shft;
		io.KeySuper = event.keyboard.sys;
	} break;
	case vk2d::Event::TextEntered:
		// Don't handle the event for unprintable characters
		if (event.text.unicode < ' ' || event.text.unicode == 127) break;
		io.AddInputCharacter(event.text.unicode);
		break;
	case vk2d::Event::LostFocus:
		io.AddFocusEvent(false);
		break;
	case vk2d::Event::Focussed:
		io.AddFocusEvent(true);
		break;
	default:
		break;
	}

	auto& main_vd = *static_cast<ViewportWindowData*>(ImGui::GetMainViewport()->PlatformUserData);
	
	for (auto& wd : main_vd.window_datas)
		wd.event_proc(event, io.DeltaTime, wd.event_user_data);
}

IMGUI_VK2D_API void ProcessViewportEvent(const vk2d::Window& window)
{
	auto& bd          = get_backend_data(window);
	auto& io          = ImGui::GetIO();
	auto& platform_io = ImGui::GetPlatformIO();

	auto& main_vd = *static_cast<ViewportWindowData*>(ImGui::GetMainViewport()->PlatformUserData);
	main_vd.window_datas.clear();

	for (int i = 1;  i < platform_io.Viewports.size(); ++i) {
		auto& vd = *static_cast<ViewportWindowData*>(platform_io.Viewports[i]->PlatformUserData);
		
		vk2d::Event e;
		while (vd.window->pollEvent(e)) {
			ProcessEvent(*vd.window, e);
			
			for (auto& wd : vd.window_datas)
				wd.event_proc(e, io.DeltaTime, wd.event_user_data);
		}
		vd.window_datas.clear();
	}
}

IMGUI_VK2D_API void setViewportEventProc(ImGuiViewport* viewport, EventProc proc, void* user_data)
{
	VK2D_ASSERT(viewport && proc);

	if (!viewport->PlatformUserData) return;

	auto& vd = *static_cast<ViewportWindowData*>(viewport->PlatformUserData);
	vd.window_datas.emplace_back(proc, user_data);
}

IMGUI_VK2D_API void Update(vk2d::Window& window, float dt)
{
	auto& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		Update(window, vk2d::Mouse::getPosition(), dt);
	else
		Update(window, vk2d::Mouse::getPosition(window), dt);
}

IMGUI_VK2D_API void Update(vk2d::Window& window, const ivec2& mouse_pos, float dt)
{
	auto& bd = get_backend_data(window);
	auto& io = ImGui::GetIO();

	io.DisplaySize = to_ImVec2(window.getFrameBufferSize());
	io.DeltaTime   = dt;

	if (window.hasFocus()) {
		if (io.WantSetMousePos) {
			vk2d::Mouse::setPosition(to_vec2(io.MousePos));
		} else {
			io.AddMousePosEvent((float)mouse_pos.x, (float)mouse_pos.y);
		}

		auto curr_cursor = ImGui::GetIO().MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();

		if (bd.last_cursor != curr_cursor) {
			if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)) {
				if (curr_cursor == ImGuiMouseCursor_None) {
					vk2d::Cursor::setVisible(false);
				} else {
					vk2d::Cursor::setVisible(true);
					if (bd.cursors[curr_cursor].valid()) {
						vk2d::Cursor::setCursor(bd.cursors[curr_cursor]);
					} else {
						vk2d::Cursor::setCursor(bd.cursors[ImGuiMouseCursor_Arrow]);
					}
				}
			}

			bd.last_cursor = curr_cursor;
		}
	}

	ImGui::NewFrame();
}

static void prepare_frame_data(ImDrawData* draw_data, ViewportWindowData::FrameData& frame) {
	auto& inst   = vk2d::VK2DContext::get();
	auto& device = inst.device;

	{ // prepare vertex/index buffers
		size_t vertex_size = draw_data->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size  = draw_data->TotalIdxCount * sizeof(ImDrawIdx);
		
		if (frame.vertex_buffer.size() < vertex_size)
			frame.vertex_buffer.resize(vertex_size);
		
		if (frame.index_buffer.size() < index_size)
			frame.index_buffer.resize(index_size, vk::BufferUsageFlagBits::eIndexBuffer);

		auto* vtx_dst = (ImDrawVert*)frame.vertex_buffer.data();
		auto* idx_dst = (ImDrawIdx*)frame.index_buffer.data();

		for (int n = 0; n < draw_data->CmdListsCount; n++) {
			const auto* cmd_list = draw_data->CmdLists[n];
			memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtx_dst += cmd_list->VtxBuffer.Size;
			idx_dst += cmd_list->IdxBuffer.Size;
		}

		frame.vertex_buffer.update();
		frame.index_buffer.update();
	}
}

IMGUI_VK2D_API void Render(ImDrawData* draw_data)
{
	if (!draw_data || draw_data->TotalVtxCount == 0) return;

	auto& inst     = vk2d::VK2DContext::get();
	auto* viewport = draw_data->OwnerViewport;
	auto& vd       = *(ViewportWindowData*)viewport->PlatformUserData;
	auto& bd       = *vd.backend;
	auto& io       = ImGui::GetIO();
	auto fb_size   = vd.window->getFrameBufferSize();

	if (vd.window->isMinimized() || !fb_size.x || !fb_size.y) return;

	vk2d::RenderTarget& target = *vd.window;
	target.beginRenderPass();
	vd.frames.resize(target.getFrameCount());

	auto& frame     = vd.frames[target.getCurrentFrameIdx()];
	auto cmd_buffer = target.getCommandBuffer();

	if (fb_size.x <= 0 || fb_size.y <= 0) return;
	
	prepare_frame_data(draw_data, frame);

	auto SetupRenderState = [&]() {
		cmd_buffer.bindPipeline(
			vk::PipelineBindPoint::eGraphics,
			bd.pipeline.pipeline
		);

		vk::Viewport viewport = {
			0, 0, (float)fb_size.x, (float)fb_size.y
		};

		cmd_buffer.setViewport(0, 1, &viewport);

		if (draw_data->TotalVtxCount > 0) {
			vk::Buffer vertex_buffer = frame.vertex_buffer.getBuffer();
			vk::DeviceSize offset = 0;

			cmd_buffer.bindVertexBuffers(0, 1, &vertex_buffer, &offset);
			cmd_buffer.bindIndexBuffer(frame.index_buffer.getBuffer(), 0, sizeof(ImDrawIdx) == 2 ? vk::IndexType::eUint16 : vk::IndexType::eUint32);
		}

		float pc[4];
		pc[0] = 2.0f / draw_data->DisplaySize.x;
		pc[1] = 2.0f / draw_data->DisplaySize.y;
		pc[2] = -1.0f - draw_data->DisplayPos.x * pc[0];
		pc[3] = -1.0f - draw_data->DisplayPos.y * pc[1];

		cmd_buffer.pushConstants(*bd.pipeline.pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(vec2[2]), pc);
	};

	SetupRenderState();

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	// (Because we merged all buffers into a single one, we maintain our own offset into them)
	int global_vtx_offset = 0;
	int global_idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback != nullptr)
			{
				// User callback, registered via ImDrawList::AddCallback()
				// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
				if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
					SetupRenderState();
				else
					pcmd->UserCallback(cmd_list, pcmd);
			} else {
				// Project scissor/clipping rectangles into framebuffer space
				ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
				ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);

				// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
				if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
				if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
				if (clip_max.x > fb_size.x) { clip_max.x = (float)fb_size.x; }
				if (clip_max.y > fb_size.y) { clip_max.y = (float)fb_size.y; }
				if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
					continue;

				// Apply scissor/clipping rectangle
				vk::Rect2D scissor = {
					{ (int32_t)(clip_min.x), (int32_t)(clip_min.y) },
					{ (uint32_t)(clip_max.x - clip_min.x), (uint32_t)(clip_max.y - clip_min.y) }
				};
				
				cmd_buffer.setScissor(0, 1, &scissor);

				auto descriptor_set = *(vk::DescriptorSet*)&pcmd->TextureId;
				cmd_buffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics, 
					*bd.pipeline.pipeline_layout, 
					0, 
					1, &descriptor_set,
					0, nullptr);

				// Draw
				cmd_buffer.drawIndexed(pcmd->ElemCount, 1, pcmd->IdxOffset + global_idx_offset, pcmd->VtxOffset + global_vtx_offset, 0);
			}
		}
		global_idx_offset += cmd_list->IdxBuffer.Size;
		global_vtx_offset += cmd_list->VtxBuffer.Size;
	}
}

IMGUI_VK2D_API void RenderViewports(vk2d::Window& window)
{
	auto& bd = get_backend_data(window);

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	for (int i = 1; i < platform_io.Viewports.Size; i++) {
		ImGuiViewport* viewport = platform_io.Viewports[i];
		if (viewport->Flags & ImGuiViewportFlags_IsMinimized) continue;
		if (platform_io.Platform_RenderWindow) platform_io.Platform_RenderWindow(viewport, nullptr);
		if (platform_io.Renderer_RenderWindow) platform_io.Renderer_RenderWindow(viewport, nullptr);
	}
}

IMGUI_VK2D_API void DisplayViewports(vk2d::Window& window, bool display_main_viewport)
{
	auto& bd = get_backend_data(window);

	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	for (int i = display_main_viewport ? 0 : 1; i < platform_io.Viewports.Size; i++) {
		ImGuiViewport* viewport = platform_io.Viewports[i];
		if (viewport->Flags & ImGuiViewportFlags_IsMinimized) continue;
		if (platform_io.Platform_SwapBuffers) platform_io.Platform_SwapBuffers(viewport, nullptr);
		if (platform_io.Renderer_SwapBuffers) platform_io.Renderer_SwapBuffers(viewport, nullptr);
	}
}

IMGUI_VK2D_API void ShutDown(vk2d::Window& window)
{
	auto handle = (std::intptr_t)window.getNativeHandle();
	auto& bd    = get_backend_data(window);

	bd.pipeline.destroy();
	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i)
		bd.cursors[i].destroy();

	ImGui::DestroyContext(bd.context);
	delete &bd;

	backends.erase(handle);
}

IMGUI_VK2D_API bool UpdateFontTexture(const vk2d::Window& window)
{
	auto& bd = get_backend_data(window);
	auto& io = ImGui::GetIO();

	vk2d::Color* pixels;
	int width, height;

	io.Fonts->GetTexDataAsRGBA32((unsigned char**)&pixels, &width, &height);

	bd.font_texture.resize(width, height);
	bd.font_texture.update(pixels, uvec2(width, height));

	auto desc_set = bd.font_texture.getDescriptorSet();
	io.Fonts->SetTexID(*(ImTextureID*)&desc_set);

	return true;
}

IMGUI_VK2D_API vk2d::Texture& GetFontTexture(const vk2d::Window& window)
{
	return backends[(std::intptr_t)window.getNativeHandle()]->font_texture;
}

IMGUI_VK2D_END