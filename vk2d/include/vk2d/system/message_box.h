#pragma once

#include "dialog.h"
#include "window.h"

VK2D_BEGIN

class MessageBox {
public:
	enum Button {
		OK            = 1 << 0,
		Yes           = 1 << 1,
		No            = 1 << 2,
		Cancel        = 1 << 3,
		Retry         = 1 << 4,
		Close         = 1 << 5,
		OK_Cacnel     = OK | Cancel,
		Retry_Cancel  = Retry | Cancel,
		Yes_No        = Yes | No,
		Yes_No_Cancel = Yes | No | Cancel
	};

	enum class Icon {
		None,
		Error,
		Information,
		Sheild,
		Warning,
	};

	MessageBox();

	DialogResult showDialog();

	const Window* owner;
	std::wstring  title;
	std::wstring  msg;
	std::wstring  additional_msg;
	Button        buttons;
	Icon          icon;
};

VK2D_END