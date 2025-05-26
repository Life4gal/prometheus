// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <filesystem>

#include <primitive/circle.hpp>
#include <primitive/color.hpp>
#include <primitive/ellipse.hpp>
#include <primitive/extent.hpp>
#include <primitive/point.hpp>
#include <primitive/rect.hpp>
#include <primitive/vertex.hpp>

#include <memory/unique_ptr.hpp>
#include <memory/reference_wrapper.hpp>
#include <functional/enumeration.hpp>

namespace gal::prometheus::gfx
{
	// =========================================================
	// CONTEXT
	// =========================================================

	class Context;

	// =========================================================
	// PRIMITIVE
	// =========================================================

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

	// D3D11: ID3D11ShaderResourceView
	// D3D12: D3D12_GPU_DESCRIPTOR_HANDLE::ptr / HEAP index + constant offset
	using texture_id_type = std::uintptr_t;
	constexpr texture_id_type invalid_texture_id{0};

	class TextureDescriptor final
	{
	public:
		// 32-bits (RGBA)
		using element_type = std::uint32_t;
		// size.width * size.height
		using data_type = std::unique_ptr<element_type[]>;
		using data_view_type = std::span<element_type>;

		using size_type = primitive::basic_extent_2d<std::uint32_t>;

		// ==============================
		// CPU side
		// ==============================

		data_type data;
		size_type size;

		static_assert(sizeof(texture_id_type) == sizeof(std::uint64_t));
		std::uint64_t dirty : 1;

		// ==============================
		// GPU side
		// ==============================

		std::uint64_t id : 63;
	};

	// =========================================================
	// GLYPH / FONT
	// =========================================================

	// index
	using font_id_type = std::uint32_t;
	constexpr font_id_type invalid_font_id{std::numeric_limits<font_id_type>::max()};

	enum class GlyphFlag : std::uint8_t
	{
		NONE = 0,
		BOLD = 1 << 0,
		ITALIC = 1 << 1,
	};

	class GlyphParser
	{
	public:
		class [[nodiscard]] FontDescriptor final
		{
		public:
			using element_type = std::uint8_t;
			using data_type = std::unique_ptr<element_type>;
			using data_view_type = std::span<element_type>;

			using size_type = std::uint32_t;

			std::string_view identifier;
			font_id_type id;

			[[nodiscard]] constexpr static auto error() noexcept -> FontDescriptor
			{
				return {.identifier = {}, .id = invalid_font_id};
			}

			[[nodiscard]] constexpr explicit operator bool() const noexcept
			{
				return id != invalid_font_id;
			}

			[[nodiscard]] constexpr auto valid() const noexcept -> bool
			{
				return operator bool();
			}
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

		class [[nodiscard]] GlyphDescriptor final
		{
		public:
			// The offset of the bitmap may be negative (signed)
			using rect_type = primitive::basic_rect_2d<std::int32_t, std::uint32_t>;

			// Bitmap infos of this glyph
			rect_type rect;
			float advance_x;
			bool visible;
			bool colored;

			// FIXME-OPT:
			// We could have pre-specified an area to write glyph data to,
			// but that would have meant exposing the texture implementation details to the outside world,
			// and its implementation will be coupled with the GlyphParser, so we will leave the choice to the GlyphParser,
			// which can find a way to reduce the number of dynamic allocations in the internal.
			// note:
			// We always assume that the data length of @c data is not less than @c rect.size.width * @c rect.size.height
			TextureDescriptor::data_type data;

			[[nodiscard]] constexpr static auto error() noexcept -> GlyphDescriptor
			{
				return {.rect = {-0xbad, -0xbad, 0xc0ffee, 0xc0ffee}, .advance_x = -1, .visible = false, .colored = false, .data = nullptr};
			}

			[[nodiscard]] constexpr explicit operator bool() const noexcept
			{
				return data != nullptr;
			}

			[[nodiscard]] constexpr auto valid() const noexcept -> bool
			{
				return operator bool();
			}
		};

		GlyphParser(const GlyphParser&) noexcept = delete;
		GlyphParser(GlyphParser&&) noexcept = default;
		auto operator=(const GlyphParser&) noexcept -> GlyphParser& = delete;
		auto operator=(GlyphParser&&) noexcept -> GlyphParser& = default;

		virtual ~GlyphParser() noexcept;

		GlyphParser() noexcept = default;

		/**
		 * @brief Load font data from @c path, return id of font
		 * @param path Font path
		 * @return id of the font, or invalid_font_id if failed to load
		 */
		[[nodiscard]] virtual auto load(const std::filesystem::path& path) noexcept -> FontDescriptor = 0;

		/**
		 * @brief Determines whether the target font contains the glyphs of the specified codepoint
		 * @param id The id returned by loading the font from the previous load
		 * @param codepoint The codepoint of the glyph to be checked
		 * @return Exists or not
		 */
		[[nodiscard]] virtual auto has_glyph(font_id_type id, std::uint32_t codepoint) const noexcept -> bool = 0;

		/**
		 * @brief Get the glyph information of the specified size (and style) of the target codepoint, if it does not exist then data is a null pointer
		 * @param id The id returned by loading the font from the previous load
		 * @param code {codepoint, size, flag}
		 * @return The glyph information of the specified size (and style) of the target codepoint
		 */
		[[nodiscard]] virtual auto parse(font_id_type id, const GlyphCode& code) noexcept -> GlyphDescriptor = 0;
	};

	// =========================================================
	// RENDERER LIST
	// =========================================================

	enum class RenderListFlag : std::uint8_t
	{
		NONE = 0,
		ANTI_ALIASED_LINE = 1 << 0,
		ANTI_ALIASED_LINE_USE_TEXTURE = 1 << 1,
		ANTI_ALIASED_FILL = 1 << 2,

		DEFAULT = ANTI_ALIASED_LINE | ANTI_ALIASED_LINE_USE_TEXTURE | ANTI_ALIASED_FILL,
	};

	enum class RenderRectFlag : std::uint8_t
	{
		NONE = 0,
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

	class RenderListSharedData
	{
	public:
		using circle_segment_count_type = std::uint8_t;
		constexpr static std::size_t circle_segment_counts_count = 64;
		using circle_segment_counts_type = std::array<circle_segment_count_type, circle_segment_counts_count>;

		constexpr static std::uint32_t circle_segments_min = 4;
		constexpr static std::uint32_t circle_segments_max = 512;

		constexpr static std::size_t vertex_sample_points_count = 48;
		using vertex_sample_points_type = std::array<point_type, vertex_sample_points_count>;

		constexpr static std::size_t baked_line_uv_count = 64;
		using baked_line_uvs_type = std::array<rect_type, baked_line_uv_count>;

		circle_segment_counts_type circle_segment_counts;

		vertex_sample_points_type vertex_sample_points;

		baked_line_uvs_type baked_line_uvs;
		point_type white_pixel_uv;

		// Maximum error (in pixels) allowed when using `circle`/`circle_filled` or drawing rounded corner rectangles with no explicit segment count specified.
		// Decrease for higher quality but more geometry.
		float circle_segment_max_error;
		// Cutoff radius after which arc drawing will fall back to slower `path_arc`
		float arc_fast_radius_cutoff;
		// Tessellation tolerance when using `path_bezier_curve` without a specific number of segments.
		// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
		float curve_tessellation_tolerance;

		// --------------------------------------------------

		RenderListSharedData() noexcept;

		// --------------------------------------------------

		[[nodiscard]] auto circle_auto_segment_count(float radius) const noexcept -> circle_segment_count_type;

		[[nodiscard]] auto vertex_sample_point(std::size_t index) const noexcept -> const point_type&;

		// --------------------------------------------------

		auto set_circle_tessellation_max_error(float max_error) noexcept -> void;

		auto set_curve_tessellation_tolerance(float tolerance) noexcept -> void;
	};

	class RenderData final
	{
	public:
		using size_type = std::uint32_t;

		struct [[nodiscard]] command_type
		{
			rect_type scissor;
			texture_id_type texture;

			// =======================

			// set by RenderList::index_list.size()
			// start offset in @c RenderList::index_list
			size_type index_offset;
			// set by RenderList::draw_xxx
			// number of indices (multiple of 3) to be rendered as triangles
			size_type element_count;
		};

		using vertex_list_type = std::vector<vertex_type>;
		using index_list_type = std::vector<index_type>;
		using command_list_type = std::vector<command_type>;

		std::reference_wrapper<const vertex_list_type> vertex_list;
		std::reference_wrapper<const index_list_type> index_list;
		std::reference_wrapper<const command_list_type> command_list;
	};

	using render_data_list_type = std::vector<RenderData>;

	class RenderList
	{
		friend class Context;
		friend class Renderer;

	public:
		class RenderListContext;

		class Painter final
		{
		public:
			using path_list_type = std::vector<point_type>;

		private:
			memory::RefWrapper<RenderList> render_list_;
			path_list_type path_list_;

			auto draw_polygon_line(color_type color, float thickness, bool close) noexcept -> void;
			auto draw_polygon_line_aa(color_type color, float thickness, bool close) noexcept -> void;
			auto draw_convex_polygon_line_filled(color_type color) noexcept -> void;
			auto draw_convex_polygon_line_filled_aa(color_type color) noexcept -> void;

		public:
			Painter(const Painter&) noexcept = delete;
			Painter(Painter&&) noexcept = default;
			auto operator=(const Painter&) noexcept -> Painter& = delete;
			auto operator=(Painter&&) noexcept -> Painter& = default;

			~Painter() noexcept;

			explicit Painter(RenderList& render_list, std::size_t reserve_point) noexcept;

			auto clear() noexcept -> Painter&;
			auto reserve_extra(std::size_t size) noexcept -> Painter&;
			auto reserve(std::size_t size) noexcept -> Painter&;

			auto pin(const point_type& point) noexcept -> Painter&;

			auto line(const point_type& from, const point_type& to) noexcept -> Painter&;
			auto triangle(const point_type& a, const point_type& b, const point_type& c) noexcept -> Painter&;
			auto quadrilateral(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> Painter&;
			auto rect(const rect_type& rect, float rounding, RenderRectFlag flag) noexcept -> Painter&;
			auto rect(const rect_type::point_type& left_top, const rect_type::extent_type& extent, float rounding, RenderRectFlag flag) noexcept -> Painter&;
			auto rect(const rect_type::point_type& left_top, const rect_type::point_type& right_bottom, float rounding, RenderRectFlag flag) noexcept -> Painter&;
			auto circle_n(const circle_type& circle, std::uint32_t segments) noexcept -> Painter&;
			auto circle_n(const circle_type::point_type& center, circle_type::radius_value_type radius, std::uint32_t segments) noexcept -> Painter&;
			auto circle(const circle_type& circle) noexcept -> Painter&;
			auto circle(const circle_type::point_type& center, circle_type::radius_value_type radius) noexcept -> Painter&;
			auto ellipse_n(const ellipse_type& ellipse, std::uint32_t segments) noexcept -> Painter&;
			auto ellipse_n(const ellipse_type::point_type& center, const ellipse_type::radius_type& radius, ellipse_type::rotation_value_type rotation, std::uint32_t segments) noexcept -> Painter&;
			auto ellipse(const ellipse_type& ellipse) noexcept -> Painter&;
			auto ellipse(const ellipse_type::point_type& center, const ellipse_type::radius_type& radius, ellipse_type::rotation_value_type rotation) noexcept -> Painter&;
			auto arc_fast(const circle_type& circle, int sample_point_from, int sample_point_to) noexcept -> Painter&;
			auto arc_fast(const circle_type& circle, RenderArcFlag flag) noexcept -> Painter&;
			auto arc_n(const circle_type& circle, float degree_from, float degree_to, std::uint32_t segments) noexcept -> Painter&;
			auto arc(const circle_type& circle, float degree_from, float degree_to) noexcept -> Painter&;
			auto arc_n(const ellipse_type& ellipse, float degree_from, float degree_to, std::uint32_t segments) noexcept -> Painter&;
			auto bezier_cubic_n(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4, std::uint32_t segments) noexcept -> Painter&;
			auto bezier_cubic(const point_type& p1, const point_type& p2, const point_type& p3, const point_type& p4) noexcept -> Painter&;
			auto bezier_quadratic_n(const point_type& p1, const point_type& p2, const point_type& p3, std::uint32_t segments) noexcept -> Painter&;
			auto bezier_quadratic(const point_type& p1, const point_type& p2, const point_type& p3) noexcept -> Painter&;

			/**
			 * @brief Draws the fill shape according to the specified path points
			 * @param color Shape color
			 */
			auto stroke(color_type color) noexcept -> void;

			/**
			 * @brief Plot the corresponding lines/shapes according to the specified path points
			 * @param color Line/shape color
			 * @param thickness Line thickness
			 * @param close Whether the graph is closed, in other words, whether the first point should be connected to the last point
			 */
			auto stroke(color_type color, float thickness, bool close) noexcept -> void;
		};

	private:
		memory::UniquePointer<RenderListContext> context_;

		explicit RenderList(Context& context, RenderListFlag flag) noexcept;

	public:
		RenderList(const RenderList&) noexcept = delete;
		RenderList(RenderList&&) noexcept; //= default;
		auto operator=(const RenderList&) noexcept -> RenderList& = delete;
		auto operator=(RenderList&&) noexcept -> RenderList&; // = default;

		~RenderList() noexcept;

		auto reset() noexcept -> void;

		auto set_flag(RenderListFlag flag) noexcept -> void;

		// ----------------------------------------------------------------------------
		// SCISSOR & TEXTURE

		auto push_scissor(const rect_type& rect, bool intersect_with_current_scissor) noexcept -> rect_type&;

		auto pop_scissor() noexcept -> void;

		auto push_texture(texture_id_type texture) noexcept -> void;

		auto pop_texture() noexcept -> void;

		// ----------------------------------------------------------------------------
		// PAINTER

		[[nodiscard]] auto painter(const Painter::path_list_type::size_type reserve_point = 0) noexcept -> Painter
		{
			return Painter{*this, reserve_point};
		}

		// ----------------------------------------------------------------------------
		// PRIMITIVE

		auto line(
			const point_type& from,
			const point_type& to,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto triangle_filled(
			const point_type& a,
			const point_type& b,
			const point_type& c,
			color_type color
		) noexcept -> void;

		auto quadrilateral(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto quadrilateral_filled(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color
		) noexcept -> void;

		auto rect(
			const rect_type& rect,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL,
			float thickness = 1.f
		) noexcept -> void;

		auto rect(
			const point_type& left_top,
			const rect_type::extent_type& extent,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL,
			float thickness = 1.f
		) noexcept -> void;

		auto rect(
			const rect_type::point_type& left_top,
			const rect_type::point_type& right_bottom,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL,
			float thickness = 1.f
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const rect_type::extent_type& extent,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL
		) noexcept -> void;

		auto rect_filled(
			const rect_type::point_type& left_top,
			const rect_type::point_type& right_bottom,
			color_type color,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::ROUND_CORNER_ALL
		) noexcept -> void;

		auto rect_filled(
			const rect_type& rect,
			color_type color_left_top,
			color_type color_right_top,
			color_type color_left_bottom,
			color_type color_right_bottom
		) noexcept -> void;

		auto rect_filled(
			const point_type& left_top,
			const rect_type::extent_type& extent,
			color_type color_left_top,
			color_type color_right_top,
			color_type color_left_bottom,
			color_type color_right_bottom
		) noexcept -> void;

		auto rect_filled(
			const rect_type::point_type& left_top,
			const rect_type::point_type& right_bottom,
			color_type color_left_top,
			color_type color_right_top,
			color_type color_left_bottom,
			color_type color_right_bottom
		) noexcept -> void;

		auto circle_n(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n(
			const circle_type::point_type& center,
			circle_type::radius_value_type radius,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_n_filled(
			const circle_type& circle,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle_n_filled(
			const circle_type::point_type& center,
			circle_type::radius_value_type radius,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto circle(
			const circle_type& circle,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto circle(
			const circle_type::point_type& center,
			circle_type::radius_value_type radius,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto circle_filled(
			const circle_type& circle,
			color_type color
		) noexcept -> void;

		auto circle_filled(
			const circle_type::point_type& center,
			circle_type::radius_value_type radius,
			color_type color
		) noexcept -> void;

		auto ellipse_n(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n(
			const ellipse_type::point_type& center,
			const ellipse_type::radius_type& radius,
			ellipse_type::rotation_value_type rotation,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_n_filled(
			const ellipse_type& ellipse,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse_n_filled(
			const ellipse_type::point_type& center,
			const ellipse_type::radius_type& radius,
			ellipse_type::rotation_value_type rotation,
			color_type color,
			std::uint32_t segments
		) noexcept -> void;

		auto ellipse(
			const ellipse_type& ellipse,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse(
			const ellipse_type::point_type& center,
			const ellipse_type::radius_type& radius,
			ellipse_type::rotation_value_type rotation,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto ellipse_filled(
			const ellipse_type& ellipse,
			color_type color
		) noexcept -> void;

		auto ellipse_filled(
			const ellipse_type::point_type& center,
			const ellipse_type::radius_type& radius,
			ellipse_type::rotation_value_type rotation,
			color_type color
		) noexcept -> void;

		auto bezier_cubic_n(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto bezier_cubic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			const point_type& p4,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		auto bezier_quadratic_n(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			color_type color,
			std::uint32_t segments,
			float thickness = 1.f
		) noexcept -> void;

		auto bezier_quadratic(
			const point_type& p1,
			const point_type& p2,
			const point_type& p3,
			color_type color,
			float thickness = 1.f
		) noexcept -> void;

		// ----------------------------------------------------------------------------
		// TEXT

		auto text(
			std::string_view utf8_text,
			std::uint32_t font_size,
			const point_type& point,
			color_type color,
			float wrap_width = 99999999.f
		) noexcept -> void;

		auto text(
			std::string_view utf8_text,
			std::uint32_t font_size,
			const point_type& point,
			color_type color,
			GlyphFlag flag,
			float wrap_width = 99999999.f
		) noexcept -> void;

		// FIXME: There is no suitable place to implement this function, this function must be highly consistent with the implementation when drawing text
		auto text_size(
			std::string_view utf8_text,
			std::uint32_t font_size,
			float wrap_width = 99999999.f
		) noexcept -> extent_type;

		// FIXME: There is no suitable place to implement this function, this function must be highly consistent with the implementation when drawing text
		auto text_size(
			std::string_view utf8_text,
			std::uint32_t font_size,
			GlyphFlag flag,
			float wrap_width = 99999999.f
		) noexcept -> extent_type;

		// ----------------------------------------------------------------------------
		// IMAGE

		// p1________ p2
		//    |            |
		//    |            |
		// p4|_______| p3
		auto image(
			texture_id_type texture_id,
			const rect_type::point_type& display_p1,
			const rect_type::point_type& display_p2,
			const rect_type::point_type& display_p3,
			const rect_type::point_type& display_p4,
			const uv_type& uv_p1 = {0, 0},
			const uv_type& uv_p2 = {1, 0},
			const uv_type& uv_p3 = {1, 1},
			const uv_type& uv_p4 = {0, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const rect_type& display_rect,
			const rect_type& uv_rect = {0, 0, 1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;
		
		auto image(
			texture_id_type texture_id,
			const rect_type::point_type& display_left_top,
			const rect_type::extent_type& display_size,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image(
			texture_id_type texture_id,
			const rect_type::point_type& display_left_top,
			const rect_type::point_type& display_right_bottom,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const rect_type& display_rect,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::NONE,
			const rect_type& uv_rect = {0, 0, 1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;
		
		auto image_rounded(
			texture_id_type texture_id,
			const rect_type::point_type& display_left_top,
			const rect_type::extent_type& display_size,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;

		auto image_rounded(
			texture_id_type texture_id,
			const rect_type::point_type& display_left_top,
			const rect_type::point_type& display_right_bottom,
			float rounding = .0f,
			RenderRectFlag flag = RenderRectFlag::NONE,
			const uv_type& uv_left_top = {0, 0},
			const uv_type& uv_right_bottom = {1, 1},
			color_type color = primitive::colors::white
		) noexcept -> void;
	};

	// =========================================================
	// RENDERER
	// =========================================================

	class Renderer
	{
		friend Context;

	public:
		Renderer(const Renderer&) noexcept = delete;
		Renderer(Renderer&&) noexcept = default;
		auto operator=(const Renderer&) noexcept -> Renderer& = delete;
		auto operator=(Renderer&&) noexcept -> Renderer& = default;

		virtual ~Renderer() noexcept;

	protected:
		Renderer() noexcept;

	public:
		/**
		 * @brief Constructs the renderer, which assumes that all the parameters needed for the renderer have been set
		 */
		[[nodiscard]] auto construct() noexcept -> bool;

		/**
		 * @brief Destructs the renderer
		 */
		auto destruct() noexcept -> void;

		/**
		 * @brief Whether the current renderer is ready (@c construct was called and succeeded)
		 */
		[[nodiscard]] auto ready() const noexcept -> bool;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto new_frame(Context& context) noexcept -> void;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto present(Context& context, const extent_type& display_size) noexcept -> void;

		/**
		 * @brief
		 * @note @c new_frame -> @c present -> @c end_frame
		 */
		auto end_frame(Context& context) noexcept -> void;

	private:
		virtual auto do_construct() noexcept -> bool = 0;
		virtual auto do_destruct() noexcept -> void = 0;
		[[nodiscard]] virtual auto do_ready() const noexcept -> bool = 0;

		[[nodiscard]] virtual auto do_texture_create(TextureDescriptor::data_view_type data, TextureDescriptor::size_type size) noexcept -> texture_id_type = 0;
		virtual auto do_texture_update(const TextureDescriptor& texture) noexcept -> void = 0;
		virtual auto do_texture_destroy(texture_id_type texture_id) noexcept -> void = 0;

		virtual auto do_present(const render_data_list_type& render_data_list, const extent_type& display_size) noexcept -> void = 0;
	};

	// =========================================================
	// CONTEXT
	// =========================================================

	[[nodiscard]] auto create_context(std::shared_ptr<GlyphParser> glyph_parser, std::shared_ptr<Renderer> renderer) noexcept -> Context*;
	auto destroy_context(Context& context) noexcept -> void;
	auto destroy_context(Context* context) noexcept -> void;

	// =========================================================
	// GLYPH PARSER
	// =========================================================

	/**
	 * @brief Setting the glyph parser of context
	 * @return Previous glyph parser, or nullptr if newly set
	 */
	auto set_glyph_parser(Context& context, std::shared_ptr<GlyphParser> glyph_parser) noexcept -> std::shared_ptr<GlyphParser>;

	// =========================================================
	// RENDERER
	// =========================================================

	/**
	 * @brief Setting the renderer of context
	 * @return Previous renderer, or nullptr if newly set
	 */
	auto set_renderer(Context& context, std::shared_ptr<Renderer> renderer) noexcept -> std::shared_ptr<Renderer>;

	// =========================================================
	// FONT
	// =========================================================

	/**
	 * @brief Load font from the specified path, assuming the path is a valid font file
	 * @note *Must* ensure that at least one font is added before calling @c Renderer::new_frame
	 */
	auto add_font(Context& context, const std::filesystem::path& path) noexcept -> void;

	// =========================================================
	// RENDER LIST
	// =========================================================

	auto new_render_list(Context& context, RenderListFlag flag = RenderListFlag::DEFAULT) noexcept -> RenderList&;
} // namespace gal::prometheus::gfx

namespace gal::prometheus::meta::user_defined
{
	template<>
	struct enum_is_flag<gfx::GlyphFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderListFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderRectFlag> : std::true_type {};

	template<>
	struct enum_is_flag<gfx::RenderArcFlag> : std::true_type {};
} // namespace gal::prometheus::meta::user_defined
