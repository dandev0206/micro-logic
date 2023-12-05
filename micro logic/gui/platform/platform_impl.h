#pragma once

#include "../custom_titlebar.h"
#include "../resizing_loop.h"

void CreateNewProcess();
void SleepMS(uint32_t milliseconds);
void InjectTitleBar(CustomTitleBar* titlebar);
void InjectResizingLoop(const vk2d::Window& window, ResizingLoop* resizing_loop);