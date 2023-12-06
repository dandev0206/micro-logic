#pragma once

#include <vk2d/system/window.h>
#include <string>

enum class SystemFolders {
	Desktop,
	Document,
};

std::string GetSystemFolderPath(SystemFolders folder);

enum class DialogResult {
	OK,
	Cancel,
	Yes,
	No,
	Retry,
	Error
};

class FileDialogBase abstract {
public:
	struct filteritem_t {
		filteritem_t();
		filteritem_t(const char* name, const char* spec);

		const char* name;
		const char* spec;
	};

	using filter_t = std::vector<filteritem_t>;

	FileDialogBase();

	const vk2d::Window* owner;
	std::string         title;
	std::string         default_dir;
	std::string         default_name;
	filter_t            filters;
};

class FolderOpenDialog {
public:
	FolderOpenDialog();

	DialogResult showDialog();

	const vk2d::Window* owner;
	std::string         title;
	std::string         default_dir;
	std::string         default_name;

	std::string path;
};

class FileOpenDialog : public FileDialogBase {
public:
	FileOpenDialog(bool multi_selectable = false);

	DialogResult showDialog();

	bool multi_selectable;

	std::vector<std::string> paths;
};

class FileSaveDialog : public FileDialogBase {
public:
	FileSaveDialog();

	DialogResult showDialog();

	std::string path;
};
