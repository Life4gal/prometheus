// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/font.hpp>

#include <chars/chars.hpp>
#include <gfx/context.hpp>

namespace gal::prometheus::gfx
{
	auto FontFace::find_or_parse_glyph(const GlyphKey& key) noexcept -> GlyphInfo*
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id_ != invalid_font_id);

		if (const auto it = glyphs_.find(key); it != glyphs_.end())
		{
			return std::addressof(it->second);
		}

		auto& parser = context_.get().parser();
		// if (not parser.has_glyph(id_, key.codepoint))
		// {
		// 	return nullptr;
		// }

		auto result = parser.parse(id_, key);
		if (not result.valid())
		{
			return nullptr;
		}

		GlyphInfo info{};
		info.rect = result.rect;
		info.advance_x = result.advance_x;
		info.visible = result.visible;
		info.colored = result.colored;

		const auto [it, inserted] = glyphs_.emplace(key, info);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(inserted);
		parsed_info_queue_.emplace_back(memory::ref(it->second), std::move(result));

		return std::addressof(it->second);
	}

	FontFace::FontFace(TextureContext& context, std::string name, const font_id_type id) noexcept
		: context_{context},
		  name_{std::move(name)},
		  id_{id},
		  fallback_glyph_{nullptr} {}

	FontFace::FontFace(TextureContext& context, const std::string_view name, const font_id_type id) noexcept
		: FontFace{context, std::string{name}, id} {}

	auto FontFace::name() const noexcept -> std::string_view
	{
		return name_;
	}

	auto FontFace::id() const noexcept -> font_id_type
	{
		return id_;
	}

	auto FontFace::initialize() noexcept -> void
	{
		GlyphKey key{.codepoint = u'\xFFFD', .size = 16u, .flag = GlyphFlag::NONE};
		fallback_glyph_ = find_glyph_no_fallback(key);

		if (fallback_glyph_ == nullptr)
		{
			key.codepoint = u'?';
			fallback_glyph_ = find_glyph_no_fallback(key);
		}
		if (fallback_glyph_ == nullptr)
		{
			key.codepoint = u' ';
			fallback_glyph_ = find_glyph_no_fallback(key);
		}

		if (fallback_glyph_ == nullptr)
		{
			// todo
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
		}
	}

	auto FontFace::upload() noexcept -> void
	{
		std::ranges::for_each(
			parsed_info_queue_,
			[&context = context_.get()](parsed_info_type& parsed_info) noexcept -> void
			{
				auto& info = parsed_info.info.get();

				const auto [texture_atlas_id, uv] = context.upload_parsed_info_to_texture(parsed_info.result);
				info.texture_atlas_id = texture_atlas_id;
				info.uv = uv;
			}
		);
		parsed_info_queue_.clear();
	}

	auto FontFace::find_glyph(const GlyphKey& key) noexcept -> const GlyphInfo&
	{
		if (const auto* info = find_or_parse_glyph(key); info != nullptr)
		{
			return *info;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(fallback_glyph_ != nullptr);
		return *fallback_glyph_;
	}

	auto FontFace::find_glyph_no_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = find_or_parse_glyph(key); info)
		{
			return info;
		}

		return nullptr;
	}
} // namespace gal::prometheus::gfx
