#pragma once

#include "../custom_titlebar.h"
#include "../resizing_loop.h"

void InjectTitleBar(CustomTitleBar* titlebar);
void InjectResizingLoop(const vk2d::Window& window, ResizingLoop* resizing_loop);