#pragma once

#include "../gui/custom_titlebar.h"
#include "../gui/resizing_loop.h"

void CreateNewProcess();
void SleepMS(uint32_t milliseconds);
void InjectTitleBar(CustomTitleBar* titlebar);
void InjectResizingLoop(const vk2d::Window& window, ResizingLoop* resizing_loop);