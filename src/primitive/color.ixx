// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.primitive:color;

import std;
import :multidimensional;

#else
#pragma once

#include <type_traits>
#include <tuple>
#include <format>

#include <prometheus/macro.hpp>
#include <primitive/multidimensional.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::primitive)
{
	using universal_32_bit_color_type = std::uint32_t;

	enum class ColorFormat
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

	template<ColorFormat>
	struct color_format_type {};

	template<ColorFormat Format>
	constexpr auto color_format = color_format_type<Format>{};

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct basic_color;

	template<typename>
	struct is_basic_color : std::false_type {};

	template<typename T>
	struct is_basic_color<basic_color<T>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_color_v = is_basic_color<T>::value;

	template<typename T>
	concept basic_color_t = is_basic_color_v<T>;

	template<typename T>
		requires std::is_arithmetic_v<T>
	struct [[nodiscard]] GAL_PROMETHEUS_COMPILER_EMPTY_BASE basic_color final : multidimensional<basic_color<T>, T>
	{
		using value_type = T;
		constexpr static auto is_integral_value = std::is_integral_v<value_type>;

		value_type alpha;
		value_type blue;
		value_type green;
		value_type red;

		constexpr explicit(false) basic_color(const value_type value = value_type{0}) noexcept
			: alpha{0},
			  blue{value},
			  green{value},
			  red{value} {}

		constexpr basic_color(const value_type red, const value_type green, const value_type blue, const value_type alpha) noexcept
			: alpha{alpha},
			  blue{blue},
			  green{green},
			  red{red} {}

		template<ColorFormat Format>
		constexpr basic_color(const universal_32_bit_color_type color, const color_format_type<Format>) noexcept
			: basic_color{}
		{
			*this = [color]() noexcept -> basic_color
			{
				constexpr auto to_value = [](const auto v) noexcept
				{
					if constexpr (is_integral_value)
					{
						return static_cast<value_type>(v);
					}
					else
					{
						return static_cast<value_type>(v) / static_cast<value_type>(255);
					}
				};

				if constexpr (Format == ColorFormat::R_G_B)
				{
					return
					{
							// red
							to_value((color >> 16) & 0xff),
							// green
							to_value((color >> 8) & 0xff),
							// blue
							to_value((color >> 0) & 0xff),
							// alpha
							to_value(0xff)
					};
				}
				else if constexpr (Format == ColorFormat::R_G_B_A)
				{
					return
					{
							// red
							to_value((color >> 24) & 0xff),
							// green
							to_value((color >> 16) & 0xff),
							// blue
							to_value((color >> 8) & 0xff),
							// alpha
							to_value((color >> 0) & 0xff)
					};
				}
				else if constexpr (Format == ColorFormat::A_R_G_B)
				{
					return
					{
							// red
							to_value((color >> 16) & 0xff),
							// green
							to_value((color >> 8) & 0xff),
							// blue
							to_value((color >> 0) & 0xff),
							// alpha
							to_value((color >> 24) & 0xff)
					};
				}
				else if constexpr (Format == ColorFormat::B_G_R)
				{
					return
					{
							// red
							to_value((color >> 0) & 0xff),
							// green
							to_value((color >> 8) & 0xff),
							// blue
							to_value((color >> 16) & 0xff),
							// alpha
							to_value(0xff)
					};
				}
				else if constexpr (Format == ColorFormat::B_G_R_A)
				{
					return
					{
							// red
							to_value((color >> 8) & 0xff),
							// green
							to_value((color >> 16) & 0xff),
							// blue
							to_value((color >> 24) & 0xff),
							// alpha
							to_value((color >> 0) & 0xff)
					};
				}
				else if constexpr (Format == ColorFormat::A_B_G_R)
				{
					return
					{
							// red
							to_value((color >> 0) & 0xff),
							// green
							to_value((color >> 8) & 0xff),
							// blue
							to_value((color >> 16) & 0xff),
							// alpha
							to_value((color >> 24) & 0xff)
					};
				}
				else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
			}();
		}

		[[nodiscard]] constexpr auto transparent() const noexcept -> basic_color
		{
			return {
					// red
					red,
					// green
					green,
					// blue
					blue,
					// alpha
					0
			};
		}

		template<ColorFormat Format>
		[[nodiscard]] constexpr auto to() const noexcept -> universal_32_bit_color_type
		{
			constexpr auto to_value = [](const auto v) noexcept
			{
				if constexpr (is_integral_value)
				{
					return static_cast<universal_32_bit_color_type>(v);
				}
				else
				{
					return static_cast<universal_32_bit_color_type>(v * 255);
				}
			};

			if constexpr (Format == ColorFormat::R_G_B)
			{
				return
						((to_value(red) & 0xff) << 16) |
						((to_value(green) & 0xff) << 8) |
						((to_value(blue) & 0xff) << 0);
			}
			else if constexpr (Format == ColorFormat::R_G_B_A)
			{
				return
						((to_value(red) & 0xff) << 24) |
						((to_value(green) & 0xff) << 16) |
						((to_value(blue) & 0xff) << 8) |
						((to_value(alpha) & 0xff) << 0);
			}
			else if constexpr (Format == ColorFormat::A_R_G_B)
			{
				return
						((to_value(red) & 0xff) << 16) |
						((to_value(green) & 0xff) << 8) |
						((to_value(blue) & 0xff) << 0) |
						((to_value(alpha) & 0xff) << 24);
			}
			else if constexpr (Format == ColorFormat::B_G_R)
			{
				return
						((to_value(red) & 0xff) << 0) |
						((to_value(green) & 0xff) << 8) |
						((to_value(blue) & 0xff) << 16);
			}
			else if constexpr (Format == ColorFormat::B_G_R_A)
			{
				return
						((to_value(red) & 0xff) << 8) |
						((to_value(green) & 0xff) << 16) |
						((to_value(blue) & 0xff) << 24) |
						((to_value(alpha) & 0xff) << 0);
			}
			else if constexpr (Format == ColorFormat::A_B_G_R)
			{
				return
						((to_value(red) & 0xff) << 0) |
						((to_value(green) & 0xff) << 8) |
						((to_value(blue) & 0xff) << 16) |
						((to_value(alpha) & 0xff) << 24);
			}
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		template<ColorFormat Format>
		[[nodiscard]] constexpr auto to(const color_format_type<Format>) const noexcept -> universal_32_bit_color_type //
		{
			return this->to<Format>();
		}
	};

	namespace colors
	{
		using color_type = basic_color<std::uint8_t>;
		constexpr auto build_color = [](const std::uint8_t red, const std::uint8_t green, const std::uint8_t blue) noexcept -> color_type
		{
			return {red, green, blue, 0xff};
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

		constexpr color_type gray[] =
		{
				build_color(0, 0, 0),
				build_color(3, 3, 3),
				build_color(5, 5, 5),
				build_color(8, 8, 8),
				build_color(10, 10, 10),
				build_color(13, 13, 13),
				build_color(15, 15, 15),
				build_color(18, 18, 18),
				build_color(20, 20, 20),
				build_color(23, 23, 23),
				build_color(26, 26, 26),
				build_color(28, 28, 28),
				build_color(31, 31, 31),
				build_color(33, 33, 33),
				build_color(36, 36, 36),
				build_color(38, 38, 38),
				build_color(41, 41, 41),
				build_color(43, 43, 43),
				build_color(46, 46, 46),
				build_color(48, 48, 48),
				build_color(51, 51, 51),
				build_color(54, 54, 54),
				build_color(56, 56, 56),
				build_color(59, 59, 59),
				build_color(61, 61, 61),
				build_color(64, 64, 64),
				build_color(66, 66, 66),
				build_color(69, 69, 69),
				build_color(71, 71, 71),
				build_color(74, 74, 74),
				build_color(77, 77, 77),
				build_color(79, 79, 79),
				build_color(82, 82, 82),
				build_color(84, 84, 84),
				build_color(87, 87, 87),
				build_color(89, 89, 89),
				build_color(92, 92, 92),
				build_color(94, 94, 94),
				build_color(97, 97, 97),
				build_color(99, 99, 99),
				build_color(102, 102, 102),
				build_color(105, 105, 105),
				build_color(107, 107, 107),
				build_color(110, 110, 110),
				build_color(112, 112, 112),
				build_color(115, 115, 115),
				build_color(117, 117, 117),
				build_color(120, 120, 120),
				build_color(122, 122, 122),
				build_color(125, 125, 125),
				build_color(127, 127, 127),
				build_color(130, 130, 130),
				build_color(133, 133, 133),
				build_color(135, 135, 135),
				build_color(138, 138, 138),
				build_color(140, 140, 140),
				build_color(143, 143, 143),
				build_color(145, 145, 145),
				build_color(148, 148, 148),
				build_color(150, 150, 150),
				build_color(153, 153, 153),
				build_color(156, 156, 156),
				build_color(158, 158, 158),
				build_color(161, 161, 161),
				build_color(163, 163, 163),
				build_color(166, 166, 166),
				build_color(168, 168, 168),
				build_color(171, 171, 171),
				build_color(173, 173, 173),
				build_color(176, 176, 176),
				build_color(179, 179, 179),
				build_color(181, 181, 181),
				build_color(184, 184, 184),
				build_color(186, 186, 186),
				build_color(189, 189, 189),
				build_color(191, 191, 191),
				build_color(194, 194, 194),
				build_color(196, 196, 196),
				build_color(199, 199, 199),
				build_color(201, 201, 201),
				build_color(204, 204, 204),
				build_color(207, 207, 207),
				build_color(209, 209, 209),
				build_color(212, 212, 212),
				build_color(214, 214, 214),
				build_color(217, 217, 217),
				build_color(219, 219, 219),
				build_color(222, 222, 222),
				build_color(224, 224, 224),
				build_color(227, 227, 227),
				build_color(229, 229, 229),
				build_color(232, 232, 232),
				build_color(235, 235, 235),
				build_color(237, 237, 237),
				build_color(240, 240, 240),
				build_color(242, 242, 242),
				build_color(245, 245, 245),
				build_color(247, 247, 247),
				build_color(250, 250, 250),
				build_color(252, 252, 252),
				build_color(255, 255, 255),
		};
	}
}

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE_STD
{
	template<std::size_t Index, typename T>
	struct
			#if defined(GAL_PROMETHEUS_COMPILER_MSVC)
			[[msvc::known_semantics]]
			#endif
			tuple_element<Index, gal::prometheus::primitive::basic_color<T>> // NOLINT(cert-dcl58-cpp)
	{
		using type = T;
	};

	template<typename T>
	struct tuple_size<gal::prometheus::primitive::basic_color<T>> // NOLINT(cert-dcl58-cpp)
			: std::integral_constant<std::size_t, 4> {};

	template<typename T>
	struct formatter<gal::prometheus::primitive::basic_color<T>> // NOLINT(cert-dcl58-cpp)
	{
		template<typename ParseContext>
		constexpr auto parse(ParseContext& context) const noexcept -> auto
		{
			(void)this;
			return context.begin();
		}

		template<typename FormatContext>
		auto format(const gal::prometheus::primitive::basic_color<T>& color, FormatContext& context) const noexcept -> auto
		{
			return std::format_to(
				context.out(),
				"#{:x}",
				color.template to<gal::prometheus::primitive::ColorFormat::A_R_G_B>()
			);
		}
	};
}
