#pragma once

#include "../transform.hpp"
#include "pipeline.h"
#include "texture.h"
#include "buffer.h"

#include <utility>
#include <optional>

VK2D_BEGIN

struct RenderOptions {
	RenderOptions() = default;

	RenderOptions(const Pipeline& pipeline) :
		pipeline(&pipeline) {}

	RenderOptions(const Texture& texture) :
		texture(&texture) {}

	RenderOptions(const Transform& transform) :
		transform(transform) {}

	void clear() {
		buffer.reset();
		pipeline.reset();
		texture.reset();
		transform.reset();
		viewport.reset();
		scissor.reset();
	}

	std::optional<const Buffer*>   buffer;
	std::optional<const Pipeline*> pipeline;
	std::optional<const Texture*>  texture;
	std::optional<Transform>       transform;
	std::optional<uRect>           viewport;
	std::optional<uRect>           scissor;
};

VK2D_END