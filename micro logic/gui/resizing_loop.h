#pragma once

#include <vk2d/system/window.h>
#include "../util/stopwatch.h"


class ResizingLoop {
public:
	ResizingLoop();

protected:
	void initResizingLoop(const vk2d::Window& window);

public:
	virtual void loop();
	virtual void eventProc(const vk2d::Event& e) {};

	StopWatch stopwatch;
	float     delta_time;
	bool      resize_tab_hovered;
};