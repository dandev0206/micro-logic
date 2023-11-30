#pragma once

#include "../custom_titlebar.h"
#include "../../main_window.h"

void InjectTitleBar(const vk2d::Window& window, const CustomTitleBar& titlebar);
void InjectWndProc(MainWindow* main_window);