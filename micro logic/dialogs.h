#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture_view.h>
#include "gui/dialog.h"

class MessageBox : public Dialog {
public:
	MessageBox();

	std::string showDialog() const;

	std::string       content;
	std::string       buttons; // separated by ;
	vk2d::TextureView icon;
};

class ListMessageBox : public Dialog {
public:
	ListMessageBox();

	std::string showDialog() const;

	std::string              content;
	std::vector<std::string> list;
	std::string              buttons; // separated by ;
	vk2d::TextureView        icon;
};

class ProjectSaveDialog : public Dialog {
public:
	ProjectSaveDialog();

	std::string showDialog();

	bool save_as;

	std::string project_dir;
	std::string project_name;
};

class SchematicSheetNameDialog : public Dialog {
public:
	SchematicSheetNameDialog();

	std::string showDialog();

	std::string content;
	std::string parent_dir;
	std::string sheet_name;
	std::string buttons; // separated by ;
	
	std::string sheet_path;
};