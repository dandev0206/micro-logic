#pragma once

#include "drawable.h"
#include "color.hpp"
#include "../transformable.hpp"
#include "../rect.hpp"

VK2D_BEGIN

class Texture;

class TextureView : public Drawable, public Transformable {
public:
	TextureView();
	TextureView(const TextureView& rhs) = default;
	TextureView(TextureView&& rhs) VK2D_NOTHROW = default;
	TextureView(const Texture& texture);
	TextureView(const Texture& texture, const uRect& region);

	TextureView& operator=(const TextureView& rhs) = default;
	TextureView& operator=(TextureView&& rhs) VK2D_NOTHROW = default;

	const Texture& getTexture() const;
	TextureView& setTexture(const Texture& texture);

	uRect getRegion() const;
	TextureView& setRegion(const uRect& region);

	Color getColor() const;
	TextureView& setColor(const Color& color);

	bool empty() const;

private:
	void draw(RenderTarget& target, RenderStates& states) const override;

private:
	const Texture* texture;
	uRect region;
	Color color;
};

VK2D_END