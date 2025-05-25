// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/font.hpp>

#include <fstream>

#include <gfx_new/internal/texture.hpp>
#include <gfx_new/internal/context.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx_new
{
	auto GlyphUploadQueue::push(GlyphInfo& info, GlyphParser::GlyphDescriptor&& descriptor) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(descriptor.valid());

		list_.emplace_back(memory::ref(info), std::move(descriptor));
	}

	auto GlyphUploadQueue::upload(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			list_,
			[&context](element_type& element) noexcept -> void
			{
				auto& info = element.info.get();
				const auto& descriptor = element.descriptor;

				const auto size = descriptor.rect.size();
				const Texture::data_view_type data{descriptor.data.get(), static_cast<std::size_t>(size.width) * size.height};

				// FIXME-OPT:
				// Do we need to specify a texture for the upload?
				const auto [texture_atlas_id, uv] = context.write(data, size);
				info.texture_atlas_id = texture_atlas_id;
				info.uv = uv;
			}
		);
		list_.clear();
	}

	auto Font::get_glyph(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(descriptor.id != invalid_font_id);

		if (const auto it = cached_glyphs.find(key); it != cached_glyphs.end())
		{
			return std::addressof(it->second);
		}

		return nullptr;
	}

	auto Font::set_glyph(const GlyphKey& key, const GlyphParser::GlyphDescriptor& glyph_descriptor) noexcept -> GlyphInfo&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(descriptor.id != invalid_font_id);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_descriptor.valid());

		GlyphInfo info{};
		info.rect = glyph_descriptor.rect;
		info.advance_x = glyph_descriptor.advance_x;
		info.visible = glyph_descriptor.visible;
		info.colored = glyph_descriptor.colored;

		return cached_glyphs.insert_or_assign(key, info).first->second;
	}

	FontLoadQueue::FontLoadQueue() noexcept
		: new_font_index_{0} {}

	auto FontLoadQueue::push(const std::filesystem::path& path) noexcept -> bool
	{
		std::ifstream file{path, std::ios::binary};
		if (not file.is_open())
		{
			// todo: error handling
			return false;
		}

		file.seekg(0, std::ios::end);
		const auto size = file.tellg();

		auto data = std::make_unique_for_overwrite<element_type[]>(size);
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(data.get()), size);
		file.close();

		list_.emplace_back(data_type{data.release()}, static_cast<size_type>(size));
		return true;
	}

	auto FontLoadQueue::upload(GlyphParser& parser, const functional::function_reference_wrapper<void(Font&&)> font_dest) noexcept -> void
	{
		if (const auto size = static_cast<list_type::difference_type>(list_.size()); size > new_font_index_)
		{
			auto new_fonts = std::ranges::subrange{list_.begin() + new_font_index_, list_.end()};

			std::ranges::for_each(
				new_fonts,
				[&](const Descriptor& descriptor) noexcept -> void
				{
					if (const auto result = parser.load({descriptor.data.get(), descriptor.size}); result.valid())
					{
						Font font{.descriptor = {.identifier = result.identifier, .id = result.id}, .cached_glyphs = {}};
						font_dest(std::move(font));
					}
					else
					{
						// todo: error handling
						GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
					}
				}
			);

			new_font_index_ = size;
		}
	}
}
