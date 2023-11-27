#ifdef _WIN32
#include "platform/windows_cursor_impl.hpp"

VK2D_BEGIN

HCURSOR CursorImpl::last_cursor = LoadCursor(NULL, IDC_ARROW);
bool    CursorImpl::cursor_visible = true;

VK2D_END
#endif

VK2D_BEGIN

void Cursor::setVisible(bool visible)
{
	CursorImpl::setVisible(visible);
}

void Cursor::setCursor(const Cursor& cursor) 
{
	CursorImpl::setCursor(cursor);
}

void Cursor::updateCursor()
{
	CursorImpl::updateCursor();
}

Cursor::Cursor() : 
	impl(new CursorImpl()) {}

Cursor::~Cursor()
{
	impl->destroy();
	delete impl;
}

void Cursor::loadFromPixels(const uint8_t* pixels, const uvec2& size, const uvec2& hotspot)
{
	impl->destroy();
	impl->loadFromPixels(pixels, size, hotspot);
}

void Cursor::loadFromSystem(Cursor::Type type)
{
	impl->destroy();
	impl->loadFromSystem(type);
}

void Cursor::destroy()
{
	impl->destroy();
}

bool Cursor::valid() const
{
	return impl->valid();
}

VK2D_END