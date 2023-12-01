#pragma once

#include <chrono>
#include <cmath>

class StopWatch {
	using clock      = std::chrono::high_resolution_clock;
	using time_point = clock::time_point;

public:
	inline StopWatch()
		: ticking(true), begin(clock::now()) {}

	inline void start() {
		begin = clock::now();
		ticking = true;
	}

	inline void stop() {
		end = clock::now();
		ticking = false;
	}

	inline bool isTicking() { 
		return ticking; 
	}

private:
	static constexpr double pow10(uint32_t exp) {
		constexpr double table[] = {
			1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11 };
		return exp > 11 ? 1e12 : table[exp];
	}

public:
	inline int64_t in_tick() {
		if (ticking)
			return (clock::now() - begin).count();
		else
			return (end - begin).count();
	}

	inline double in_hour(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round((tick / 36e11) * pow) / pow;
	}

	inline double in_min(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round((tick / 6e10) * pow) / pow;
	}

	inline double in_s(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round((tick / 1e9) * pow) / pow;
	}

	inline double in_ms(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round((tick / 1e6) * pow) / pow;
	}

	inline double in_us(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round((tick / 1e3) * pow) / pow;
	}

	inline double in_ns(uint32_t precision = 3) {
		int64_t tick = in_tick();
		auto pow = pow10(precision);
		return std::round(tick * pow) / pow;
	}

private:
	bool       ticking;
	time_point begin;
	time_point end;
};
