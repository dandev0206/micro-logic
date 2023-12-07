#include "../include/vk2d/graphics/texture_view.h"

#include "../include/vk2d/core/vk2d_context_impl.h"
#include "../include/vk2d/graphics/texture.h"
#include "../include/vk2d/graphics/render_target.h"
#include "../include/vk2d/graphics/render_states.h"

VK2D_BEGIN

TextureView::TextureView() :
	texture(nullptr), 
	region() {}

TextureView::TextureView(const Texture& texture) :
	texture(&texture),
	region({0, 0}, texture.size()) {}

TextureView::TextureView(const Texture& texture, const uRect& region) :
	texture(&texture),
	region(region) {}

const Texture& TextureView::getTexture() const
{
	return *texture;
}

TextureView& TextureView::setTexture(const Texture& texture)
{
	this->texture = &texture;
	return *this;
}

uRect TextureView::getRegion() const
{
	return region;
}

TextureView& TextureView::setRegion(const uRect& region)
{
	this->region = region;
	return *this;
}

Color TextureView::getColor() const
{
	return color;
}

TextureView& TextureView::setColor(const Color& color)
{
	this->color = color;
	return *this;
}

bool TextureView::empty() const 
{
	return !texture;
}

void TextureView::draw(RenderTarget& target, RenderStates& states) const
{
	VK2D_ASSERT(texture && "TextureView no texture");

	auto& inst = VK2DContext::get();

	auto cmd_buffer = target.getCommandBuffer();

	const Pipeline* pipeline = nullptr;

	// bind pipeline
	if (states.options.pipeline.has_value())
		pipeline = states.options.pipeline.value();
	else
		pipeline = &inst.basic_pipelines[2];

	cmd_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline->pipeline);

	states.transform *= getTransform();
	states.bindTVS(cmd_buffer, *pipeline->pipeline_layout);

	vec2 texture_size = texture->size();

	vec2 p0 = (vec2)region.getPosition();
	vec2 p1 = (vec2)(region.getPosition() + region.getSize());
	vec2 p2 = (vec2)region.getPosition() / texture_size;
	vec2 p3 = (vec2)(region.getPosition() + region.getSize()) / texture_size;

	struct PC {
		vec4 color;
		vec2 vertices_uvs[8];
	};

	PC pc = {
		Color(Colors::White).to_vec4(),
		{
			p0,
			{ p1.x, p0.y },
			p1,
			{ p0.x, p1.y },
			p2,
			{ p3.x, p2.y },
			p3,
			{ p2.x, p3.y }
		}
	};

	cmd_buffer.pushConstants(
		*pipeline->pipeline_layout,
		vk::ShaderStageFlagBits::eVertex,
		sizeof(Transform),
		sizeof(PC), &pc);

	// bind descriptor set
	auto desc_set   = texture->getDescriptorSet();
	cmd_buffer.bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		*pipeline->pipeline_layout,
		0, 
		1, &desc_set, 
		0, nullptr);

	cmd_buffer.draw(4, 1, 0, 0);
}

VK2D_END