#pragma once

#include <vk2d/system/window.h>
#include <vk2d/graphics/texture_view.h>
#include "gui/dialog.h"

class MessageBox : public Dialog {
public:
	MessageBox();

	std::string showDialog() const;

	std::string       buttons; // separated by ;
	std::string       content;
	vk2d::TextureView icon;
};

class ProjectCloseDialog : public Dialog {
public:
	ProjectCloseDialog();

	std::string showDialog() const;
};

class ProjectSaveDialog : public Dialog {
public:
	ProjectSaveDialog();

	std::string showDialog() const;

	mutable std::string project_dir;
	mutable std::string project_name;
	bool                save_as;
private:
	void dialogLoop() const override;
	
	void updateProjectDir() const;

	mutable std::string project_location;
	mutable std::string result;
	mutable bool        create_subdirectory;
};