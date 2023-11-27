#include "../../include/vk2d/system/clipboard.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

VK2D_BEGIN

bool Clipboard::available()
{
	return IsClipboardFormatAvailable(CF_TEXT);
}


std::string Clipboard::getString() 
{
	if (!IsClipboardFormatAvailable(CF_TEXT))
		VK2D_ERROR("Failed to get the clipboard data in text format.");

	if (!OpenClipboard(NULL))
		VK2D_ERROR("Failed to open the Win32 clipboard.");

	HANDLE clipboard_handle = GetClipboardData(CF_TEXT);

	if (!clipboard_handle)
		VK2D_ERROR("Failed to get Win32 handle for clipboard content.");

	std::string str = static_cast<const char*>(GlobalLock(clipboard_handle));
	GlobalUnlock(clipboard_handle);
	CloseClipboard();

	return str;
}

void Clipboard::setString(const std::string& str)
{
	if (!OpenClipboard(NULL))
		VK2D_ERROR("Failed to open the Win32 clipboard.");

	if (!EmptyClipboard())
		VK2D_ERROR("Failed to open the Win32 clipboard.");

	size_t string_size   = str.size() + 1;
	HANDLE string_handle = GlobalAlloc(GMEM_MOVEABLE, string_size);

	if (string_handle) {
		memcpy(GlobalLock(string_handle), str.c_str(), string_size);
		GlobalUnlock(string_handle);
		SetClipboardData(CF_TEXT, string_handle);
	}

	CloseClipboard();
}

VK2D_END