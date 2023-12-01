#pragma once

#include <string>
#include <assert.h>

static std::string to_unicode(const std::wstring& wstr) {
	std::string str;
	size_t size;
	str.resize(wstr.length() * 2);
	auto res = wcstombs_s(&size, &str[0], str.size(), wstr.c_str(), _TRUNCATE);
	assert(!res);
	str.resize(size - 1);

	return str;
}

static std::wstring to_wide(const std::string& str) {
	std::wstring wstr;
	size_t size;
	wstr.resize(str.length() + 1);
	auto res = mbstowcs_s(&size, &wstr[0], wstr.size(), str.c_str(), _TRUNCATE);
	assert(!res);
	wstr.resize(size - 1);
	return wstr;
}