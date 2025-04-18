// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <array>
#include <vector>
#include <span>

#include <primitive/rect.hpp>
#include <primitive/color.hpp>
#include <primitive/vertex.hpp>

#include <i18n/range.hpp>
#include <functional/enumeration.hpp>

namespace gal::prometheus
{
	// ==================================================
	// TYPE & FLAG
	// ==================================================

	namespace gui
	{
		using rect_type = primitive::basic_rect_2d<float, float>;
		using point_type = rect_type::point_type;
		using extent_type = rect_type::extent_type;

		using uv_type = primitive::basic_point_2d<float>;
		using color_type = primitive::basic_color;

		using vertex_type = primitive::basic_vertex<point_type, uv_type, color_type>;
		using index_type = std::uint16_t;

		using texture_id_type = std::uintptr_t;
		using time_type = float;

		//------------------------------------------------------------------
		// FONT
		//------------------------------------------------------------------

		class FontOption
		{
		public:
			using value_type = extent_type::value_type;
			// todo: char32_t ?
			using char_type = char16_t;

			using glyph_value_type = i18n::RangeBuilder::value_type;
			using glyph_ranges_type = i18n::RangeBuilder::ranges_type;

			// todo
			constexpr static std::uint32_t default_baked_line_max_width = 63;

			std::string font_path{};
			glyph_ranges_type glyph_ranges{};
			std::uint32_t pixel_height{};

			std::uint32_t baked_line_max_width{default_baked_line_max_width};
			value_type scale{1.f};
			char_type fallback_char{u'?'};
			extent_type display_offset{.0f, .0f};
			point_type white_pixel_uv{-1, -1};
		};

		class [[nodiscard]] Texture final
		{
		public:
			using size_type = std::uint32_t;
			// size.width * size.height (RGBA)
			using data_type = std::unique_ptr<std::uint32_t[]>;

			size_type width;
			size_type height;
			data_type data;
			std::reference_wrapper<texture_id_type> id;

			explicit Texture(texture_id_type& texture_id) noexcept;

			Texture(const Texture& other) = delete;
			Texture(Texture&& other) noexcept = default;
			auto operator=(const Texture& other) -> Texture& = delete;
			auto operator=(Texture&& other) noexcept -> Texture& = default;

			~Texture() noexcept;

			[[nodiscard]] auto valid() const noexcept -> bool;

			auto bind(texture_id_type texture_id) noexcept -> void;
		};

		//------------------------------------------------------------------
		// THEME
		//------------------------------------------------------------------

		enum class ThemeCategory : std::uint8_t
		{
			TEXT = 0,

			BORDER,
			BORDER_SHADOW,

			WINDOW_BACKGROUND,

			TITLEBAR,
			TITLEBAR_COLLAPSED,

			RESIZE_GRIP,
			RESIZE_GRIP_HOVERED,
			RESIZE_GRIP_ACTIVATED,

			SCROLLBAR_BACKGROUND,
			SCROLLBAR_GRAB,
			SCROLLBAR_GRAB_HOVERED,
			SCROLLBAR_GRAB_ACTIVATED,

			CLOSE_BUTTON,
			CLOSE_BUTTON_HOVERED,
			CLOSE_BUTTON_ACTIVATED,

			TOOLTIP_BACKGROUND,
			TOOLTIP_TEXT,

			BUTTON,
			BUTTON_HOVERED,
			BUTTON_ACTIVATED,

			// RADIO-BUTTON / CHECKBOX / SLIDER
			FRAME_BACKGROUND,

			RADIO_BUTTON_HOVERED,
			RADIO_BUTTON_ACTIVATED,

			CHECKBOX_HOVERED,
			CHECKBOX_ACTIVATED,

			SLIDER,
			SLIDER_ACTIVATED,

			// -------------------------------
			INTERNAL_COUNT
		};

		constexpr auto theme_category_count = static_cast<std::size_t>(ThemeCategory::INTERNAL_COUNT);

		class Theme final
		{
		public:
			using value_type = extent_type::value_type;

			using color_type = primitive::basic_color;
			using alpha_type = value_type;

			using colors_type = std::array<color_type, theme_category_count>;

			// < 0
			constexpr static alpha_type window_fill_alpha_not_set{-.99999f};

			//-----------------
			// WINDOW
			//-----------------

			// Default alpha of window background
			value_type window_background_alpha;
			// Rounding of the window corners
			value_type window_corner_rounding;
			// Minimum size of the window
			extent_type window_min_size;
			// Size of the window resize grip
			extent_type window_resize_grip_size;
			// width: Minimum distance of the first element of each line from the left border of the window
			// height: Minimum distance of all elements of the first line from the upper border of the window (excluding the title bar)
			extent_type window_padding;
			// The extra padding space after adapting the window to the contents of the window (i.e., shrinking the window to just fit all the contents)
			extent_type window_auto_fit_padding;
			// Width of the vertical scrollbar
			value_type window_vertical_scrollbar_width;

			//-----------------
			// WINDOW CANVAS LAYOUT
			//-----------------

			// The factor of the line (window width) that an element takes up by default
			value_type item_default_width_factor;
			// Extra padding size for elements with borders (e.g. horizontal and vertical fill for text in button boxes)
			extent_type item_frame_padding;
			// Filling distance between two different elements when they are on the `same line`
			extent_type item_spacing;
			// If an element consists of more than one sub-element, the distance between these sub-elements is filled
			extent_type item_inner_spacing;

			//-----------------
			// WINDOW WIDGET COLOR
			//-----------------

			// Default alpha of all widget
			alpha_type alpha;
			colors_type colors;

			//-----------------
			// DRAW
			//-----------------

			// Maximum error (in pixels) allowed when using @c DrawList::circle and @c DrawList::circle_filled or drawing rounded corner rectangles with no explicit segment count specified
			// Decrease for higher quality but more geometry
			value_type circle_segment_max_error;
			// Tessellation tolerance when using @c DrawList::path_bezier_curve without a specific number of segments
			// Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality
			value_type draw_curve_tessellation_tolerance;
		};

		//------------------------------------------------------------------
		// IO
		//------------------------------------------------------------------

		enum class MouseKey : std::uint8_t
		{
			LEFT = 0,
			MIDDLE,
			RIGHT,
			X1,
			X2,

			// -------------------------------
			INTERNAL_COUNT
		};

		constexpr auto mouse_key_count = static_cast<std::size_t>(MouseKey::INTERNAL_COUNT);

		// ReSharper disable once CppInconsistentNaming
		class IO final
		{
		public:
			using value_type = extent_type::value_type;

			// Update once (it can also be updated every frame basis if desired)
			// Current window (viewport) size
			extent_type display_size{0, 0};
			// Update every frame
			// Time elapsed since last frame, in seconds
			// FPS = 60 ==> time_delta = 1/60
			time_type delta_time{1.f / 60.f};

			//-----------------
			// MOUSE
			//-----------------

			struct mouse_button_state_type
			{
				std::array<bool, mouse_key_count> state{};

				[[nodiscard]] constexpr auto operator[](const MouseKey key) noexcept -> bool&
				{
					return state[static_cast<std::size_t>(key)];
				}

				[[nodiscard]] constexpr auto operator[](const MouseKey key) const noexcept -> bool
				{
					return state[static_cast<std::size_t>(key)];
				}
			};

			// Update every frame
			// Current mouse position (this frame)
			point_type mouse_position{0, 0};
			// Update every frame
			// Current mouse wheel value
			value_type mouse_wheel{0};
			// Update every frame
			// Current mouse button state (this frame)
			mouse_button_state_type mouse_button_state{};

			// Update once (it can also be updated every frame basis if desired)
			// The interval between two clicks is less than this threshold to be considered as a double click, in seconds
			time_type mouse_double_click_interval_threshold{.3f};
			// Update once (it can also be updated every frame basis if desired)
			// The mouse position delta between two clicks is less than this threshold to be considered as a double click
			value_type mouse_double_click_distance_threshold{6};
			// Update once (it can also be updated every frame basis if desired)
			// When holding a button, time before it starts repeating, in seconds
			time_type mouse_repeat_click_delay{.275f};
			// Update once (it can also be updated every frame basis if desired)
			// When holding a button, rate at which it repeats, in seconds
			time_type mouse_repeat_click_rate{.05f};

			//-----------------
			// KEYBOARD
			//-----------------
		};

		//------------------------------------------------------------------
		// DRAW
		//------------------------------------------------------------------

		enum class DrawListFlag : std::uint8_t
		{
			NONE = 0,
			ANTI_ALIASED_LINE = 1 << 0,
			ANTI_ALIASED_LINE_USE_TEXTURE = 1 << 1,
			ANTI_ALIASED_FILL = 1 << 2,
		};

		class DrawData final
		{
		public:
			using size_type = std::uint32_t;

			struct [[nodiscard]] command_type
			{
				rect_type clip_rect;
				texture_id_type texture_id;

				// =======================

				// set by DrawList::index_list.size()
				// start offset in @c DrawList::index_list
				size_type index_offset;
				// set by DrawList::draw_xxx
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

		//------------------------------------------------------------------
		// WIDGET
		//------------------------------------------------------------------

		enum class WindowFlag : std::uint16_t
		{
			NONE = 0,

			BORDERED = 1 << 0,

			NO_TITLEBAR = 1 << 1,
			NO_CLOSE = 1 << 2,
			NO_RESIZE = 1 << 3,
			NO_MOVE = 1 << 4,
			NO_SCROLLBAR = 1 << 5,
			NO_SCROLLBAR_WITH_MOUSE = 1 << 6,

			AUTO_RESIZE = 1 << 7,
		};
	}

	namespace meta::user_defined
	{
		template<>
		struct enum_is_flag<gui::DrawListFlag> : std::true_type {};

		template<>
		struct enum_is_flag<gui::WindowFlag> : std::true_type {};
	}

	// ==================================================
	// CONTEXT + API
	// ==================================================

	namespace gui
	{
		//------------------------------------------------------------------
		// CONTEXT
		//------------------------------------------------------------------

		class Context;

		// Create context
		[[nodiscard]] auto create_context() noexcept -> Context*;
		// Destroy context
		auto destroy_context(Context& context) noexcept -> void;
		// Destroy context, for smart pointer
		auto destroy_context(Context* context) noexcept -> void;

		//------------------------------------------------------------------
		// FONT
		//------------------------------------------------------------------

		[[nodiscard]] auto set_default_font(Context& context, const FontOption& option) noexcept -> Texture;

		// [[nodiscard]] auto push_font(Context& context, const FontOption& option) noexcept -> Texture;
		// auto pop_font(Context& context) noexcept -> void;

		//------------------------------------------------------------------
		// THEME
		//------------------------------------------------------------------

		auto set_default_theme(Context& context, const Theme& theme) noexcept -> void;

		auto push_theme(Context& context, ThemeCategory category, Theme::color_type new_color) noexcept -> void;
		auto pop_theme(Context& context) noexcept -> void;

		//------------------------------------------------------------------
		// IO
		//------------------------------------------------------------------

		[[nodiscard]] auto get_io(Context& context) noexcept -> IO&;

		//------------------------------------------------------------------
		// DRAW
		//------------------------------------------------------------------

		auto set_default_draw_list_flag(Context& context, DrawListFlag flag) noexcept -> void;

		// auto push_draw_list_flag(Context& context, DrawListFlag new_flag) noexcept -> void;
		// auto pop_draw_list_flag(Context& context) noexcept -> void;

		auto new_frame(Context& context) noexcept -> void;
		auto end_frame(Context& context) noexcept -> void;
		auto render(Context& context) noexcept -> void;

		[[nodiscard]] auto get_draw_data(Context& context) noexcept -> std::vector<DrawData>;

		//------------------------------------------------------------------
		// WIDGET
		//------------------------------------------------------------------

		auto set_next_window_point(Context& context, const point_type& point) noexcept -> void;

		auto begin_window(
			Context& context,
			std::string_view name,
			const extent_type& size = {0, 0},
			Theme::value_type fill_alpha = Theme::window_fill_alpha_not_set,
			WindowFlag flag = WindowFlag::NONE
		) noexcept -> bool;
		auto end_window(Context& context) noexcept -> void;

		auto begin_child_window(
			Context& context,
			std::string_view name,
			const extent_type& size = {0, 0},
			bool border = false,
			WindowFlag flag = WindowFlag::NONE
		) noexcept -> void;
		auto end_child_window(Context& context) noexcept -> void;

		auto begin_tooltip_window(Context& context) noexcept -> void;
		auto end_tooltip_window(Context& context) noexcept -> void;

		/**
		 * @brief Draw a piece of text
		 */
		auto draw_text(Context& context, std::string_view utf8_text) noexcept -> void;

		/**
		 * @brief Draw a piece of text with specified color
		 */
		auto draw_text_colored(Context& context, std::string_view utf8_text, Theme::color_type color) noexcept -> void;

		/**
		 * @brief Draw a button, if the specified size is smaller than the size occupied by the text (plus padding), the text will not be completely inside the box
		 * @return Whether the button is pressed or not
		 */
		auto draw_button(Context& context, std::string_view utf8_text, const extent_type& size = {0, 0}, bool repeat_when_held = false) noexcept -> bool;

		/**
		 * @brief Draw a button, it fits within text without additional spacing
		 * @return Whether the button is pressed or not
		 */
		auto draw_small_button(Context& context, std::string_view utf8_text, bool repeat_when_held = false) noexcept -> bool;

		/**
		 * @brief Draw a radio button, its initial state is required
		 * @return Whether the radio button is pressed (being pressed does not mean that the state is switched, it can also be a repeat selection)
		 */
		auto draw_radio_button(Context& context, std::string_view utf8_text, bool checked) noexcept -> bool;

		/**
		 * @brief Draw a radio button, its initial state and referenced state is required
		 * @return Whether the radio button is pressed (being pressed does not mean that the state is switched, it can also be a repeat selection)
		 */
		template<std::equality_comparable T>
		auto draw_radio_button(Context& context, std::string_view utf8_text, T& reference, const std::type_identity_t<T>& identifier) noexcept -> bool
		{
			const auto prev_selected = reference == identifier;
			const auto pressed = gui::draw_radio_button(context, utf8_text, prev_selected);

			if (pressed)
			{
				reference = identifier;
			}
			return pressed;
		}

		/**
		 * @brief Draw a checkbox, its initial state is required
		 * @return Current state of the checkbox (checked or unchecked)
		 */
		auto draw_checkbox(Context& context, std::string_view utf8_text, bool checked) noexcept -> bool;

		/**
		 * @brief Draw a checkbox, its initial state and referenced state is required
		 * @return Current state of the checkbox (checked or unchecked)
		 */
		template<std::equality_comparable T>
		static auto draw_checkbox(
			Context& context,
			const std::string_view utf8_text,
			T& reference,
			const std::type_identity_t<T>& checked_identifier,
			const std::type_identity_t<T>& unchecked_identifier
		) noexcept -> bool
		{
			const auto prev_checked = reference == checked_identifier;
			const auto checked = gui::draw_checkbox(context, utf8_text, prev_checked);

			if (checked != prev_checked)
			{
				if (prev_checked)
				{
					reference = unchecked_identifier;
				}
				else
				{
					reference = checked_identifier;
				}
			}
			return checked;
		}

		auto draw_slider(
			Context& context,
			std::string_view utf8_text,
			float& reference,
			float min,
			float max,
			std::uint32_t decimal_precision = 3,
			float power = 1
		) noexcept -> bool;

		template<typename T, typename ValueType>
			requires std::is_arithmetic_v<ValueType>
		auto draw_slider(
			Context& context,
			const std::string_view utf8_text,
			T& reference,
			const ValueType min,
			const std::type_identity_t<ValueType> max,
			const std::uint32_t decimal_precision = 3,
			const float power = 1
		) noexcept -> bool
		{
			auto v = static_cast<float>(reference);

			const auto result = gui::draw_slider(
				context,
				utf8_text,
				v,
				static_cast<float>(min),
				static_cast<float>(max),
				decimal_precision,
				power
			);
			reference = v;

			return result;
		}

		template<meta::basic_fixed_string MemberName, typename T, typename ValueType>
			requires std::is_arithmetic_v<ValueType>
		auto draw_slider(
			Context& context,
			T&& object,
			const ValueType min,
			const std::type_identity_t<ValueType> max,
			const std::uint32_t decimal_precision = 3,
			const float power = 1
		) noexcept -> bool
		{
			auto& ref = meta::member_of_name<MemberName>(std::forward<T>(object));
			auto v = static_cast<float>(ref);

			const auto result = gui::draw_slider(
				context,
				MemberName,
				v,
				static_cast<float>(min),
				static_cast<float>(max),
				decimal_precision,
				power
			);
			ref = v;

			return result;
		}

		auto draw_slider_n(
			Context& context,
			std::string_view utf8_text,
			std::span<float> references,
			float min,
			float max,
			std::uint32_t decimal_precision = 3,
			float power = 1
		) noexcept -> bool;

		//------------------------------------------------------------------
		// WIDGET LAYOUT
		//------------------------------------------------------------------

		// < 0
		constexpr Theme::value_type layout_auto_size = -1;

		/**
		 * @brief Each element drawn will move the cursor to the next line of the canvas,
		 * this function forces the cursor to move to the end of the previous line (immediately after the previous element)
		 * @note @c column_width == @c layout_auto_size and @c spacing_width == @c layout_auto_size,
		 * the x-axis distance of the next drawn element from the previous element will depend on @c theme.item_spacing.width
		 * @note @c column_width == @c layout_auto_size and @c spacing_width != @c layout_auto_size,
		 * the x-axis distance of the next drawn element from the previous element will depend on @c spacing_width
		 * @note @c column_width != @c layout_auto_size (@c spacing_width can be any value, force to 0 if less than 0),
		 * the x-axis distance of the next drawn element from the previous element will depend on @c column_width + @c spacing_width
		 */
		auto layout_same_line(Context& context, Theme::value_type column_width = layout_auto_size, Theme::value_type spacing_width = layout_auto_size) noexcept -> void;

		//------------------------------------------------------------------
		// CANVAS LAYOUT
		//------------------------------------------------------------------

		auto push_item_width(Context& context, Theme::value_type new_item_width) noexcept -> void;
		auto pop_item_width(Context& context) noexcept -> void;

		auto push_text_wrap_width(Context& context, Theme::value_type new_wrap_width) noexcept -> void;
		auto pop_text_wrap_width(Context& context) noexcept -> void;

		//------------------------------------------------------------------
		// WIDGET STATE
		//------------------------------------------------------------------

		// The available area of the current drawing unit
		[[nodiscard]] auto get_content_region_max(const Context& context) noexcept -> extent_type;
		// The available area of the current window
		[[nodiscard]] auto get_window_content_region_min(const Context& context) noexcept -> extent_type;
		// The available area of the current window
		[[nodiscard]] auto get_window_content_region_max(const Context& context) noexcept -> extent_type;

		// Whether the last drawn item was hovered by the mouse
		[[nodiscard]] auto is_item_hovered(const Context& context) noexcept -> bool;
		// Whether the last drawn item was focused by the mouse
		[[nodiscard]] auto is_item_focused(const Context& context) noexcept -> bool;
	}

	// ==================================================
	// API
	// ==================================================

	namespace gui
	{
		//------------------------------------------------------------------
		// CONTEXT
		//------------------------------------------------------------------

		auto set_current_context(Context& context) noexcept -> void;
		auto get_current_context() noexcept -> Context&;

		auto create_current_context() noexcept -> void;
		auto destroy_current_context() noexcept -> void;

		//------------------------------------------------------------------
		// FONT
		//------------------------------------------------------------------

		[[nodiscard]] auto set_default_font(const FontOption& option) noexcept -> Texture;

		// [[nodiscard]] auto push_font(const FontOption& option) noexcept -> Texture;
		// auto pop_font() noexcept -> void;

		//------------------------------------------------------------------
		// THEME
		//------------------------------------------------------------------

		auto set_default_theme(const Theme& theme) noexcept -> void;

		auto push_theme(ThemeCategory category, Theme::color_type new_color) noexcept -> void;
		auto pop_theme() noexcept -> void;

		//------------------------------------------------------------------
		// IO
		//------------------------------------------------------------------

		[[nodiscard]] auto get_io() noexcept -> IO&;

		//------------------------------------------------------------------
		// DRAW
		//------------------------------------------------------------------

		auto set_default_draw_list_flag(DrawListFlag flag) noexcept -> void;

		// auto push_draw_list_flag(DrawListFlag new_flag) noexcept -> void;
		// auto pop_draw_list_flag() noexcept -> void;

		auto new_frame() noexcept -> void;
		auto end_frame() noexcept -> void;
		auto render() noexcept -> void;

		[[nodiscard]] auto get_draw_data() noexcept -> std::vector<DrawData>;

		//------------------------------------------------------------------
		// WIDGET
		//------------------------------------------------------------------

		auto set_next_window_point(const point_type& point) noexcept -> void;

		auto begin_window(
			std::string_view name,
			const extent_type& size = {0, 0},
			Theme::value_type fill_alpha = Theme::window_fill_alpha_not_set,
			WindowFlag flag = WindowFlag::NONE
		) noexcept -> bool;
		auto end_window() noexcept -> void;

		auto begin_child_window(
			std::string_view name,
			const extent_type& size = {0, 0},
			bool border = false,
			WindowFlag flag = WindowFlag::NONE
		) noexcept -> void;
		auto end_child_window() noexcept -> void;

		auto begin_tooltip_window() noexcept -> void;
		auto end_tooltip_window() noexcept -> void;

		/**
		 * @brief Draw a piece of text
		 */
		auto draw_text(std::string_view utf8_text) noexcept -> void;

		/**
		 * @brief Draw a piece of text with specified color
		 */
		auto draw_text_colored(std::string_view utf8_text, Theme::color_type color) noexcept -> void;

		/**
		 * @brief Draw a button, if the specified size is smaller than the size occupied by the text (plus padding), the text will not be completely inside the box
		 * @return Whether the button is pressed or not
		 */
		auto draw_button(std::string_view utf8_text, const extent_type& size = {0, 0}, bool repeat_when_held = false) noexcept -> bool;

		/**
		 * @brief Draw a button, it fits within text without additional spacing
		 * @return Whether the button is pressed or not
		 */
		auto draw_small_button(std::string_view utf8_text, bool repeat_when_held = false) noexcept -> bool;

		/**
		 * @brief Draw a radio button, its initial state is required
		 * @return Whether the radio button is pressed (being pressed does not mean that the state is switched, it can also be a repeat selection)
		 */
		auto draw_radio_button(std::string_view utf8_text, bool checked) noexcept -> bool;

		/**
		 * @brief Draw a radio button, its initial state and referenced state is required
		 */
		template<std::equality_comparable T>
		auto draw_radio_button(std::string_view utf8_text, T& reference, const std::type_identity_t<T>& identifier) noexcept -> void
		{
			auto& context = get_current_context();

			gui::draw_radio_button(context, utf8_text, reference, identifier);
		}

		/**
		 * @brief Draw a checkbox, its initial state is required
		 * @return Current state of the checkbox (checked or unchecked)
		 */
		auto draw_checkbox(std::string_view utf8_text, bool checked) noexcept -> bool;

		/**
		 * @brief Draw a checkbox, its initial state and referenced state is required
		 */
		template<std::equality_comparable T>
		static auto draw_checkbox(
			const std::string_view utf8_text,
			T& reference,
			const std::type_identity_t<T>& checked_identifier,
			const std::type_identity_t<T>& unchecked_identifier
		) noexcept -> void
		{
			auto& context = get_current_context();

			gui::draw_checkbox(context, utf8_text, reference, checked_identifier, unchecked_identifier);
		}

		auto draw_slider(
			std::string_view utf8_text,
			float& reference,
			float min,
			float max,
			std::uint32_t decimal_precision = 3,
			float power = 1
		) noexcept -> bool;

		template<typename T, meta::basic_fixed_string MemberName, typename ValueType>
			requires std::is_arithmetic_v<ValueType>
		auto draw_slider(
			T& object,
			const ValueType min,
			const std::type_identity_t<ValueType> max,
			const std::uint32_t decimal_precision = 3,
			const float power = 1
		) noexcept -> bool
		{
			auto& context = get_current_context();

			return gui::draw_slider<T, MemberName, ValueType>(context, object, min, max, decimal_precision, power);
		}

		auto draw_slider_n(
			std::string_view utf8_text,
			std::span<float> references,
			float min,
			float max,
			std::uint32_t decimal_precision = 3,
			float power = 1
		) noexcept -> bool;

		//------------------------------------------------------------------
		// WIDGET LAYOUT
		//------------------------------------------------------------------

		/**
		 * @brief Each element drawn will move the cursor to the next line of the canvas,
		 * this function forces the cursor to move to the end of the previous line (immediately after the previous element)
		 * @note @c column_width == @c auto_size and @c spacing_width == @c auto_size,
		 * the x-axis distance of the next drawn element from the previous element will depend on @c theme.item_spacing.width
		 * @note @c column_width == @c auto_size and @c spacing_width != @c auto_size,
		 * the x-axis distance of the next drawn element from the previous element will depend on @c spacing_width
		 * @note @c column_width != @c auto_size (@c spacing_width can be any value, force to 0 if less than 0),
		 * the x-axis distance of the next drawn element from the previous element will depend on @c column_width + @c spacing_width
		 */
		auto layout_same_line(Theme::value_type column_width = layout_auto_size, Theme::value_type spacing_width = layout_auto_size) noexcept -> void;

		//------------------------------------------------------------------
		// CANVAS LAYOUT
		//------------------------------------------------------------------

		auto push_item_width(Theme::value_type new_item_width) noexcept -> void;
		auto pop_item_width() noexcept -> void;

		auto push_text_wrap_width(Theme::value_type new_wrap_width) noexcept -> void;
		auto pop_text_wrap_width() noexcept -> void;

		//------------------------------------------------------------------
		// WIDGET STATE
		//------------------------------------------------------------------

		// The available area of the current drawing unit
		[[nodiscard]] auto get_content_region_max() noexcept -> extent_type;
		// The available area of the current window
		[[nodiscard]] auto get_window_content_region_min() noexcept -> extent_type;
		// The available area of the current window
		[[nodiscard]] auto get_window_content_region_max() noexcept -> extent_type;

		// Whether the last drawn item was hovered by the mouse
		[[nodiscard]] auto is_item_hovered() noexcept -> bool;
		// Whether the last drawn item was focused by the mouse
		[[nodiscard]] auto is_item_focused() noexcept -> bool;

		//------------------------------------------------------------------
		// FOR TEST
		//------------------------------------------------------------------

		// todo
		[[nodiscard]] auto test_theme() noexcept -> Theme;

		auto show_theme_editor() noexcept -> bool;
	}
}
