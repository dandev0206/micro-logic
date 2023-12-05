#pragma once

#include <vk2d/system/window.h>
#include <chrono>

class ResizingLoop {
public:
	ResizingLoop();

protected:
	void initResizingLoop(const vk2d::Window& window);

public:
	virtual void loop();
	virtual void eventProc(const vk2d::Event& e) {};
	
	void setFrameLimit(int32_t fps);

	float delta_time;
	bool  resize_tab_hovered;

private:
	using clock_t     = std::chrono::steady_clock;
	using timepoint_t = clock_t::time_point;

	timepoint_t last_frame;

	float min_frame_time;
};