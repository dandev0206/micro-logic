#pragma once

#include <ostream>
#include <istream>
#include <iomanip>

class Serialrizable {
public:
	virtual void serialize(std::ostream& os) const = 0;
};

class Unserialrizable {
public:
	virtual void unserialize(std::istream& is) = 0;
};

template <class T>
void write_binary(std::ostream& os, const T& val) {
	os.write(reinterpret_cast<const char*>(&val), sizeof(T));
}

static void write_binary_str(std::ostream& os, const std::string& str, size_t size) {
	assert(str.size() <= size);

	os.write(str.c_str(), str.size());
	for (int i = 0; i < size - str.size(); ++i)
		os.write("\0", 1);
}

static void write_binary_str(std::ostream& os, const std::wstring& str, size_t size) {
	assert(str.size() <= 2 * size);

	os.write(reinterpret_cast<const char*>(str.c_str()), str.size());
	for (int i = 0; i < 2 * size - str.size(); ++i)
		os.write("\0", 1);
}

template <class T>
void read_binary(std::istream& is, T& val) {
	is.read(reinterpret_cast<char*>(&val), sizeof(T));
}

static void read_binary_str(std::istream& is, std::string& str, size_t size) {
	str.resize(size);
	is.read(str.data(), size);
	str.resize(strlen(str.data()));
}

static void read_binary_str(std::istream& is, std::wstring& str, size_t size) {
	str.resize(2 * size);
	is.read(reinterpret_cast<char*>(str.data()), 2 * size);
	str.resize(2 * wcslen(str.data()));
}