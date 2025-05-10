// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <primitive/circle.hpp>
#include <primitive/color.hpp>
#include <primitive/ellipse.hpp>
#include <primitive/extent.hpp>
#include <primitive/point.hpp>
#include <primitive/rect.hpp>
#include <primitive/vertex.hpp>

#include <functional/enumeration.hpp>

namespace gal::prometheus::gfx
{
	using point_type = primitive::basic_point_2d<float>;
	using uv_type = primitive::basic_point_2d<float>;
	using color_type = primitive::basic_color;
	using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
	using index_type = std::uint16_t;

	using extent_type = primitive::basic_extent_2d<float>;
	using rect_type = primitive::basic_rect_2d<float, float>;
	using circle_type = primitive::basic_circle_2d<float>;
	using ellipse_type = primitive::basic_ellipse_2d<float, float>;

	// =========================================================
	// TEXTURE
	// =========================================================

	// DX11: ID3D11ShaderResourceView
	// DX12: D3D12_GPU_DESCRIPTOR_HANDLE::ptr
	using texture_id_type = std::uintptr_t;
	constexpr texture_id_type invalid_texture_id{0};

	class Texture;
	class BorrowTexture;

	// =========================================================
	// FONT
	// =========================================================

	using texture_atlas_id_type = std::uint32_t;
	constexpr texture_atlas_id_type invalid_texture_atlas_id{std::numeric_limits<texture_atlas_id_type>::max()};

	using font_id_type = std::uint32_t;
	constexpr font_id_type invalid_font_id{std::numeric_limits<font_id_type>::max()};

	enum class GlyphFlag : std::uint8_t
	{
		NONE = 0,
		BOLD = 1 << 0,
		ITALIC = 1 << 1,
	};

	class GlyphKey;
	class GlyphInfo;

	class GlyphParsedInfo;
	class GlyphParser;

	class FontFace;

	// =========================================================
	// RENDERER LIST
	// =========================================================

	enum class RenderListFlag : std::uint8_t
	{
		NONE = 0,
		ANTI_ALIASED_LINE = 1 << 0,
		ANTI_ALIASED_LINE_USE_TEXTURE = 1 << 1,
		ANTI_ALIASED_FILL = 1 << 2,
	};

	enum class RenderFlag : std::uint8_t
	{
		NONE = 0,
		// specify that shape should be closed
		// @see RenderList::draw_polygon_line
		// @see RenderList::draw_polygon_line_aa
		// @see RenderList::path_stroke
		CLOSED = 1 << 0,
		// enable rounding left-top corner only (when rounding > 0.0f, we default to all corners)
		// @see RenderList::path_rect
		// @see RenderList::rect
		// @see RenderList::rect_filled
		ROUND_CORNER_LEFT_TOP = 1 << 1,
		// enable rounding right_top corner only (when rounding > 0.0f, we default to all corners)
		// @see RenderList::path_rect
		// @see RenderList::rect
		// @see RenderList::rect_filled
		ROUND_CORNER_RIGHT_TOP = 1 << 2,
		// enable rounding left-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see RenderList::path_rect
		// @see RenderList::rect
		// @see RenderList::rect_filled
		ROUND_CORNER_LEFT_BOTTOM = 1 << 3,
		// enable rounding right-bottom corner only (when rounding > 0.0f, we default to all corners)
		// @see RenderList::path_rect
		// @see RenderList::rect
		// @see RenderList::rect_filled
		ROUND_CORNER_RIGHT_BOTTOM = 1 << 4,
		// disable rounding on all corners (when rounding > 0.0f)
		ROUND_CORNER_NONE = 1 << 5,

		ROUND_CORNER_LEFT = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_LEFT_BOTTOM,
		ROUND_CORNER_TOP = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_RIGHT_TOP,
		ROUND_CORNER_RIGHT = ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_RIGHT_BOTTOM,
		ROUND_CORNER_BOTTOM = ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,

		ROUND_CORNER_ALL = ROUND_CORNER_LEFT_TOP | ROUND_CORNER_RIGHT_TOP | ROUND_CORNER_LEFT_BOTTOM | ROUND_CORNER_RIGHT_BOTTOM,
		ROUND_CORNER_DEFAULT = ROUND_CORNER_ALL,
		ROUND_CORNER_MASK = ROUND_CORNER_ALL | ROUND_CORNER_NONE,
	};

	enum class RenderArcFlag : std::uint8_t
	{
		// [0~3)
		Q1 = 1 << 0,
		// [3~6)
		Q2 = 1 << 1,
		// [6~9)
		Q3 = 1 << 2,
		// [9~12)
		Q4 = 1 << 3,

		RIGHT_TOP = Q1,
		LEFT_TOP = Q2,
		LEFT_BOTTOM = Q3,
		RIGHT_BOTTOM = Q4,
		TOP = Q1 | Q2,
		BOTTOM = Q3 | Q4,
		LEFT = Q2 | Q3,
		RIGHT = Q1 | Q4,
		ALL = Q1 | Q2 | Q3 | Q4,

		// [3, 0)
		Q1_CLOCK_WISH = 1 << 4,
		// [6, 3)
		Q2_CLOCK_WISH = 1 << 5,
		// [9, 6)
		Q3_CLOCK_WISH = 1 << 6,
		// [12, 9)
		Q4_CLOCK_WISH = 1 << 7,

		RIGHT_TOP_CLOCK_WISH = Q1_CLOCK_WISH,
		LEFT_TOP_CLOCK_WISH = Q2_CLOCK_WISH,
		LEFT_BOTTOM_CLOCK_WISH = Q3_CLOCK_WISH,
		RIGHT_BOTTOM_CLOCK_WISH = Q4_CLOCK_WISH,
		TOP_CLOCK_WISH = Q1_CLOCK_WISH | Q2_CLOCK_WISH,
		BOTTOM_CLOCK_WISH = Q3_CLOCK_WISH | Q4_CLOCK_WISH,
		LEFT_CLOCK_WISH = Q2_CLOCK_WISH | Q3_CLOCK_WISH,
		RIGHT_CLOCK_WISH = Q1_CLOCK_WISH | Q4_CLOCK_WISH,
		ALL_CLOCK_WISH = Q1_CLOCK_WISH | Q2_CLOCK_WISH | Q3_CLOCK_WISH | Q4_CLOCK_WISH,
	};

	class RenderListSharedData;
	class RenderList;

	// =========================================================
	// RENDERER
	// =========================================================

	class Renderer;

	// =========================================================
	// CONTEXT
	// =========================================================

	class TextureContext;
	class RenderContext;
} // namespace gal::prometheus::gfx

namespace gal::prometheus::meta::user_defined
{
	template<>
	struct enum_is_flag<gfx::GlyphFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderListFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderArcFlag> : std::true_type {};
} // namespace gal::prometheus::meta::user_defined
