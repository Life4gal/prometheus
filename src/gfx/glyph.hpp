// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <string_view>

#include <gfx/type.hpp>

#include <functional/enumeration.hpp>

namespace gal::prometheus::gfx
{
	enum class FontLoadResult : std::uint8_t
	{
		SUCCESS = 0,

		FILE_ALREADY_LOADED = 1 << 0,
		FILE_NOT_FOUND = 1 << 1,
		INVALID_FONT_FORMAT = 1 << 2,
		OUT_OF_MEMORY = 1 << 3,

		UNKNOWN_ERROR = 1 << 7,
	};

	enum class GlyphFlag : std::uint8_t
	{
		NONE = 0,
		BOLD = 1 << 0,
		ITALIC = 1 << 1,
	};

	class GlyphCode final
	{
	public:
		std::uint32_t codepoint;
		std::uint32_t size;
		GlyphFlag flag;

		constexpr GlyphCode(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept
			: codepoint{codepoint},
			  size{size},
			  flag{flag} {}
	};

	class GlyphDescriptor final
	{
	public:
		// The offset of the bitmap may be negative (signed)
		using rect_type = primitive::basic_rect_2d<std::int32_t, std::uint32_t>;
		// Texture::data_type data;
		using data_type = std::unique_ptr<std::uint32_t[]>;

		// Bitmap infos of this glyph
		rect_type rect;
		float advance_x;

		bool visible;
		bool colored;

		// note:
		//  - visible: We always assume that the data length of @c data is not less than @c rect.size.width * @c rect.size.height
		//  - invisible: nullptr (1.currently loaded font does not contain the specified codepoint, 2.it only affects the layout and does not need to be rendered, e.g. line breaks)
		data_type data;
	};

	class GlyphParser
	{
	public:
		GlyphParser(const GlyphParser&) noexcept = delete;
		GlyphParser(GlyphParser&&) noexcept = default;
		auto operator=(const GlyphParser&) noexcept -> GlyphParser& = delete;
		auto operator=(GlyphParser&&) noexcept -> GlyphParser& = default;

		virtual ~GlyphParser() noexcept;

	protected:
		GlyphParser() noexcept = default;

	public:
		/**
		 * @brief Load font data from @c path, return id of font
		 * @param path Font path
		 * @return id of the font, or invalid_font_id if failed to load
		 */
		[[nodiscard]] virtual auto load(std::string_view path) noexcept -> FontLoadResult = 0;

		/**
		 * @brief Determines whether the target font contains the glyphs of the specified codepoint
		 * @param codepoint The codepoint of the glyph to be checked
		 * @return Exists or not
		 */
		[[nodiscard]] virtual auto has_glyph(std::uint32_t codepoint) const noexcept -> bool = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint
		 * @param code {codepoint, size, flag}
		 * @return The glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] virtual auto parse(const GlyphCode& code) noexcept -> GlyphDescriptor = 0;
	};
}

// ReSharper disable once CppRedundantNamespaceDefinition
namespace gal::prometheus::meta::user_defined
{
	template<>
	struct enum_is_flag<gfx::GlyphFlag> : std::true_type {};
} // namespace gal::prometheus::meta::user_defined
