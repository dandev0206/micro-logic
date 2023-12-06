#include "system_dialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <Shlwapi.h >
#include "../util/convert_string.h"

int32_t get_csidl(SystemFolders folder) {
	switch (folder) {
	case SystemFolders::Desktop:  return CSIDL_DESKTOP;
	case SystemFolders::Document: return CSIDL_MYDOCUMENTS;
	}
}

std::string GetSystemFolderPath(SystemFolders folder)
{
	wchar_t path[MAX_PATH + 1];
	int32_t csidl = get_csidl(folder);

	if (SUCCEEDED(SHGetFolderPath(NULL, csidl, NULL, 0, path)))
		return to_unicode(path);
	else
		return "";
}

class COMGuard {
public:
	~COMGuard() {
		CoUninitialize();
	}

	HRESULT init(LPVOID pvReserved, DWORD dwCoInit) {
		return CoInitializeEx(pvReserved, dwCoInit);
	}
};

class ReleaseGuard {
public:
	ReleaseGuard(IUnknown* obj) noexcept :
		obj(obj) {}

	~ReleaseGuard() {
		obj->Release(); 
	}

private:
	IUnknown* obj;
};

static HRESULT set_filter(IFileDialog* dialog, const FileDialogBase::filter_t& filters) {
	std::vector<COMDLG_FILTERSPEC> filter_specs(filters.size() + 1);
	std::vector<std::wstring> spec_names(filters.size() + 1);
	std::vector<std::wstring> specs(filters.size() + 1);

	if (!filters.empty()) {
		for (size_t i = 0; i < filters.size(); ++i) {
			auto& spec = specs[i];
			
			spec += L"*.";
			for (const char* ch = filters[i].spec; *ch; ++ch) {
				if (*ch == ',')
					spec += L";*.";
				else
					spec += *ch;
			}

			spec_names.emplace_back(to_wide(filters[i].name));

			filter_specs[i].pszName = spec_names[i].c_str();
			filter_specs[i].pszSpec = spec.c_str();
		}
	}

	filter_specs[filters.size()].pszName = L"All files";
	filter_specs[filters.size()].pszSpec = L"*.*";

	return dialog->SetFileTypes((UINT)filter_specs.size(), filter_specs.data());
}

HRESULT set_default_dir(IFileDialog* dialog, const wchar_t* default_dir) {
	if (!default_dir || !*default_dir) return S_OK;

	IShellItem* folder;
	HRESULT hr = SHCreateItemFromParsingName(default_dir, nullptr, IID_PPV_ARGS(&folder));

	if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE)) {
		return 0;
	} else if (SUCCEEDED(hr)) {
		// Failed to create ShellItem for setting the default path
		return hr;
	}

	hr = dialog->SetDefaultFolder(folder);

	folder->Release();

	return hr;
}

HRESULT set_default_name(IFileDialog* dialog, const wchar_t* default_name) {
	if (!default_name || !*default_name) return S_OK;

	return dialog->SetFileName(default_name);
}

HRESULT add_options(IFileDialog* dialog, FILEOPENDIALOGOPTIONS new_options) {
	FILEOPENDIALOGOPTIONS old_options;
	HRESULT hr;

	if (!SUCCEEDED(hr = dialog->GetOptions(&old_options))) return hr;
	if (!SUCCEEDED(dialog->SetOptions(old_options | new_options))) return hr;

	return S_OK;
}

FileDialogBase::filteritem_t::filteritem_t() :
	name(nullptr),
	spec(nullptr)
{}

FileDialogBase::filteritem_t::filteritem_t(const char* name, const char* spec) :
	name(name),
	spec(spec)
{}

FileDialogBase::FileDialogBase() :
	owner(nullptr)
{}

FolderOpenDialog::FolderOpenDialog() :
	owner(nullptr)
{}

DialogResult FolderOpenDialog::showDialog()
{
	COMGuard com_guard;
	if (FAILED(com_guard.init(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		return DialogResult::Error;

	IFileOpenDialog* dialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		NULL,
		CLSCTX_ALL,
		IID_IFileOpenDialog,
		reinterpret_cast<void**>(&dialog));

	if (FAILED(hr)) return DialogResult::Error;

	ReleaseGuard dialog_guard(dialog);

	if (FAILED(dialog->SetTitle(to_wide(title).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, to_wide(default_dir).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, to_wide(default_name).c_str()))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_FORCEFILESYSTEM))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_PICKFOLDERS))) return DialogResult::Error;

	hr = dialog->Show(owner ? (HWND)owner->getNativeHandle() : nullptr);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return DialogResult::Cancel;
	if (FAILED(hr)) return DialogResult::Error;

	IShellItem* item;
	if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

	ReleaseGuard item_guard(item);

	wchar_t* path_wstr;
	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_wstr))) return DialogResult::Error;
	
	path = to_unicode(path_wstr);
	
	CoTaskMemFree(path_wstr);

	return DialogResult::OK;
}

FileOpenDialog::FileOpenDialog(bool multi_selectable) :
	multi_selectable(multi_selectable) 
{}

DialogResult FileOpenDialog::showDialog()
{
	paths.clear();

	COMGuard com_guard;
	if (FAILED(com_guard.init(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		return DialogResult::Error;

	IFileOpenDialog* dialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileOpenDialog,
		NULL,
		CLSCTX_ALL,
		IID_IFileOpenDialog,
		reinterpret_cast<void**>(&dialog));

	if (FAILED(hr)) return DialogResult::Error;

	ReleaseGuard dialog_guard(dialog);

	if (FAILED(dialog->SetTitle(to_wide(title).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, to_wide(default_dir).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, to_wide(default_name).c_str()))) return DialogResult::Error;
	if (FAILED(set_filter(dialog, filters))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_FORCEFILESYSTEM))) return DialogResult::Error;
	if (multi_selectable) 
		if (FAILED(add_options(dialog, FOS_ALLOWMULTISELECT))) return DialogResult::Error;

	hr = dialog->Show(owner ? (HWND)owner->getNativeHandle() : nullptr);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return DialogResult::Cancel;
	if (FAILED(hr)) return DialogResult::Error;

	if (multi_selectable) {
		IShellItemArray* items;
		DWORD item_count, i;

		if (FAILED(dialog->GetResults(&items))) return DialogResult::Error;
		if (FAILED(items->GetCount(&item_count))) return DialogResult::Error;
		
		paths.reserve(item_count);
		for (i = 0; i < item_count; ++i) {
			IShellItem* item;
			wchar_t*    path_wstr;

			if (FAILED(items->GetItemAt(i, &item))) break;
			ReleaseGuard item_guard(item);

			if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_wstr))) break;
			paths.emplace_back(to_unicode(path_wstr));
			CoTaskMemFree(path_wstr);
		}

		if (i != item_count) {
			paths.clear();
			return DialogResult::Error;
		}

		return DialogResult::OK;
	} else {
		IShellItem* item;
		wchar_t*    path_wstr;

		if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

		ReleaseGuard item_guard(item);

		paths.reserve(1);
		if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_wstr))) {
			paths.clear();
			return DialogResult::Error;
		}
		paths.emplace_back(to_unicode(path_wstr));
		CoTaskMemFree(path_wstr);
		
		return DialogResult::OK;
	}
}

FileSaveDialog::FileSaveDialog() :
	path(nullptr) 
{}

DialogResult FileSaveDialog::showDialog()
{
	COMGuard com_guard;
	if (FAILED(com_guard.init(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)))
		return DialogResult::Error;

	IFileSaveDialog* dialog;
	HRESULT hr = CoCreateInstance(
		CLSID_FileSaveDialog,
		NULL,
		CLSCTX_ALL,
		IID_IFileSaveDialog,
		reinterpret_cast<void**>(&dialog));

	if (FAILED(hr)) return DialogResult::Error;

	ReleaseGuard dialog_guard(dialog);

	if (FAILED(dialog->SetTitle(to_wide(title).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, to_wide(default_dir).c_str()))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, to_wide(default_name).c_str()))) return DialogResult::Error;
	if (FAILED(set_filter(dialog, filters))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_FORCEFILESYSTEM))) return DialogResult::Error;

	hr = dialog->Show(owner ? (HWND)owner->getNativeHandle() : nullptr);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return DialogResult::Cancel;
	if (FAILED(hr)) return DialogResult::Error;

	IShellItem* item;
	if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

	ReleaseGuard item_guard(item);

	wchar_t* path_wstr;
	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path_wstr))) return DialogResult::Error;
	path = to_unicode(path_wstr);
	CoTaskMemFree(path_wstr);

	return DialogResult::OK;
}