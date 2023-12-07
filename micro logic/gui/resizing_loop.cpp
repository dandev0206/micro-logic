#include "resizing_loop.h"

#include "../platform/platform_impl.h"
#include <imgui_internal.h>
#include <thread>

ResizingLoop::ResizingLoop() :
	delta_time(1.f / 60.f),
	resize_tab_hovered(false),
	last_frame(clock_t::now()),
	min_frame_time(0.f)
{}

void ResizingLoop::initResizingLoop(const vk2d::Window& window)
{
	InjectResizingLoop(window, this);
}

void ResizingLoop::loop()
{
	using namespace std::chrono;

	auto now = clock_t::now();
	duration<float> work_time_s = now - last_frame;

	if (work_time_s.count() < min_frame_time) {
		auto delay_begin = clock_t::now();
		
		SleepMS(1000 * (min_frame_time - work_time_s.count()));
		
		auto delay_end = clock_t::now();

		duration<float> frame_real_s = delay_end - delay_begin;

		delta_time = (work_time_s + frame_real_s).count();
	} else {
		delta_time = work_time_s.count();
	}

	last_frame = clock_t::now();
}

void ResizingLoop::setFrameLimit(int32_t fps)
{
	if (0 < fps)
		min_frame_time = 1.f / fps;
	else
		min_frame_time = 0.f;
}