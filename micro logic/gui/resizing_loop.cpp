#include "resizing_loop.h"
#include "platform/platform_impl.h"

ResizingLoop::ResizingLoop() :
	delta_time(1.f / 60.f),
	resize_tab_hovered(false)
{}

void ResizingLoop::initResizingLoop(const vk2d::Window& window)
{
	InjectResizingLoop(window, this);
}

void ResizingLoop::loop()
{
	delta_time = stopwatch.in_s(6);
	stopwatch.start();
}
