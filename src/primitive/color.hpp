// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <tuple>
#include <format>

#include <prometheus/macro.hpp>

namespace gal::prometheus::primitive
{
	enum class ColorFormat : std::uint8_t
	{
		// red + green + blue
		R_G_B,
		// red + green + blue + alpha
		R_G_B_A,
		// alpha + red + green + blue
		A_R_G_B,
		// blue + green + red
		B_G_R,
		// blue + green + red + alpha
		B_G_R_A,
		// alpha + blue + green + red
		A_B_G_R,
	};

	namespace color_detail
	{
		template<ColorFormat>
		struct color_format_type {};
	}

	template<ColorFormat Format>
	constexpr auto color_format = color_detail::color_format_type<Format>{};

	using universal_32_bit_color_type = std::uint32_t;

	struct [[nodiscard]] basic_color final
	{
		using value_type = std::uint8_t;

		value_type red;
		value_type green;
		value_type blue;
		value_type alpha;

		template<ColorFormat Format = ColorFormat::R_G_B_A>
		constexpr static auto from(const universal_32_bit_color_type color, const color_detail::color_format_type<Format>  = {}) noexcept -> basic_color
		{
			const auto to_value = [](const auto value) noexcept -> value_type
			{
				return static_cast<value_type>(value & 0xff);
			};

			if constexpr (Format == ColorFormat::R_G_B)
			{
				return
				{
						.red = to_value(color >> 16),
						.green = to_value(color >> 8),
						.blue = to_value(color >> 0),
						.alpha = to_value(0xff)
				};
			}
			else if constexpr (Format == ColorFormat::R_G_B_A)
			{
				return
				{
						.red = to_value(color >> 24),
						.green = to_value(color >> 16),
						.blue = to_value(color >> 8),
						.alpha = to_value(color >> 0)
				};
			}
			else if constexpr (Format == ColorFormat::A_R_G_B)
			{
				return
				{
						.red = to_value(color >> 16),
						.green = to_value(color >> 8),
						.blue = to_value(color >> 0),
						.alpha = to_value(color >> 24)
				};
			}
			else if constexpr (Format == ColorFormat::B_G_R)
			{
				return
				{
						.red = to_value(color >> 0),
						.green = to_value(color >> 8),
						.blue = to_value(color >> 16),
						.alpha = to_value(0xff)
				};
			}
			else if constexpr (Format == ColorFormat::B_G_R_A)
			{
				return
				{
						.red = to_value(color >> 8),
						.green = to_value(color >> 16),
						.blue = to_value(color >> 24),
						.alpha = to_value(color >> 0)
				};
			}
			else if constexpr (Format == ColorFormat::A_B_G_R)
			{
				return
				{
						.red = to_value(color >> 0),
						.green = to_value(color >> 8),
						.blue = to_value(color >> 16),
						.alpha = to_value(color >> 24)
				};
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}

		[[nodiscard]] constexpr auto transparent() const noexcept -> basic_color
		{
			return
			{
					.red = red,
					.green = green,
					.blue = blue,
					.alpha = 0
			};
		}

		template<ColorFormat Format = ColorFormat::R_G_B_A>
		[[nodiscard]] constexpr auto to(const color_detail::color_format_type<Format>  = {}) const noexcept -> universal_32_bit_color_type
		{
			const auto to_value = [](const auto value) noexcept -> universal_32_bit_color_type
			{
				return static_cast<universal_32_bit_color_type>(value);
			};

			if constexpr (Format == ColorFormat::R_G_B)
			{
				return
						to_value(red << 16) |
						to_value(green << 8) |
						to_value(blue << 0);
			}
			else if constexpr (Format == ColorFormat::R_G_B_A)
			{
				return
						to_value(red << 24) |
						to_value(green << 16) |
						to_value(blue << 8) |
						to_value(alpha << 0);
			}
			else if constexpr (Format == ColorFormat::A_R_G_B)
			{
				return
						to_value(red << 16) |
						to_value(green << 8) |
						to_value(blue << 0) |
						to_value(alpha << 24);
			}
			else if constexpr (Format == ColorFormat::B_G_R)
			{
				return
						to_value(red << 0) |
						to_value(green << 8) |
						to_value(blue << 16);
			}
			else if constexpr (Format == ColorFormat::B_G_R_A)
			{
				return
						to_value(red << 8) |
						to_value(green << 16) |
						to_value(blue << 24) |
						to_value(alpha << 0);
			}
			else if constexpr (Format == ColorFormat::A_B_G_R)
			{
				return
						to_value(red << 0) |
						to_value(green << 8) |
						to_value(blue << 16) |
						to_value(alpha << 24);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	};

	static_assert(sizeof(basic_color) == sizeof(universal_32_bit_color_type));

	namespace colors
	{
		using color_type = basic_color;
		using value_type = color_type::value_type;

		constexpr auto build_color = [](const value_type red, const value_type green, const value_type blue) noexcept -> basic_color
		{
			return {.red = red, .green = green, .blue = blue, .alpha = 0xff};
		};

		constexpr color_type alice_blue = build_color(240, 248, 255);
		constexpr color_type antique_white = build_color(250, 235, 215);
		constexpr color_type aquamarine = build_color(50, 191, 193);
		constexpr color_type azure = build_color(240, 255, 255);
		constexpr color_type beige = build_color(245, 245, 220);
		constexpr color_type bisque = build_color(255, 228, 196);
		constexpr color_type black = build_color(0, 0, 0);
		constexpr color_type blanched_almond = build_color(255, 235, 205);
		constexpr color_type blue = build_color(0, 0, 255);
		constexpr color_type blue_violet = build_color(138, 43, 226);
		constexpr color_type brown = build_color(165, 42, 42);
		constexpr color_type burly_wood = build_color(222, 184, 135);
		constexpr color_type cadet_blue = build_color(95, 146, 158);
		constexpr color_type kchartreuse = build_color(127, 255, 0);
		constexpr color_type chocolate = build_color(210, 105, 30);
		constexpr color_type coral = build_color(255, 114, 86);
		constexpr color_type cornflower_blue = build_color(34, 34, 152);
		constexpr color_type corn_silk = build_color(255, 248, 220);
		constexpr color_type cyan = build_color(0, 255, 255);
		constexpr color_type dark_goldenrod = build_color(184, 134, 11);
		constexpr color_type dark_green = build_color(0, 86, 45);
		constexpr color_type dark_khaki = build_color(189, 183, 107);
		constexpr color_type dark_olive_green = build_color(85, 86, 47);
		constexpr color_type dark_orange = build_color(255, 140, 0);
		constexpr color_type dark_orchid = build_color(139, 32, 139);
		constexpr color_type dark_salmon = build_color(233, 150, 122);
		constexpr color_type dark_sea_green = build_color(143, 188, 143);
		constexpr color_type dark_slate_blue = build_color(56, 75, 102);
		constexpr color_type dark_slate_gray = build_color(47, 79, 79);
		constexpr color_type dark_turquoise = build_color(0, 166, 166);
		constexpr color_type dark_violet = build_color(148, 0, 211);
		constexpr color_type deep_pink = build_color(255, 20, 147);
		constexpr color_type deep_sky_blue = build_color(0, 191, 255);
		constexpr color_type dim_gray = build_color(84, 84, 84);
		constexpr color_type dodger_blue = build_color(30, 144, 255);
		constexpr color_type firebrick = build_color(142, 35, 35);
		constexpr color_type floral_white = build_color(255, 250, 240);
		constexpr color_type forest_green = build_color(80, 159, 105);
		constexpr color_type gains_boro = build_color(220, 220, 220);
		constexpr color_type ghost_white = build_color(248, 248, 255);
		constexpr color_type gold = build_color(218, 170, 0);
		constexpr color_type goldenrod = build_color(239, 223, 132);
		constexpr color_type green = build_color(0, 255, 0);
		constexpr color_type green_yellow = build_color(173, 255, 47);
		constexpr color_type honeydew = build_color(240, 255, 240);
		constexpr color_type hot_pink = build_color(255, 105, 180);
		constexpr color_type indian_red = build_color(107, 57, 57);
		constexpr color_type ivory = build_color(255, 255, 240);
		constexpr color_type khaki = build_color(179, 179, 126);
		constexpr color_type lavender = build_color(230, 230, 250);
		constexpr color_type lavender_blush = build_color(255, 240, 245);
		constexpr color_type lawn_green = build_color(124, 252, 0);
		constexpr color_type lemon_chiffon = build_color(255, 250, 205);
		constexpr color_type light_blue = build_color(176, 226, 255);
		constexpr color_type light_coral = build_color(240, 128, 128);
		constexpr color_type light_cyan = build_color(224, 255, 255);
		constexpr color_type light_goldenrod = build_color(238, 221, 130);
		constexpr color_type light_goldenrod_yellow = build_color(250, 250, 210);
		constexpr color_type light_gray = build_color(168, 168, 168);
		constexpr color_type light_pink = build_color(255, 182, 193);
		constexpr color_type light_salmon = build_color(255, 160, 122);
		constexpr color_type light_sea_green = build_color(32, 178, 170);
		constexpr color_type light_sky_blue = build_color(135, 206, 250);
		constexpr color_type light_slate_blue = build_color(132, 112, 255);
		constexpr color_type light_slate_gray = build_color(119, 136, 153);
		constexpr color_type light_steel_blue = build_color(124, 152, 211);
		constexpr color_type light_yellow = build_color(255, 255, 224);
		constexpr color_type lime_green = build_color(0, 175, 20);
		constexpr color_type linen = build_color(250, 240, 230);
		constexpr color_type magenta = build_color(255, 0, 255);
		constexpr color_type maroon = build_color(143, 0, 82);
		constexpr color_type medium_aquamarine = build_color(0, 147, 143);
		constexpr color_type medium_blue = build_color(50, 50, 204);
		constexpr color_type medium_forest_green = build_color(50, 129, 75);
		constexpr color_type medium_goldenrod = build_color(209, 193, 102);
		constexpr color_type medium_orchid = build_color(189, 82, 189);
		constexpr color_type medium_purple = build_color(147, 112, 219);
		constexpr color_type medium_sea_green = build_color(52, 119, 102);
		constexpr color_type medium_slate_blue = build_color(106, 106, 141);
		constexpr color_type medium_spring_green = build_color(35, 142, 35);
		constexpr color_type medium_turquoise = build_color(0, 210, 210);
		constexpr color_type medium_violet_red = build_color(213, 32, 121);
		constexpr color_type midnight_blue = build_color(47, 47, 100);
		constexpr color_type mint_cream = build_color(245, 255, 250);
		constexpr color_type misty_rose = build_color(255, 228, 225);
		constexpr color_type moccasin = build_color(255, 228, 181);
		constexpr color_type navajo_white = build_color(255, 222, 173);
		constexpr color_type navy = build_color(35, 35, 117);
		constexpr color_type navy_blue = build_color(35, 35, 117);
		constexpr color_type old_lace = build_color(253, 245, 230);
		constexpr color_type olive_drab = build_color(107, 142, 35);
		constexpr color_type orange = build_color(255, 135, 0);
		constexpr color_type orange_red = build_color(255, 69, 0);
		constexpr color_type orchid = build_color(239, 132, 239);
		constexpr color_type pale_goldenrod = build_color(238, 232, 170);
		constexpr color_type pale_green = build_color(115, 222, 120);
		constexpr color_type pale_turquoise = build_color(175, 238, 238);
		constexpr color_type pale_violet_red = build_color(219, 112, 147);
		constexpr color_type papaya_whip = build_color(255, 239, 213);
		constexpr color_type peach_puff = build_color(255, 218, 185);
		constexpr color_type peru = build_color(205, 133, 63);
		constexpr color_type pink = build_color(255, 181, 197);
		constexpr color_type plum = build_color(197, 72, 155);
		constexpr color_type powder_blue = build_color(176, 224, 230);
		constexpr color_type purple = build_color(160, 32, 240);
		constexpr color_type red = build_color(255, 0, 0);
		constexpr color_type rosy_brown = build_color(188, 143, 143);
		constexpr color_type royal_blue = build_color(65, 105, 225);
		constexpr color_type saddle_brown = build_color(139, 69, 19);
		constexpr color_type salmon = build_color(233, 150, 122);
		constexpr color_type sandy_brown = build_color(244, 164, 96);
		constexpr color_type sea_green = build_color(82, 149, 132);
		constexpr color_type sea_shell = build_color(255, 245, 238);
		constexpr color_type sienna = build_color(150, 82, 45);
		constexpr color_type sky_blue = build_color(114, 159, 255);
		constexpr color_type slate_blue = build_color(126, 136, 171);
		constexpr color_type slate_gray = build_color(112, 128, 144);
		constexpr color_type snow = build_color(255, 250, 250);
		constexpr color_type spring_green = build_color(65, 172, 65);
		constexpr color_type steel_blue = build_color(84, 112, 170);
		constexpr color_type tan = build_color(222, 184, 135);
		constexpr color_type thistle = build_color(216, 191, 216);
		constexpr color_type tomato = build_color(255, 99, 71);
		constexpr color_type transparent = build_color(0, 0, 1);
		constexpr color_type turquoise = build_color(25, 204, 223);
		constexpr color_type violet = build_color(156, 62, 206);
		constexpr color_type violet_red = build_color(243, 62, 150);
		constexpr color_type wheat = build_color(245, 222, 179);
		constexpr color_type white = build_color(255, 255, 255);
		constexpr color_type white_smoke = build_color(245, 245, 245);
		constexpr color_type yellow = build_color(255, 255, 0);
		constexpr color_type yellow_green = build_color(50, 216, 56);
	}
}

namespace std
{
	template<std::size_t Index>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_color> // NOLINT(cert-dcl58-cpp)
	{
		using type = gal::prometheus::primitive::basic_color::value_type;
	};

	template<>
	struct tuple_size<gal::prometheus::primitive::basic_color> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 4> {};

	template<>
	struct formatter<gal::prometheus::primitive::basic_color> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_color& color, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
				context.out(),
				"#{:x}",
				color.to<gal::prometheus::primitive::ColorFormat::A_R_G_B>()
			);
		}
	};
};
