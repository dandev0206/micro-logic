#include "../../include/vk2d/system/message_box.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <CommCtrl.h>

#undef MessageBox

VK2D_BEGIN

static TASKDIALOG_COMMON_BUTTON_FLAGS get_dialog_buttons(MessageBox::Button buttons)
{
	TASKDIALOG_COMMON_BUTTON_FLAGS result = 0;
	if (buttons & MessageBox::OK)     result |= TDCBF_OK_BUTTON;
	if (buttons & MessageBox::Yes)    result |= TDCBF_YES_BUTTON;
	if (buttons & MessageBox::No)     result |= TDCBF_NO_BUTTON;
	if (buttons & MessageBox::Cancel) result |= TDCBF_CANCEL_BUTTON;
	if (buttons & MessageBox::Retry)  result |= TDCBF_RETRY_BUTTON;
	if (buttons & MessageBox::Close)  result |= TDCBF_CLOSE_BUTTON;

	return result;
}

static PCWSTR get_dialog_icon(MessageBox::Icon icon) {
	switch (icon) {
	case MessageBox::Icon::None:        return NULL;
	case MessageBox::Icon::Error:       return TD_ERROR_ICON;
	case MessageBox::Icon::Information: return TD_INFORMATION_ICON;
	case MessageBox::Icon::Sheild:      return TD_SHIELD_ICON;
	case MessageBox::Icon::Warning:     return TD_WARNING_ICON;
	default:                            return NULL;
	}
}

static DialogResult get_dialog_result(int result) 
{
	switch (result) {
	case IDOK:     return DialogResult::OK;
	case IDCANCEL: return DialogResult::Cancel;
	case IDYES:	   return DialogResult::Yes;
	case IDNO:	   return DialogResult::No;
	case IDRETRY:  return DialogResult::Retry;
	default:       return DialogResult::Error;
	}
}

MessageBox::MessageBox() :
	owner(nullptr),
	buttons(Button::OK),
	icon(Icon::None)
{}

DialogResult MessageBox::showDialog()
{
	int result;

	auto hr = TaskDialog(
		(HWND)(owner ? owner->getNativeHandle() : NULL),
		GetModuleHandle(NULL),
		title.c_str(),
		msg.c_str(),
		additional_msg.c_str(),
		get_dialog_buttons(buttons),
		get_dialog_icon(icon),
		&result);

	if (FAILED(hr)) 
		return DialogResult::Error;

	return get_dialog_result(result);
}

VK2D_END