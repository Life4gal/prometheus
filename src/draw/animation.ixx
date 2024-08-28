// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.draw:animation;

import std;
import gal.prometheus.primitive;
import gal.prometheus.functional;

#else
#pragma once

#include <functional>
#include <chrono>
#include <numbers>

#include <prometheus/macro.hpp>
#include <primitive/primitive.ixx>
#include <functional/functional.ixx>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::draw::impl)
{
	using clock_type = std::chrono::steady_clock;
	using time_point_type = std::chrono::time_point<clock_type>;
	using duration_type = std::chrono::duration<float>;

	using animation_parameter_type = duration_type;

	auto request_play_animation() noexcept -> void;

	namespace easing
	{
		using function_type = auto(*)(float) noexcept -> float;

		// y = x
		constexpr auto linear = [](const float p) noexcept -> float
		{
			return p;
		};

		// y = x^2
		constexpr auto quadratic_in = [](const float p) noexcept -> float
		{
			return functional::pow(p, 2);
		};

		// y = -x^2+2x
		constexpr auto quadratic_out = [](const float p) noexcept -> float
		{
			return -(p * (p - 2.f));
		};

		// y = (1/2)((2x)^2) <- [0, 0.5)
		// y = -(1/2)((2x-1)(2x-3)-1) <- [0.5, 1]
		constexpr auto quadratic_in_out = [](const float p) noexcept -> float
		{
			if (p < .5f)
			{
				// 2 * x^2
				return 2.f * quadratic_in(p);
			}

			// -2x^2 + 4x - 1
			return -2.f * quadratic_out(p) - 1.f;
		};

		// y = x^3
		constexpr auto cubic_in = [](const float p) noexcept -> float
		{
			return functional::pow(p, 3);
		};

		// y = (x-1)^3+1
		constexpr auto cubic_out = [](const float p) noexcept -> float
		{
			const auto t = p - 1.f;
			return functional::pow(t, 3) + 1;
		};

		// y = (1/2)((2x)^3) <- [0, 0.5)
		// y = (1/2)((2x-2)^3+2) <- [0.5, 1)
		constexpr auto cubic_in_out = [](const float p) noexcept -> float
		{
			if (p < .5f)
			{
				// 4x^3
				return 4.f * cubic_in(p);
			}

			// 4x^3-12x^2+12x-3
			// 4(x-1)^3+1
			return -3.f + 4 * cubic_out(p);
		};

		// y = x^4
		constexpr auto quartic_in = [](const float p) noexcept -> float
		{
			return functional::pow(p, 4);
		};

		// y = 1-(x-1)^4
		constexpr auto quartic_out = [](const float p) noexcept -> float
		{
			const auto t = p - 1.f;
			return 1.f - functional::pow(t, 4);
		};

		// y = (1/2)((2x)^4) <- [0, 0.5)
		// y = -(1/2)((2x-2)^4-2) <- [0.5, 1)
		constexpr auto quartic_in_out = [](const float p) noexcept -> float
		{
			if (p < .5f)
			{
				// 8x^4
				return 8 * quartic_in(p);
			}

			// -8x^4+32x^3-48x^2+32x-7
			// -8(x-1)^4+1
			return -7.f + 8 * quartic_out(p);
		};

		// y = x^5
		constexpr auto quintic_in = [](const float p) noexcept -> float
		{
			return functional::pow(p, 5);
		};

		// y = (x-1)^5+1
		constexpr auto quintic_out = [](const float p) noexcept -> float
		{
			const auto t = p - 1.f;
			return functional::pow(t, 5) + 1.f;
		};

		// y = (1/2)((2x)^5) <- [0, 0.5)
		// y = (1/2)((2x-2)^5+2) <- [0.5, 1)
		constexpr auto quintic_in_out = [](const float p) noexcept -> float
		{
			if (p < .5f)
			{
				// 16x^5
				return 16 * quintic_in(p);
			}

			// 16x^5 - 80x^4 + 160x^3 - 160x^2 + 80x - 15
			// 16(x-1)^5+1
			return 16.f * quintic_out(p) - 15.f;
		};

		// y = sin((x-1)*(PI/2))+1
		constexpr auto sine_in = [](const float p) noexcept -> float
		{
			return functional::sin((p - 1.f) * (std::numbers::pi_v<float> / 2)) + 1.f;
		};

		// y = sin(x*(PI/2))
		constexpr auto sine_out = [](const float p) noexcept -> float
		{
			return functional::sin(p * (std::numbers::pi_v<float> / 2));
		};

		// y = (1/2)(1-cos(x*PI))
		constexpr auto sine_in_out = [](const float p) noexcept -> float
		{
			return .5f * (1 - functional::cos(p * std::numbers::pi_v<float>));
		};

		// y = 1-sqrt(1-x^2)
		constexpr auto circular_in = [](const float p) noexcept -> float
		{
			return 1.f - functional::sqrt(1.f - functional::pow(p, 2));
		};

		// y = sqrt((2-x)*x)
		constexpr auto circular_out = [](const float p) noexcept -> float
		{
			return functional::sqrt((2.f - p) * p);
		};

		// y = (1/2)(1-sqrt(1-4x^2)) <- [0, 0.5)
		// y = (1/2)(sqrt(-(2x-3)*(2x-1))+1) <- [0.5, 1)
		constexpr auto circular_in_out = [](const float p) noexcept -> float
		{
			if (p < .5f)
			{
				return .5f * (1.f - functional::sqrt(1.f - 4.f * functional::pow(p, 2)));
			}

			return .5f * (functional::sqrt(-(2.f * p - 3.f) * (2.f * p - 1.f)) + 1.f);
		};
	}

	class Animator
	{
	public:
		using function_type = easing::function_type;

	private:
		std::reference_wrapper<float> ref_to_value_;
		float from_;
		float to_;
		duration_type duration_;
		function_type function_;
		duration_type current_;

	public:
		explicit Animator(
			float& from,
			const float to = .0f,
			const duration_type duration = std::chrono::milliseconds{250},
			const function_type easing_function = easing::linear,
			const duration_type delay = std::chrono::milliseconds{0}
		) noexcept
			: ref_to_value_{from},
			  from_{from},
			  to_{to},
			  duration_{duration},
			  function_{easing_function},
			  current_{-delay}
		{
			request_play_animation();
		}

		auto play(const animation_parameter_type parameter) noexcept -> void
		{
			current_ += parameter;

			if (current_ >= duration_)
			{
				ref_to_value_.get() = to_;
				return;
			}

			if (current_ <= duration_)
			{
				ref_to_value_.get() = from_;
			}
			else
			{
				ref_to_value_.get() = from_ + (to_ - from_) * function_(current_ / duration_);
			}

			request_play_animation();
		}
	};

	class AnimatedColorOption
	{
	public:
		using function_type = easing::function_type;
		using color_type = primitive::basic_color<std::uint8_t>;

	private:
		color_type inactive_;
		color_type active_;
		duration_type duration_;
		function_type function_;

	public:
		AnimatedColorOption() noexcept
			: duration_{},
			  function_{nullptr} {}

		AnimatedColorOption(
			const color_type inactive,
			const color_type active,
			const duration_type duration = std::chrono::milliseconds{250},
			const function_type function = easing::quadratic_in_out
		) noexcept
			: inactive_{inactive},
			  active_{active},
			  duration_{duration},
			  function_{function} {}

		[[nodiscard]] constexpr auto enabled() const noexcept -> bool
		{
			return function_ != nullptr;
		}

		[[nodiscard]] constexpr auto inactive() const noexcept -> color_type
		{
			return inactive_;
		}

		[[nodiscard]] constexpr auto active() const noexcept -> color_type
		{
			return active_;
		}

		[[nodiscard]] constexpr auto duration() const noexcept -> duration_type
		{
			return duration_;
		}

		[[nodiscard]] constexpr auto function() const noexcept -> function_type
		{
			return function_;
		}
	};

	class AnimatedColorsOption
	{
	public:
		using option_type = AnimatedColorOption;

	private:
		option_type background_;
		option_type foreground_;

	public:
		AnimatedColorsOption(const option_type& background, const option_type& foreground) noexcept
			: background_{background},
			  foreground_{foreground} {}

		[[nodiscard]] constexpr auto background() const noexcept -> const option_type&
		{
			return background_;
		}

		[[nodiscard]] constexpr auto foreground() const noexcept -> const option_type&
		{
			return foreground_;
		}
	};
}
