#include "../../include/vk2d/system/file_dialog.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shobjidl.h>
#include <Shlwapi.h >

VK2D_BEGIN

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
	std::vector<std::wstring> specs(filters.size() + 1);

	if (!filters.empty()) {
		for (size_t i = 0; i < filters.size(); ++i) {
			auto& spec = specs[i];
			
			spec += L"*.";
			for (const wchar_t* ch = filters[i].spec; *ch; ++ch) {
				if (*ch == L',')
					spec += L";*.";
				else
					spec += *ch;
			}

			filter_specs[i].pszName = filters[i].name;
			filter_specs[i].pszSpec = spec.c_str();
		}
	}

	filter_specs[filters.size()].pszName = L"All files";
	filter_specs[filters.size()].pszSpec = L"*.*";

	return dialog->SetFileTypes(filter_specs.size(), filter_specs.data());
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

//class CDialogEventHandler : public IFileDialogEvents {
//public:
//	CDialogEventHandler() : 
//		_cRef(1) {};
//
//	IFACEMETHODIMP OnFileOk(IFileDialog* dialog) {
//		return S_OK;
//	};
//
//	IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) {
//		static const QITAB qit[] = {
//			QITABENT(CDialogEventHandler, IFileDialogEvents),
//			{ 0 },
//		};
//		return QISearch(this, qit, riid, ppv);
//	}
//
//	IFACEMETHODIMP_(ULONG) AddRef() {
//		return InterlockedIncrement(&_cRef);
//	}
//
//	IFACEMETHODIMP_(ULONG) Release() {
//		long cRef = InterlockedDecrement(&_cRef);
//		if (!cRef)
//			delete this;
//		return cRef;
//	}
//
//	IFACEMETHODIMP OnFolderChange(IFileDialog*) { return S_OK; }
//	IFACEMETHODIMP OnFolderChanging(IFileDialog*, IShellItem*) { return S_OK; }
//	IFACEMETHODIMP OnHelp(IFileDialog*) { return S_OK; }
//	IFACEMETHODIMP OnSelectionChange(IFileDialog*) { return S_OK; }
//	IFACEMETHODIMP OnShareViolation(IFileDialog*, IShellItem*, FDE_SHAREVIOLATION_RESPONSE*) { return S_OK; }
//	IFACEMETHODIMP OnTypeChange(IFileDialog*) { return S_OK; }
//	IFACEMETHODIMP OnOverwrite(IFileDialog*, IShellItem*, FDE_OVERWRITE_RESPONSE*) { return S_OK; }
//
//private:
//	~CDialogEventHandler() ;
//	long _cRef;
//};
//
//HRESULT CDialogEventHandler_CreateInstance(REFIID riid, void** ppv)
//{
//	*ppv = NULL;
//	CDialogEventHandler* pDialogEventHandler = new (std::nothrow) CDialogEventHandler();
//	HRESULT hr = pDialogEventHandler ? S_OK : E_OUTOFMEMORY;
//	if (SUCCEEDED(hr))
//	{
//		hr = pDialogEventHandler->QueryInterface(riid, ppv);
//		pDialogEventHandler->Release();
//	}
//	return hr;
//}

FileDialogBase::FileDialogBase() :
	owner(nullptr),
	title(nullptr),
	default_dir(nullptr),
	default_name(nullptr),
	filters() 
{}

FolderOpenDialog::FolderOpenDialog() :
	owner(nullptr),
	title(nullptr),
	default_dir(nullptr),
	default_name(nullptr),
	dir(nullptr) 
{}

FolderOpenDialog::~FolderOpenDialog()
{
	CoTaskMemFree(dir);
}

DialogResult FolderOpenDialog::showDialog()
{
	CoTaskMemFree(std::exchange(dir, nullptr));

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

	if (FAILED(dialog->SetTitle(title))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, default_dir))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, default_name))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_FORCEFILESYSTEM))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_PICKFOLDERS))) return DialogResult::Error;

	hr = dialog->Show(owner ? (HWND)owner->getNativeHandle() : nullptr);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return DialogResult::Cancel;
	if (FAILED(hr)) return DialogResult::Error;

	IShellItem* item;
	if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

	ReleaseGuard item_guard(item);

	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &dir))) return DialogResult::Error;

	return DialogResult::OK;
}

const wchar_t* FolderOpenDialog::getResultDir() const
{
	return dir;
}

FileOpenDialog::FileOpenDialog(bool multi_selectable) :
	multi_selectable(multi_selectable) 
{}

FileOpenDialog::~FileOpenDialog()
{
	for (wchar_t* path : paths)
		CoTaskMemFree(path);
}

DialogResult FileOpenDialog::showDialog()
{
	for (wchar_t* path : paths)
		CoTaskMemFree(path);
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

	if (FAILED(dialog->SetTitle(title))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, default_dir))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, default_name))) return DialogResult::Error;
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
		
		paths.resize(item_count, nullptr);
		for (i = 0; i < item_count; ++i) {
			IShellItem* item;

			if (FAILED(items->GetItemAt(i, &item))) break;
			ReleaseGuard item_guard(item);

			if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &paths[i]))) break;
		}

		if (i != item_count) {
			while (i > 0) CoTaskMemFree(paths[--i]);

			return DialogResult::Error;
		}

		return DialogResult::OK;
	} else {
		IShellItem* item;

		if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

		ReleaseGuard item_guard(item);

		paths.resize(1);
		if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &paths[0]))) {
			paths.clear();
			return DialogResult::Error;
		}
		
		return DialogResult::OK;
	}
}

const std::vector<wchar_t*> FileOpenDialog::getResultPaths() const
{
	return paths;
}

FileSaveDialog::FileSaveDialog() :
	path(nullptr) 
{}

FileSaveDialog::~FileSaveDialog()
{
	CoTaskMemFree(path);
}

DialogResult FileSaveDialog::showDialog()
{
	CoTaskMemFree(std::exchange(path, nullptr));

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

	if (FAILED(dialog->SetTitle(title))) return DialogResult::Error;
	if (FAILED(set_default_dir(dialog, default_dir))) return DialogResult::Error;
	if (FAILED(set_default_name(dialog, default_name))) return DialogResult::Error;
	if (FAILED(set_filter(dialog, filters))) return DialogResult::Error;
	if (FAILED(add_options(dialog, FOS_FORCEFILESYSTEM))) return DialogResult::Error;

	hr = dialog->Show(owner ? (HWND)owner->getNativeHandle() : nullptr);

	if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED)) return DialogResult::Cancel;
	if (FAILED(hr)) return DialogResult::Error;

	IShellItem* item;
	if (FAILED(dialog->GetResult(&item))) return DialogResult::Error;

	ReleaseGuard item_guard(item);

	if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) return DialogResult::Error;

	return DialogResult::OK;
}

const wchar_t* FileSaveDialog::getResultPath() const
{
	return path;
}

VK2D_END
