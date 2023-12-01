#pragma once

#include <string>

enum class SystemFolders {
	Desktop,
	Document,
};

std::string GetSystemFolderPath(SystemFolders folder);