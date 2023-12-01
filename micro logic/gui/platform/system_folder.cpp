#include "system_folder.h"

#include <windows.h>
#include <shlobj.h>
#include "../../util/convert_string.h"

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
