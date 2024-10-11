// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:io.device;

import std;

#if GAL_PROMETHEUS_COMPILER_DEBUG
import :error;
#endif

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <variant>
#include <cstdint>
#include <utility>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#endif

namespace gal::prometheus::io
{
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN

	enum class DeviceKey : std::uint16_t
	{
		NONE,

		// ========================================
		// keyboard <-------------------------------------------------------
		// from left to right, top to bottom
		// ========================================

		KB_ESCAPE,

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
		KB_I,
		KB_O,
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
		// Numeric Keypad

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

		// ========================================
		// -------------------------------------------------------> keyboard
		// ========================================

		// incoming
	};

	enum class MouseButton : std::uint32_t
	{
		LEFT = 0,
		MIDDLE = 1,
		RIGHT = 2,
		X1 = 3,
		X2 = 4
	};

	enum class MouseButtonStatus : std::uint32_t
	{
		PRESS,
		RELEASE,
	};

	enum class DeviceActionType : std::uint8_t
	{
		MOUSE_MOV = 0,
		MOUSE_BUTTON = 1,
		MOUSE_WHEEL = 2,

		TEXT = 3,
		KEY = 4,
	};

	class [[nodiscard]] DeviceEvent
	{
	public:
		struct mouse_mov_type
		{
			float x;
			float y;
		};

		struct mouse_button_type
		{
			MouseButton button;
			MouseButtonStatus status;
		};

		struct mouse_wheel_type
		{
			float x;
			float y;
		};

		struct text_type
		{
			std::uint32_t c;
			std::uint32_t pad;
		};

		struct key_type
		{
			DeviceKey key;
			bool down;
			float analog_value;

			static_assert(sizeof(DeviceKey) == 2);
			std::uint8_t pad;
		};

		using underlying_type = std::variant<mouse_mov_type, mouse_button_type, mouse_wheel_type, text_type, key_type>;
		static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(DeviceActionType::MOUSE_MOV), underlying_type>, mouse_mov_type>);
		static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(DeviceActionType::MOUSE_BUTTON), underlying_type>, mouse_button_type>);
		static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(DeviceActionType::MOUSE_WHEEL), underlying_type>, mouse_wheel_type>);
		static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(DeviceActionType::TEXT), underlying_type>, text_type>);
		static_assert(std::is_same_v<std::variant_alternative_t<static_cast<std::size_t>(DeviceActionType::KEY), underlying_type>, key_type>);

	private:
		underlying_type underlying_;

	public:
		template<typename T, typename... Args>
			requires std::is_constructible_v<underlying_type, std::in_place_type_t<T>, Args...>
		constexpr explicit DeviceEvent(std::in_place_type_t<T> type, Args&&... args) noexcept
			: underlying_{type, std::forward<Args>(args)...} {}

		template<typename... Args>
			requires std::is_constructible_v<underlying_type, Args...>
		constexpr explicit DeviceEvent(Args&&... args) noexcept
			: underlying_{std::forward<Args>(args)...} {}

		[[nodiscard]] constexpr auto type() const noexcept -> DeviceActionType
		{
			return static_cast<DeviceActionType>(underlying_.index());
		}

		// note: unchecked
		template<DeviceActionType Type>
		[[nodiscard]] constexpr auto value() const noexcept -> const auto&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(underlying_.index() == static_cast<std::size_t>(Type));
			return std::get<static_cast<std::size_t>(Type)>(underlying_);
		}

		template<typename Callable>
		constexpr auto visit(Callable&& callable) const noexcept -> void
		{
			std::visit(std::forward<Callable>(callable), underlying_);
		}
	};

	class DeviceEventQueue
	{
	public:
		template<typename T>
		using list_type = std::vector<T>;

		using queue_type = list_type<DeviceEvent>;
		using size_type = queue_type::size_type;

	private:
		queue_type queue_;

	public:
		[[nodiscard]] constexpr auto size() const noexcept -> size_type
		{
			return queue_.size();
		}

		[[nodiscard]] constexpr auto consume() noexcept -> DeviceEvent
		{
			const auto e = queue_.back();
			queue_.pop_back();
			return e;
		}

		constexpr auto mouse_move(const float x, const float y) noexcept -> void
		{
			// queue_.emplace_back(std::in_place_type<DeviceEvent::mouse_mov_type>, x, y);
			queue_.emplace_back(DeviceEvent::mouse_mov_type{.x = x, .y = y});
		}

		constexpr auto mouse_button(const MouseButton button, const MouseButtonStatus status) noexcept -> void
		{
			// queue_.emplace_back(std::in_place_type<DeviceEvent::mouse_button_type>, button, status);
			queue_.emplace_back(DeviceEvent::mouse_button_type{.button = button, .status = status});
		}

		constexpr auto mouse_wheel(const float x, const float y) noexcept -> void
		{
			// queue_.emplace_back(std::in_place_type<DeviceEvent::mouse_wheel_type>, x, y);
			queue_.emplace_back(DeviceEvent::mouse_wheel_type{.x = x, .y = y});
		}

		constexpr auto text(const std::uint32_t c) noexcept -> void
		{
			// queue_.emplace_back(std::in_place_type<DeviceEvent::text_type>, c);
			queue_.emplace_back(DeviceEvent::text_type{.c = c, .pad = 0});
		}

		constexpr auto key(const DeviceKey key, const bool down, const float analog_value) noexcept -> void
		{
			// queue_.emplace_back(std::in_place_type<DeviceEvent::key_type>, key, down, analog_value);
			queue_.emplace_back(DeviceEvent::key_type{.key = key, .down = down, .analog_value = analog_value, .pad = 0});
		}

		constexpr auto key(const DeviceKey key, const bool down) noexcept -> void
		{
			return this->key(key, down, down ? 1.f : 0.f);
		}
	};

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}
