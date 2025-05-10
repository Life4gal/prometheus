// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/font.hpp>

#include <fstream>

#include <chars/chars.hpp>
#include <gfx/context.hpp>

namespace gal::prometheus::gfx
{
	GlyphParsedInfo::GlyphParsedInfo(GlyphInfo& info, data_type data) noexcept
		: info_{info},
		  data_{std::move(data)}
	{
	}

	auto GlyphParsedInfo::upload(TextureContext& texture_context) noexcept -> void
	{
		auto& info = info_.get();

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(info.texture_atlas_id == invalid_texture_atlas_id);
		texture_context.upload_glyph_to_texture(info, data_);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(info.texture_atlas_id != invalid_texture_atlas_id);
	}

	GlyphParser::~GlyphParser() noexcept = default;

	auto GlyphParser::parse(const font_id_type id, const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> ParseResult
	{
		return this->parse(id, {.codepoint = codepoint, .size = size, .flag = flag});
	}

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

		const auto [it, inserted] = glyphs_.emplace(key, result.info);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(inserted);
		parsed_infos_upload_queue_.emplace_back(memory::ref(it->second), std::move(result.data));

		return std::addressof(it->second);
	}

	FontFace::FontFace(TextureContext& context, std::string name, const font_id_type id) noexcept
		: context_{context},
		  name_{std::move(name)},
		  id_{id},
		  fallback_glyph_{nullptr}
	{
	}

	FontFace::FontFace(TextureContext& context, const std::string_view name, const font_id_type id) noexcept
		: FontFace{context, std::string{name}, id}
	{
	}

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
				parsed_infos_upload_queue_,
				[&context = context_.get()](GlyphParsedInfo& parsed_info) noexcept -> void
				{
					parsed_info.upload(context);
				}
		);
		parsed_infos_upload_queue_.clear();
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
