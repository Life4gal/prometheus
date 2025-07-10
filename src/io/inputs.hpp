// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <chrono>
#include <initializer_list>
#include <variant>
#include <vector>
#include <span>
#include <mutex>

#include <primitive/point.hpp>
#include <primitive/extent.hpp>
#include <memory/reference_wrapper.hpp>

namespace gal::prometheus::io
{
	using position_type = primitive::basic_point_2d<float>;
	using extent_type = primitive::basic_extent_2d<float>;

	using duration_type = std::chrono::milliseconds;
	using time_point_type = std::chrono::time_point<std::chrono::system_clock, duration_type>;

	// ======================================================================
	// MOUSE
	// ======================================================================

	enum class MouseButton : std::uint8_t
	{
		LEFT = 0,
		MIDDLE,
		RIGHT,
		X1,
		X2,

		INTERNAL_COUNT,
		NONE,
	};

	constexpr auto total_mouse_button = static_cast<std::size_t>(MouseButton::INTERNAL_COUNT);

	enum class MouseWheel : std::uint8_t
	{
		HORIZONTAL = 0,
		VERTICAL,

		NONE,
	};

	// ======================================================================
	// KEYBOARD
	// ======================================================================

	// todo: keycode layout
	enum class KeyboardKeyCode : std::uint8_t
	{
		// ========================================
		// from left to right, top to bottom
		// ========================================

		KB_ESCAPE = 0,

		// ==================
		// function keys

		KB_F1,
		KB_F2,
		KB_F3,
		KB_F4,
		KB_F5,
		KB_F6,
		KB_F7,
		KB_F8,
		KB_F9,
		KB_F10,
		KB_F11,
		FK_F12,

		// ==================
		// main area

		// `
		KB_GRAVE_ACCENT,
		KB_1,
		KB_2,
		KB_3,
		KB_4,
		KB_5,
		KB_6,
		KB_7,
		KB_8,
		KB_9,
		KB_0,
		// - OR _
		KB_MINUS,
		// = OR +
		KB_PLUS,
		KB_BACKSPACE,

		KB_TAB,
		KB_Q,
		KB_W,
		KB_E,
		KB_R,
		KB_T,
		KB_Y,
		KB_U,
		KB_I, // NOLINT(misc-confusable-identifiers)
		KB_O, // NOLINT(misc-confusable-identifiers)
		KB_P,
		// [ OR {
		KB_LEFT_BRACKET,
		// ] OR }
		KB_RIGHT_BRACKET,
		// \ OR |
		KB_BACKSLASH,

		KB_CAPS_LOCK,
		KB_A,
		KB_S,
		KB_D,
		KB_F,
		KB_G,
		KB_H,
		KB_J,
		KB_K,
		KB_L,
		// ; OR :
		KB_SEMICOLON,
		// ' OR "
		KB_QUOTATION,
		KB_ENTER,

		KB_LEFT_SHIFT,
		KB_Z,
		KB_X,
		KB_C,
		KB_V,
		KB_B,
		KB_N,
		KB_M,
		// , OR <
		KB_COMMA,
		// . OR >
		KB_PERIOD,
		// / OR ?
		KB_SLASH,
		KB_RIGHT_SHIFT,

		KB_LEFT_CTRL,
		KB_LEFT_SUPER,
		KB_LEFT_ALT,
		KB_SPACE,
		KB_RIGHT_ALT,
		KB_RIGHT_SUPER,
		KB_MENU,
		KB_RIGHT_CTRL,

		// ==================
		// navigation keys

		KB_PAUSE,
		KB_SCROLL_LOCK,
		KB_PRINT_SCREEN,

		KB_INSERT,
		KB_HOME,
		KB_PAGE_UP,
		KB_DELETE,
		KB_END,
		KB_PAGE_DOWN,

		KB_ARROW_UP,
		KB_ARROW_LEFT,
		KB_ARROW_DOWN,
		KB_ARROW_RIGHT,

		// ==================
		// numeric keypad

		KB_KEYPAD_NUM_LOCK,
		// /
		KB_KEYPAD_DIVIDE,
		// *
		KB_KEYPAD_MULTIPLY,
		// -
		KB_KEYPAD_MINUS,

		KB_KEYPAD_7,
		KB_KEYPAD_8,
		KB_KEYPAD_9,
		KB_KEYPAD_PLUS,

		KB_KEYPAD_4,
		KB_KEYPAD_5,
		KB_KEYPAD_6,

		KB_KEYPAD_1,
		KB_KEYPAD_2,
		KB_KEYPAD_3,
		KB_KEYPAD_ENTER,

		KB_KEYPAD_0,
		KB_KEYPAD_DECIMAL,

		INTERNAL_COUNT,
		NONE,
	};

	constexpr auto total_keyboard_code = static_cast<std::size_t>(KeyboardKeyCode::INTERNAL_COUNT);

	// ======================================================================
	// ALL DEVICE
	// ======================================================================

	enum class DeviceKeyAction : std::uint8_t
	{
		NONE = 0,

		UP,
		DOWN,
	};

	enum class DeviceType : std::uint8_t
	{
		NONE = 0,

		MOUSE,
		KEYBOARD,
		DISPLAY,
	};

	// ======================================================================
	// MOUSE
	// ======================================================================

	// 8 bytes
	class MouseMoveEventData final
	{
	public:
		position_type position{0, 0};
	};

	// 2 bytes
	class MouseButtonEventData final
	{
	public:
		MouseButton button{MouseButton::NONE};
		DeviceKeyAction action{DeviceKeyAction::NONE};
	};

	// 8 bytes
	class MouseWheelEventData final
	{
	public:
		extent_type value;
	};

	// ======================================================================
	// KEYBOARD
	// ======================================================================

	// 2 bytes
	class KeyboardEventData final
	{
	public:
		KeyboardKeyCode code{KeyboardKeyCode::NONE};
		DeviceKeyAction action{DeviceKeyAction::NONE};
	};

	// ======================================================================
	// DISPLAY
	// ======================================================================

	// 8 bytes
	class DisplayMoveEventData final
	{
	public:
		position_type position{0, 0};
	};

	// 8 bytes
	class DisplayResizeEventData final
	{
	public:
		extent_type size{0, 0};
	};

	// ======================================================================
	// Handler
	// ======================================================================

	using input_event_type = std::variant<
		MouseMoveEventData,
		MouseButtonEventData,
		MouseWheelEventData,
		KeyboardEventData,
		DisplayMoveEventData,
		DisplayResizeEventData
	>;

	class InputHandler
	{
	public:
		using mutex_type = std::mutex;
		using event_queue_type = std::vector<input_event_type>;

		using value_type = extent_type::value_type;

	private:
		mutex_type mutex_;
		event_queue_type event_queue_;

		// ============
		// MOUSE
		// ============

		class Mouse final
		{
		public:
			struct press_record_type
			{
				time_point_type time_point{};
				position_type position{0, 0};
			};

			struct key_state_type
			{
				static_assert(sizeof(time_point_type) == sizeof(std::uint64_t));

				// Whether this key is pressed or not in this frame (depends on DeviceKeyAction), reset to 0 every frame
				std::uint64_t down_this_frame : 1 {0};
				// If pressed, the time of the moment of the press, this value is only updated the next time it is released, not reset every frame
				// fixme: If the runtime is too long, the 63-bit will not fully represent the 64-bit time values
				std::uint64_t down_time : 63 {0};

				std::vector<press_record_type> press_records{};
			};

			using mouse_states_type = std::array<key_state_type, total_mouse_button>;

			// Update every frame
			// Current mouse position (this frame)
			position_type position_current;
			// Update every frame
			// Previous mouse position (this frame)
			position_type position_previous;
			// Update every frame
			// Mouse distance between two frames
			extent_type position_delta;
			// Update every frame
			// Current mouse wheel value
			extent_type wheel;
			// Update every frame
			// Current mouse button state (this frame)
			mouse_states_type states;

			auto reset(const time_point_type& time_point, const InputHandler& handler) noexcept -> void;

			[[nodiscard]] auto is_up(const InputHandler& self, MouseButton button) const noexcept -> bool;
			[[nodiscard]] auto is_down(const InputHandler& self, MouseButton button) const noexcept -> bool;

			[[nodiscard]] auto is_clicked(const InputHandler& self, MouseButton button, bool repeat) const noexcept -> bool;
			[[nodiscard]] auto is_double_clicked(const InputHandler& self, MouseButton button) const noexcept -> bool;

			[[nodiscard]] auto is_pressing(const InputHandler& self, MouseButton button) const noexcept -> bool;
		};

		Mouse mouse_;

		// ============
		// KEYBOARD
		// ============

		class Keyboard final
		{
		public:
			struct press_record_type
			{
				time_point_type time_point{};
			};

			struct key_state_type
			{
				static_assert(sizeof(time_point_type) == sizeof(std::uint64_t));

				// Whether this key is pressed or not in this frame (depends on DeviceKeyAction), reset to 0 every frame
				std::uint64_t down_this_frame : 1 {0};
				// If pressed, the time of the moment of the press, this value is only updated the next time it is released, not reset every frame
				// fixme: If the runtime is too long, the 63-bit will not fully represent the 64-bit time values
				std::uint64_t down_time : 63 {0};

				std::vector<press_record_type> press_records{};
			};

			using keyboard_states_type = std::array<key_state_type, total_keyboard_code>;

			// Update every frame
			// Current keyboard button state (this frame)
			keyboard_states_type states;

			auto reset(const time_point_type& time_point, const InputHandler& handler) noexcept -> void;

			[[nodiscard]] auto is_up(const InputHandler& self, KeyboardKeyCode code) const noexcept -> bool;
			[[nodiscard]] auto is_down(const InputHandler& self, KeyboardKeyCode code) const noexcept -> bool;

			[[nodiscard]] auto is_pressing(const InputHandler& self, KeyboardKeyCode code) const noexcept -> bool;

			[[nodiscard]] auto is_combination_pressing(const InputHandler& self, std::span<const KeyboardKeyCode> codes) const noexcept -> bool;
			[[nodiscard]] auto is_combination_pressing(const InputHandler& self, std::initializer_list<const KeyboardKeyCode> codes) const noexcept -> bool;
		};

		Keyboard keyboard_;

		// ============
		// DISPLAY
		// ============

		struct display_data_type
		{
			// Update once (it can also be updated every frame basis if desired)
			// Current window (viewport) position
			position_type position;
			// Update once (it can also be updated every frame basis if desired)
			// Current window (viewport) size
			extent_type size;
		};

		display_data_type display_;

		// ============
		// CONFIG
		// ============

		// Update once (it can also be updated every frame basis if desired)
		// The interval between two clicks is less than this threshold to be considered as a double click, in seconds
		duration_type mouse_double_click_interval_threshold_;
		// Update once (it can also be updated every frame basis if desired)
		// The mouse position delta between two clicks is less than this threshold to be considered as a double click
		value_type mouse_double_click_distance_threshold_;
		// Update once (it can also be updated every frame basis if desired)
		// When holding a button, time before it starts repeating, in seconds
		duration_type mouse_repeat_click_delay_;
		// Update once (it can also be updated every frame basis if desired)
		// When holding a button, rate at which it repeats, in seconds
		duration_type mouse_repeat_click_rate_;

		auto process_event(const input_event_type& event) noexcept -> void;

	public:
		InputHandler(const InputHandler&) noexcept = delete;
		InputHandler(InputHandler&&) noexcept = delete;
		auto operator=(const InputHandler&) noexcept -> InputHandler& = delete;
		auto operator=(InputHandler&&) noexcept -> InputHandler& = delete;
		~InputHandler() noexcept = default;

		InputHandler() noexcept;

		auto begin_frame() noexcept -> void;

		auto end_frame() noexcept -> void;

		auto push_event(const input_event_type& event) noexcept -> void;

		// ============
		// MOUSE
		// ============

		struct [[nodiscard]] mouse_proxy final
		{
			memory::RefWrapper<const InputHandler> self;

			[[nodiscard]] auto position() const noexcept -> position_type;
			[[nodiscard]] auto position_delta() const noexcept -> extent_type;
			[[nodiscard]] auto wheel() const noexcept -> extent_type;

			[[nodiscard]] auto is_up(MouseButton button) const noexcept -> bool;
			[[nodiscard]] auto is_down(MouseButton button) const noexcept -> bool;

			[[nodiscard]] auto is_click(MouseButton button, bool repeat = false) const noexcept -> bool;
			[[nodiscard]] auto is_double_click(MouseButton button) const noexcept -> bool;

			[[nodiscard]] auto is_pressing(MouseButton button) const noexcept -> bool;
		};

		[[nodiscard]] auto mouse() const noexcept -> mouse_proxy;

		// ============
		// KEYBOARD
		// ============

		struct [[nodiscard]] keyboard_proxy final
		{
			memory::RefWrapper<const InputHandler> self;

			[[nodiscard]] auto is_up(KeyboardKeyCode code) const noexcept -> bool;
			[[nodiscard]] auto is_down(KeyboardKeyCode code) const noexcept -> bool;

			[[nodiscard]] auto is_pressing(KeyboardKeyCode code) const noexcept -> bool;

			[[nodiscard]] auto is_combination_pressing(std::span<const KeyboardKeyCode> codes) const noexcept -> bool;
			[[nodiscard]] auto is_combination_pressing(std::initializer_list<const KeyboardKeyCode> codes) const noexcept -> bool;
		};

		[[nodiscard]] auto keyboard() const noexcept -> keyboard_proxy;

		// ============
		// DISPLAY
		// ============

		struct [[nodiscard]] display_proxy final
		{
			memory::RefWrapper<const InputHandler> self;

			[[nodiscard]] auto position() const noexcept -> position_type;
			[[nodiscard]] auto size() const noexcept -> extent_type;
		};

		[[nodiscard]] auto display() const noexcept -> display_proxy;

		// ============
		// CONFIG
		// ============

		struct [[nodiscard]] config_proxy final
		{
			memory::RefWrapper<InputHandler> self;

			auto set_mouse_double_click_interval_threshold(duration_type interval) noexcept -> void;
			auto set_mouse_double_click_distance_threshold(value_type distance) noexcept -> void;
			auto set_mouse_repeat_click_delay(duration_type delay) noexcept -> void;
			auto set_mouse_repeat_click_rate(duration_type rate) noexcept -> void;
		};

		[[nodiscard]] auto config() noexcept -> config_proxy;
	};
}
