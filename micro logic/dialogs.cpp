#include "dialogs.h"

#include <imgui_internal.h>
#include <thread>
#include "gui/imgui_impl_vk2d.h"

static std::vector<std::string> split_str(const std::string& str, char ch)
{
	std::vector<std::string> result;

	size_t first = 0;
	size_t last = str.find(ch);

	while (last != std::string::npos) {
		result.push_back(str.substr(first, last - first));
		first = last + 1;
		last = str.find(ch, first);
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
		auto size = ImVec2(text_size.x + 2.f * style.FramePadding.x, text_size.y + 2.f * style.FramePadding.y);

		size = ImGui::CalcItemSize(ImVec2(100, 40), size.x, size.y);

		buttons_size.x += size.x + style.ItemSpacing.x;
		buttons_size.y = size.y;
	}
	buttons_size.x -= style.ItemSpacing.x;

	return buttons_size;
}

MessageBox::MessageBox() :
	Dialog(false)
{}

std::string MessageBox::showDialog()
{
	using namespace std::chrono;

	static const ImVec2 button_size(100, 40);
	static const auto flags =
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDecoration;

	Dialog::beginShowDialog();

	auto& style = ImGui::GetStyle();

	auto button_names = split_str(buttons, ';');
	auto buttons_size = calc_buttons_size(button_names);
	auto result = std::string("Error");

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
	titlebar.setButtonStyle(false, false, true);
	titlebar.setCaptionRect({ 0.f, 0.f, window_size.x, title_height });
	window.setVisible(true);

	while (window.isVisible()) {
		vk2d::Event e;
		while (window.pollEvent(e)) {
			if (e.type == vk2d::Event::Close) {
				result = std::string("Close");
				window.setVisible(false);
			}
			ImGui::VK2D::ProcessEvent(window, e);
		}

		ImGui::VK2D::Update(window, getDeltaTime());

		if (titlebar.beginTitleBar()) {
			ImGui::TextUnformatted(title.c_str());
			titlebar.endTitleBar();
		}

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

	Dialog::endShowDialog();

	return result;
}