#include "message_box.h"

#include "imgui_impl_vk2d.h"
#include <imgui_internal.h>
#include "../micro_logic_config.h"
#include <thread>

static std::vector<std::string> split_str(const std::string& str, char ch) 
{
	std::vector<std::string> result;

	size_t first = 0;
	size_t last  = str.find(ch);

	while (last != std::string::npos) {
		result.push_back(str.substr(first, last - first));
		first = last + 1;
		last  = str.find(ch, first);
	}

	result.push_back(str.substr(first, std::min(last, str.size()) - first + 1));

	return result;
}

static ImVec2 calc_buttons_size(const std::vector<std::string>& button_names)
{
	auto& style = ImGui::GetStyle();

	auto buttons_size = ImVec2(0.f, 0.f);
	for (const auto& name : button_names) {
		auto text_size = ImGui::CalcTextSize(name.c_str(), nullptr, true);
		auto size      = ImVec2(text_size.x + 2.f * style.FramePadding.x, text_size.y + 2.f * style.FramePadding.y);

		size = ImGui::CalcItemSize(ImVec2(100, 40), size.x, size.y);

		buttons_size.x += size.x + style.ItemSpacing.x;
		buttons_size.y  = size.y;
	}
	buttons_size.x -= style.ItemSpacing.x;

	return buttons_size;
}

MessageBox::MessageBox() :
	owner(nullptr)
{
	titlebar.setButtonStyle(false, false, true);
}

MessageBox::~MessageBox()
{
	ImGui::VK2D::ShutDown(window);
}

void MessageBox::init(const vk2d::Window* owner)
{
	auto* prev_context = ImGui::GetCurrentContext();
	
	int32_t window_style = vk2d::Window::Style::None;

	if (owner) {
		window.create(100, 100, "MessageBox", *owner, window_style);
		this->owner = owner;
	} else
		window.create(100, 100, "MessageBox", window_style);

	ImGui::VK2D::Init(window);

	auto& io    = ImGui::GetIO();
	auto& style = ImGui::GetStyle();

	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.f);

	io.IniFilename = nullptr;

	auto* font = io.Fonts->AddFontFromFileTTF(FONT_PATH, FONT_SIZE);
	ImGui::VK2D::UpdateFontTexture(window);
	
	ImGui::VK2D::Update(window, 0.1f);
	ImGui::Begin("TempWindow");
	ImGui::SetCurrentFont(font);
	ImGui::End();
	ImGui::EndFrame();

	ImGui::SetCurrentContext(prev_context);
}

const vk2d::Window& MessageBox::getWindow() const
{
	return window;
}

const CustomTitleBar& MessageBox::getTitlebar() const
{
	return titlebar;
}

std::string MessageBox::showDialog() 
{
	using namespace std::chrono;

	static const ImVec2 button_size(100, 40);
	static const auto flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDecoration;

	auto* prev_context = ImGui::GetCurrentContext();
	ImGui::VK2D::SetCurrentContext(window);

	auto& style = ImGui::GetStyle();

	auto button_names  = split_str(buttons, ';');
	auto buttons_size  = calc_buttons_size(button_names);
	auto result        = std::string("Error");

	ImVec2 text_size = ImGui::CalcTextSize(content.c_str(), nullptr, false, 0.f);
	ImVec2 window_size;
	window_size.x = 500.f;
	window_size.x = std::max(window_size.x, 100.f + buttons_size.x);
	window_size.x = std::max(window_size.x, text_size.x + 80.f);
	window_size.y = std::max(150.f, text_size.y + buttons_size.y + 90.f);

	float title_height = ImGui::GetFontSize() + 20;
	ImVec2 client_size = ImVec2(window_size.x, window_size.y - title_height);
	
	window.setTitle(title.c_str());
	window.setSize((uvec2)to_vec2(window_size));
	if (owner)
		window.setPosition(owner->getPosition() + (ivec2)(owner->getSize() - window.getSize()) / 2);
	window.setVisible(true);
	titlebar.setCaptionRect({ 0.f, 0.f, window_size.x, title_height });

	while (window.isVisible()) {
		vk2d::Event e;
		while (window.pollEvent(e)) {
			if (e.type == vk2d::Event::Close) {
				result = std::string("Close");
				window.setVisible(false);
			}
			ImGui::VK2D::ProcessEvent(window, e);
		}

		ImGui::VK2D::Update(window, 0.1f);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
		if (ImGui::BeginMainMenuBar()) {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.f, 0.f, 0.f, 0.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 1.f, 1.f, 1.f));
			ImGui::TextUnformatted(title.c_str());
			titlebar.showButtons();
			ImGui::PopStyleColor(3);
			ImGui::EndMainMenuBar();
		}
		ImGui::PopStyleVar();

		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.f));
		ImGui::Begin("MessageBox", nullptr, flags);
		ImGui::SetWindowPos(ImVec2(0.f, title_height)); 
		ImGui::SetWindowSize(client_size);

		ImGui::SetCursorPos(ImVec2(20, 20));
		ImGui::TextUnformatted(content.c_str());

		ImGui::SetCursorPosX(client_size.x - buttons_size.x - 10);
		ImGui::SetCursorPosY(client_size.y - buttons_size.y - 10);
		for (const auto& name : button_names) {
			if (ImGui::Button(name.c_str(), button_size)) {
				result = name;
				window.setVisible(false);
			}
			ImGui::SameLine();
		}

		ImGui::End();
		ImGui::PopStyleColor();

		ImGui::Render();
		ImGui::VK2D::Render(ImGui::GetDrawData());
		ImGui::EndFrame();

		window.display();

		std::this_thread::sleep_for(30ms);
	}

	ImGui::SetCurrentContext(prev_context);

	return result;
}