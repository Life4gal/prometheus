// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/glyph.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	auto GlyphUploadQueue::push(GlyphInfo& info, Texture::data_type&& data) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data != nullptr);

		list_.emplace_back(memory::ref(info), std::move(data));
	}

	auto GlyphUploadQueue::upload(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			list_,
			[&context](element_type& element) noexcept -> void
			{
				auto& info = element.info.get();

				const auto size = info.rect.size();

				auto [writer, texture_atlas_id] = context.write(size);
				const auto& texture = context.select_texture(texture_atlas_id);
				const auto uv_scale = texture.uv_scale;

				const Texture::data_view_type data_view{element.data.get(), static_cast<std::size_t>(size.width) * size.height};
				writer.fill(data_view);

				info.texture_atlas_id = texture_atlas_id;
				info.uv.point = writer.position().to<point_type>() * uv_scale;
				info.uv.extent = size.to<extent_type>() * uv_scale;
			}
		);
		list_.clear();
	}

	auto GlyphContext::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo&
	{
		if (const auto& it = cached_glyphs_.find(key); it != cached_glyphs_.end())
		{
			return it->second;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(*glyph_parser != nullptr);

		auto [rect, advance_x, visible, colored, data] = (*glyph_parser)->parse(key);

		GlyphInfo info{};
		info.rect = rect;
		info.advance_x = advance_x;
		info.visible = visible;
		info.colored = colored;

		auto [it, inserted] = cached_glyphs_.emplace(key, info);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(inserted);

		auto& inserted_info = it->second;

		if (visible)
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data != nullptr);

			upload_queue_.push(inserted_info, std::move(data));
		}

		return inserted_info;
	}

	auto GlyphContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo&
	{
		return this->glyph_of({codepoint, size, flag});
	}

	auto GlyphContext::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<std::reference_wrapper<const GlyphInfo>>
	{
		std::vector<std::reference_wrapper<const GlyphInfo>> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto& info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(std::cref(info));
			}
		);

		return infos;
	}

	auto GlyphContext::upload_all_glyph(TextureContext& context) noexcept -> void
	{
		upload_queue_.upload(context);
	}
}
