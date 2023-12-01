#include "dialog.h"

#include <imgui_internal.h>
#include <thread>
#include "imgui_impl_vk2d.h"
#include "resizing_loop.h"
#include "platform/platform_impl.h"
#include "../micro_logic_config.h"

namespace priv {
	class DialogImpl : public ResizingLoop {
	public:
		vk2d::Window   window;
		CustomTitleBar titlebar;
		ImGuiContext*  ctx;
		ImGuiContext*  prev_ctx;
		Dialog*        curr_dialog;

		void loop() override {
			ResizingLoop::loop();

			curr_dialog->window_size = to_ImVec2(window.getSize());
			curr_dialog->client_size = curr_dialog->window_size;
			curr_dialog->client_size.y -= curr_dialog->title_height;
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
	impl->ctx = ImGui::GetCurrentContext();

	auto& io    = ImGui::GetIO();
	auto& style = ImGui::GetStyle(); 

	io.IniFilename = nullptr;

	style.Colors[ImGuiCol_WindowBg]      = ImVec4(0.14f, 0.14f, 0.14f, 1.f); 
	style.Colors[ImGuiCol_Button]        = ImVec4(0.23f, 0.23f, 0.23f, 1.f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.3f, 0.3f, 0.3f, 1.f);
	style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.2f, 0.2f, 0.2f, 1.f);

	auto* font = io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE, nullptr, io.Fonts->GetGlyphRangesKorean());
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

Dialog::Dialog(const std::string& title, bool resizable) :
	title(title),
	owner(nullptr),
	impl(get_dialog_impl(this)),
	window(impl->window),
	titlebar(impl->titlebar),
	window_size(),
	client_size(),
	title_height(impl->ctx->FontSize + 20)
{
	window.setResizable(resizable);
}

Dialog::~Dialog()
{
	window.setVisible(false);
	impl->curr_dialog = nullptr;
}

void Dialog::switchContext() const
{
	impl->prev_ctx = ImGui::GetCurrentContext();
	ImGui::VK2D::SetCurrentContext(window);
}

void Dialog::beginShowDialog(ImVec2 size) const
{
	client_size = size;
	window_size = ImVec2(size.x, size.y + title_height);
	
	titlebar.setCaptionRect({ 0.f, 0.f, size.x, title_height });
	
	window.setSize(to_uvec2(window_size));
	window.setTitle(title.c_str());
	if (owner) {
		auto center = owner->getPosition() + (ivec2)(owner->getSize() - window.getSize()) / 2;
		window.setPosition(center);
		window.setParent(*owner);
	}
	window.setVisible(true);
}

void Dialog::endShowDialog() const
{
	window.setVisible(false);
	ImGui::SetCurrentContext(impl->prev_ctx);
}

void Dialog::beginDialogWindow(const char* name, bool* p_open, ImGuiWindowFlags flags) const
{
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
	ImGui::SetNextWindowPos(ImVec2(0.f, title_height));
	ImGui::SetNextWindowSize(client_size);
	ImGui::Begin(name, p_open, flags);
}

void Dialog::endDialogWindow() const
{
	ImGui::End();
	ImGui::PopStyleColor();
}

bool Dialog::updateDialog() const
{
	vk2d::Event e;
	while (window.pollEvent(e)) {
		if (e.type == vk2d::Event::Close) {
			window.setVisible(false);
			return false;
		}
		ImGui::VK2D::ProcessEvent(window, e);
	}

	ImGui::VK2D::Update(window, getDeltaTime());

	if (titlebar.beginTitleBar()) {
		ImGui::TextUnformatted(title.c_str());
		titlebar.endTitleBar();
	}
	
	return true;
}

void Dialog::renderDialog() const
{
	using namespace std::chrono;

	ImGui::Render();
	ImGui::VK2D::Render(ImGui::GetDrawData());
	ImGui::EndFrame();

	window.display();

	std::this_thread::sleep_for(10ms);
}

float Dialog::getDeltaTime() const
{
	return impl->delta_time;
}
