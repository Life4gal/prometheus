// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include <draw/def.hpp>

#include <i18n/range.hpp>

namespace gal::prometheus::draw
{
	// ReSharper disable once CppClassCanBeFinal
	class Font
	{
	public:
		using rect_type = DrawListDef::rect_type;
		using point_type = DrawListDef::point_type;
		using extent_type = DrawListDef::extent_type;

		using color_type = DrawListDef::color_type;
		using index_type = DrawListDef::index_type;

		using texture_id_type = DrawListDef::texture_id_type;
		constexpr static texture_id_type invalid_texture_id = 0;

		using uv_rect_type = primitive::basic_rect_2d<float>;
		using uv_point_type = uv_rect_type::point_type;
		using uv_extent_type = uv_rect_type::extent_type;

		// todo: char32_t ?
		using char_type = char16_t;

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_PUSH
		// Variable '%1$s' is uninitialized. Always initialize a member variable (type.6).
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING(26495)
		#endif

		struct glyph_type
		{
			rect_type rect;
			uv_rect_type uv;
			float advance_x;
		};

		#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
		GAL_PROMETHEUS_COMPILER_DISABLE_WARNING_POP
		#endif

		using glyphs_type = std::unordered_map<char_type, glyph_type>;

		using glyph_value_type = i18n::RangeBuilder::value_type;
		using glyph_ranges_type = i18n::RangeBuilder::ranges_type;

		using baked_line_uv_type = std::vector<uv_rect_type>;

		// todo
		constexpr static std::uint32_t default_baked_line_max_width = 63;

		struct option_type
		{
			std::string font_path;
			glyph_ranges_type glyph_ranges;

			std::uint32_t pixel_height;
			// 0 => default_baked_line_max_width
			std::uint32_t baked_line_max_width;
		};

		class Texture final
		{
			friend Font;

		public:
			// size.width * size.height (RGBA)
			using data_type = std::unique_ptr<std::uint32_t[]>;

		private:
			extent_type size_;
			data_type data_;
			std::reference_wrapper<texture_id_type> id_;

			explicit Texture(texture_id_type& id) noexcept
				: size_{},
				  data_{nullptr},
				  id_{id} {}

			constexpr auto set_size(const extent_type size) noexcept -> void
			{
				size_ = size;
			}

			constexpr auto set_data(data_type&& data) noexcept -> void
			{
				data_ = std::move(data);
			}

		public:
			Texture(const Texture& other) = delete;
			Texture(Texture&& other) noexcept = default;
			auto operator=(const Texture& other) -> Texture& = delete;
			auto operator=(Texture&& other) noexcept -> Texture& = default;

			~Texture() noexcept;

			[[nodiscard]] constexpr auto valid() const noexcept -> bool
			{
				return data_ != nullptr;
			}

			[[nodiscard]] constexpr auto size() const noexcept -> extent_type
			{
				return size_;
			}

			[[nodiscard]] constexpr auto data() const & noexcept -> const data_type&
			{
				return data_;
			}

			[[nodiscard]] constexpr auto data() && noexcept -> data_type&&
			{
				return std::move(data_);
			}

			auto bind(texture_id_type id) const noexcept -> void;
		};

	private:
		std::string font_path_;
		std::uint32_t pixel_height_;
		std::uint32_t baked_line_max_width_;

		glyphs_type glyphs_;
		glyph_type fallback_glyph_;

		uv_point_type white_pixel_uv_;
		baked_line_uv_type baked_line_uv_;

		texture_id_type texture_id_;

		auto reset() noexcept -> void;

	public:
		explicit Font() noexcept;

		Font(const Font& other) = delete;
		Font(Font&& other) noexcept = default;
		auto operator=(const Font& other) -> Font& = delete;
		auto operator=(Font&& other) noexcept -> Font& = default;

		virtual ~Font() noexcept;

		auto load(const option_type& option) noexcept -> Texture;

		// ---------------------------------------------------------

		[[nodiscard]] auto loaded() const noexcept -> bool;

		[[nodiscard]] auto font_path() const noexcept -> std::string_view;

		[[nodiscard]] auto pixel_height() const noexcept -> std::uint32_t;

		[[nodiscard]] auto baked_line_max_width() const noexcept -> std::uint32_t;

		[[nodiscard]] auto glyphs() const noexcept -> const glyphs_type&;

		[[nodiscard]] auto fallback_glyph() const noexcept -> const glyph_type&;

		[[nodiscard]] auto white_pixel_uv() const noexcept -> const uv_point_type&;

		[[nodiscard]] auto baked_line_uv() const noexcept -> const baked_line_uv_type&;

		[[nodiscard]] auto texture_id() const noexcept -> texture_id_type;

		// =========================================
		// DRAW TEXT

		[[nodiscard]] virtual auto text_size(
			std::basic_string_view<char> utf8_text,
			float font_size,
			float wrap_width,
			std::basic_string<char_type>& out_text
		) const noexcept -> extent_type;

		[[nodiscard]] virtual auto text_size(
			std::basic_string_view<char> utf8_text,
			float font_size,
			float wrap_width
		) const noexcept -> extent_type;

		virtual auto text_draw(
			std::basic_string_view<char> utf8_text,
			float font_size,
			float wrap_width,
			point_type point,
			color_type color,
			DrawListDef::Accessor accessor
		) const noexcept -> void;
	};
}
