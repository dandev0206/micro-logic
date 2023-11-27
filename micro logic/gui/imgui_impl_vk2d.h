#pragma once

#include <vk2d/system/window.h>

#include "../vector_type.h"

#define IMGUI_VK2D_BEGIN namespace ImGui { namespace VK2D {
#define IMGUI_VK2D_END } }
#define IMGUI_VK2D_API

struct ImDrawData;
struct ImGuiViewport;

IMGUI_VK2D_BEGIN

typedef void(*EventProc)(const vk2d::Event&, float, void*);

IMGUI_VK2D_API void Init(vk2d::Window& window, bool load_default_font = false);

IMGUI_VK2D_API void ProcessEvent(const vk2d::Window& window, const vk2d::Event& event);
IMGUI_VK2D_API void ProcessViewportEvent(const vk2d::Window& window);
IMGUI_VK2D_API void setViewportEventProc(ImGuiViewport* viewport, EventProc proc, void* user_data);

IMGUI_VK2D_API void Update(vk2d::Window& window, float dt);
IMGUI_VK2D_API void Update(vk2d::Window& window, const ivec2& mouse_pos, float dt);

IMGUI_VK2D_API void Render(ImDrawData* draw_data);
IMGUI_VK2D_API void Render(ImDrawData* draw_data);
IMGUI_VK2D_API void RenderViewports(vk2d::Window& window);
IMGUI_VK2D_API void DisplayViewports(vk2d::Window& window, bool display_main_viewport = false);

IMGUI_VK2D_API void ShutDown(vk2d::Window&window);

IMGUI_VK2D_API bool UpdateFontTexture(const vk2d::Window& window);
IMGUI_VK2D_API vk2d::Texture& GetFontTexture(const vk2d::Window& window);

IMGUI_VK2D_END