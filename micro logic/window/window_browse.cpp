#include "window_browse.h"

#include <imgui.h>
#include "../main_window.h"

Window_Browse::Window_Browse()
{
}

void Window_Browse::showUI()
{
	DockingWindow::beginDockWindow("Browse", &show);

	DockingWindow::endDockWindow();
}
