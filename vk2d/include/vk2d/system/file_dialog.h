#pragma once

#include "dialog.h"
#include "window.h"

VK2D_BEGIN

class FileDialogBase abstract {
public:
	struct filteritem_t {
		filteritem_t();
		filteritem_t(const wchar_t* name, const wchar_t* spec);

		const wchar_t* name;
		const wchar_t* spec;
	};

	using filter_t = std::vector<filteritem_t>;

	FileDialogBase();

	const Window*  owner;
	const wchar_t* title;
	const wchar_t* default_dir;
	const wchar_t* default_name;
	filter_t       filters;
};

class FolderOpenDialog {
public:
	FolderOpenDialog();
	~FolderOpenDialog();

	DialogResult showDialog();

	const wchar_t* getResultDir() const;

	const Window*  owner;
	const wchar_t* title;
	const wchar_t* default_dir;
	const wchar_t* default_name;

	bool check_empty;

private:
	wchar_t* dir;
};

class FileOpenDialog : public FileDialogBase {
public:
	FileOpenDialog(bool multi_selectable = false);
	~FileOpenDialog();

	DialogResult showDialog();

	const std::vector<wchar_t*> getResultPaths() const;

	bool multi_selectable;

private:
	std::vector<wchar_t*> paths;
};

class FileSaveDialog : public FileDialogBase {
public:
	FileSaveDialog();
	~FileSaveDialog();

	DialogResult showDialog();

	const wchar_t* getResultPath() const;

private:
	wchar_t* path;
};

VK2D_END