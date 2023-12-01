#pragma once

#include <imgui.h>
#include "custom_titlebar.h"

namespace priv {
	class DialogImpl;
}

class Dialog {
	friend class priv::DialogImpl;

public:
	static void pushImpls(size_t count);
	static void destroyImpl();

	Dialog(const std::string& title, bool resizable);
	~Dialog();

	std::string         title;
	const vk2d::Window* owner;

private:
	priv::DialogImpl* impl;

protected:
	void switchContext() const;
	void beginShowDialog(ImVec2 window_size) const;
	void endShowDialog() const;
	void beginDialogWindow(const char* name, bool* p_open = nullptr, ImGuiWindowFlags flags = 0) const;
	void endDialogWindow() const; 
	bool updateDialog() const;
	void renderDialog() const;

	float getDeltaTime() const;

	virtual void dialogLoop() const {}
	virtual void dialogEventProc(const vk2d::Event& e) {}

	vk2d::Window&   window;
	CustomTitleBar& titlebar;
	
	mutable ImVec2 window_size;
	mutable ImVec2 client_size;
	const float    title_height;
};