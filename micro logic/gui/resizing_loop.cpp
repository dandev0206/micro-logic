#include "resizing_loop.h"

#include <Windows.h>
#include <thread>
#include <imgui_internal.h>
#include "platform/platform_impl.h"

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

void ResizingLoop::setFrameLimit(int32_t fps)
{
	if (0 < fps)
		min_frame_time = 1.f / fps;
	else
		min_frame_time = 0.f;
}

void ResizingLoop::loop()
{
	using namespace std::chrono;

	auto now = clock_t::now();
	duration<float> work_time = now - last_frame;

	if (work_time.count() < min_frame_time) {
		duration<float> delay_ms(min_frame_time - work_time.count());
		duration<float> sleep_ms(0.5 * delay_ms.count());

		auto delay_begin = clock_t::now();
		
		while ((clock_t::now() - now) < sleep_ms)
			std::this_thread::sleep_for(1ms);
		while ((clock_t::now() - now) < delay_ms);
		
		auto delay_end = clock_t::now();

		duration<float> delay_real_ms = delay_end - delay_begin;

		delta_time = delay_real_ms.count();
	} else {
		delta_time = work_time.count();
	}

	last_frame = clock_t::now();
}
