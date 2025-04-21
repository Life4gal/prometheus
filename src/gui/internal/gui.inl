// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

namespace gal::prometheus::gui
{
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

	template<std::equality_comparable T>
	auto draw_checkbox(
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

	template<typename T, typename ValueType>
		requires std::is_arithmetic_v<ValueType>
	auto draw_slider(
		Context& context,
		const std::string_view utf8_text,
		T& reference,
		const ValueType min,
		const std::type_identity_t<ValueType> max,
		const std::uint32_t decimal_precision,
		const float power
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
		const std::uint32_t decimal_precision,
		const float power
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
}
