#include "dialog.h"

#include <imgui_internal.h>
#include "imgui_impl_vk2d.h"
#include "resizing_loop.h"
#include "platform/platform_impl.h"
#include "../micro_logic_config.h"

namespace priv {
	class DialogImpl : public ResizingLoop {
	public:
		vk2d::Window   window;
		CustomTitleBar titlebar;
		ImGuiContext*  prev_ctx;
		Dialog*        curr_dialog;

		void loop() override {
			ResizingLoop::loop();
			curr_dialog->dialogLoop();
		}

		void eventProc(const vk2d::Event& e) override {
			curr_dialog->dialogEventProc(e);
		}
	};
}

std::vector<priv::DialogImpl*> impls;

static priv::DialogImpl* push_dialog_impl(Dialog* dialog = nullptr) 
{
	auto* impl = impls.emplace_back(new priv::DialogImpl);
	
	impl->window.create(100, 100, "untitled", vk2d::Window::Titlebar);
	impl->titlebar.bindWindow(impl->window);
	impl->prev_ctx    = ImGui::GetCurrentContext();
	impl->curr_dialog = dialog;

	InjectResizingLoop(impl->window, impl);
	
	ImGui::VK2D::Init(impl->window);

	auto& io    = ImGui::GetIO();
	auto& style = ImGui::GetStyle();

	io.IniFilename = nullptr;

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.f);

	auto* font = io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE);
	ImGui::VK2D::UpdateFontTexture(impl->window);
	
	ImGui::VK2D::Update(impl->window, 1.f / 60.f);
	ImGui::Begin("TempWindow");
	ImGui::SetCurrentFont(font);
	ImGui::End();
	ImGui::EndFrame();

	ImGui::SetCurrentContext(impl->prev_ctx);

	return impl;
}

static priv::DialogImpl* get_dialog_impl(Dialog* dialog) 
{
	for (int64_t i = (int64_t)impls.size() - 1; 0 <= i; --i) {
		auto* impl = impls[i];
		if (!impl->curr_dialog) {
			impl->curr_dialog = dialog;
			return impl;
		}
	}

	return push_dialog_impl(dialog);
}

void Dialog::pushImpls(size_t count)
{
	for (size_t i = 0; i < count; ++i)
		push_dialog_impl();
}

void Dialog::destroyImpl()
{
	for (auto* impl : impls) {
		ImGui::VK2D::ShutDown(impl->window);
		delete impl;
	}

	impls.clear();
}

Dialog::Dialog(bool resizable) :
	owner(nullptr),
	impl(get_dialog_impl(this)),
	window(impl->window),
	titlebar(impl->titlebar)
{
	window.setResizable(resizable);
}

Dialog::~Dialog()
{
	impl->window.setVisible(false);
	impl->curr_dialog = nullptr;
}

void Dialog::beginShowDialog() const
{
	impl->prev_ctx = ImGui::GetCurrentContext();
	ImGui::VK2D::SetCurrentContext(impl->window);
	if (owner) impl->window.setParent(*owner);
}

void Dialog::endShowDialog() const
{
	impl->window.setVisible(false);
	ImGui::SetCurrentContext(impl->prev_ctx);
}

float Dialog::getDeltaTime() const
{
	return impl->delta_time;
}
