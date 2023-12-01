#pragma once

#include "custom_titlebar.h"

namespace priv {
	class DialogImpl;
}

class Dialog {
	friend class priv::DialogImpl;

public:
	static void pushImpls(size_t count);
	static void destroyImpl();

	Dialog(bool resizable);
	~Dialog();

	const vk2d::Window* owner;

private:
	priv::DialogImpl* impl;

protected:
	void beginShowDialog() const;
	void endShowDialog() const;

	float getDeltaTime() const;

	virtual void dialogLoop() {}
	virtual void dialogEventProc(const vk2d::Event& e) {}

	vk2d::Window&   window;
	CustomTitleBar& titlebar;
};