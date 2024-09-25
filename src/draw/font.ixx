// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:font;

import std;
import gal.prometheus.primitive;

#else
#pragma once

#include <type_traits>
#include <span>
#include <vector>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>

#endif

namespace gal::prometheus::draw
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	using glyph_value_type = std::uint32_t;
	using glyph_pair_type = std::pair<glyph_value_type, glyph_value_type>;

	using glyph_range_type = std::vector<glyph_pair_type>;
	using glyph_range_view_type = std::span<const glyph_pair_type>;

	using glyph_ranges_type = std::vector<glyph_range_type>;
	using glyph_ranges_view_type = std::span<const glyph_range_type>;
	using glyph_range_views_type = std::span<const glyph_range_view_type>;

	// Latin
	[[nodiscard]] auto glyph_range_latin() noexcept -> glyph_range_view_type;

	// Latin + Greek and Coptic
	[[nodiscard]] auto glyph_range_greek() noexcept -> glyph_range_view_type;

	// Latin + Half-Width + Japanese Hiragana/Katakana + set of 2500 CJK Unified Ideographs for common simplified Chinese
	[[nodiscard]] auto glyph_range_simplified_chinese_common() noexcept -> glyph_range_view_type;

	// Latin + Half-Width + Japanese Hiragana/Katakana + full set of about 21000 CJK Unified Ideographs
	[[nodiscard]] auto glyph_range_simplified_chinese_all() noexcept -> glyph_range_view_type;

	class Font final
	{
	public:
		using rect_type = primitive::basic_rect_2d<std::int32_t, std::uint32_t>;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;
		using uv_type = primitive::basic_rect_2d<float>;
		using char_type = char32_t;

		using texture_id_type = std::uintptr_t;

		struct glyph_type
		{
			rect_type rect;
			uv_type uv;
			float advance_x;
		};

		using glyphs_type = std::unordered_map<char_type, glyph_type>;

		struct texture_type
		{
			extent_type size;
			// size.width * size.height (RGBA)
			std::unique_ptr<std::uint32_t[]> data;

			// write external
			texture_id_type& id;
		};

	private:
		std::string font_path_;

		float pixel_height_;
		texture_id_type texture_id_;

		glyphs_type glyphs_;
		glyph_type fallback_glyph_;

	public:
		Font() noexcept
			: pixel_height_{0},
			  texture_id_{0},
			  fallback_glyph_{} {}

		[[nodiscard]] constexpr auto loaded() const noexcept -> bool
		{
			return not font_path_.empty();
		}

		[[nodiscard]] constexpr auto font_path() const noexcept -> std::string_view
		{
			return font_path_;
		}

		[[nodiscard]] constexpr auto pixel_height() const noexcept -> float
		{
			return pixel_height_;
		}

		[[nodiscard]] constexpr auto texture_id() const noexcept -> texture_id_type
		{
			return texture_id_;
		}

		[[nodiscard]] constexpr auto glyphs() const noexcept -> const glyphs_type&
		{
			return glyphs_;
		}

		[[nodiscard]] constexpr auto fallback_glyph() const noexcept -> const glyph_type&
		{
			return fallback_glyph_;
		}

		[[nodiscard]] auto load(std::string_view font_path, std::uint32_t pixel_height, glyph_ranges_view_type glyph_ranges) noexcept -> texture_type;

		[[nodiscard]] auto load(std::string_view font_path, std::uint32_t pixel_height, glyph_range_views_type glyph_ranges) noexcept -> texture_type;
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
