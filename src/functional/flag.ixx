// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.functional:flag;

import std;

#else
#pragma once

#include <type_traits>

#include <prometheus/macro.hpp>

#endif

GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_NAMESPACE(gal::prometheus::functional)
{
	namespace operators
	{
		template<typename EnumType, typename Enum>
			requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		[[nodiscard]] constexpr auto operator|(const EnumType lhs, const Enum rhs) noexcept -> EnumType
		{
			return static_cast<EnumType>(lhs | std::to_underlying(rhs));
		}

		template<typename EnumType, typename Enum>
			requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		[[nodiscard]] constexpr auto operator|(const Enum lhs, const EnumType rhs) noexcept -> Enum
		{
			return static_cast<Enum>(std::to_underlying(lhs) | rhs);
		}

		template<typename Enum>
			requires std::is_scoped_enum_v<Enum>
		[[nodiscard]] constexpr auto operator|(const Enum lhs, const Enum rhs) noexcept -> Enum
		{
			return static_cast<Enum>(std::to_underlying(lhs) | std::to_underlying(rhs));
		}

		template<typename EnumType, typename Enum>
			requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		[[nodiscard]] constexpr auto operator&(const EnumType lhs, const Enum rhs) noexcept -> EnumType
		{
			return static_cast<EnumType>(lhs & std::to_underlying(rhs));
		}

		template<typename EnumType, typename Enum>
			requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		[[nodiscard]] constexpr auto operator&(const Enum lhs, const EnumType rhs) noexcept -> Enum
		{
			return static_cast<Enum>(std::to_underlying(lhs) & rhs);
		}

		template<typename Enum>
			requires std::is_scoped_enum_v<Enum>
		[[nodiscard]] constexpr auto operator&(const Enum lhs, const Enum rhs) noexcept -> Enum
		{
			return static_cast<Enum>(std::to_underlying(lhs) & std::to_underlying(rhs));
		}
	}

	enum class EnumCheckPolicy
	{
		// contains: (flag & e) == e
		// exclude: (flag & e) == 0
		ALL_BITS,
		// contains: (flag & e) != 0
		// exclude: (flag & e) != e
		ANY_BIT,
	};

	enum class EnumFoldPolicy
	{
		LOGICAL_OR,
		LOGICAL_AND,
	};

	template<EnumCheckPolicy CheckPolicy = EnumCheckPolicy::ANY_BIT, EnumFoldPolicy FoldPolicy = EnumFoldPolicy::LOGICAL_AND, typename EnumType, typename Enum, std::same_as<Enum>... Enums>
	// requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		requires std::is_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
	[[nodiscard]] constexpr auto contains(const EnumType flag, const Enum e, const Enums... es) noexcept -> bool
	{
		if constexpr (sizeof...(es) == 0)
		{
			using namespace operators;
			if constexpr (CheckPolicy == EnumCheckPolicy::ALL_BITS)
			{
				return (e & flag) == e;
			}
			else if constexpr (CheckPolicy == EnumCheckPolicy::ANY_BIT)
			{
				return (flag & e) != 0;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
		else
		{
			if constexpr (FoldPolicy == EnumFoldPolicy::LOGICAL_AND)
			{
				return contains(flag, e) and contains(flag, es...);
			}
			else if constexpr (FoldPolicy == EnumFoldPolicy::LOGICAL_OR)
			{
				return contains(flag, e) or contains(flag, es...);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	}

	template<EnumCheckPolicy CheckPolicy = EnumCheckPolicy::ANY_BIT, EnumFoldPolicy FoldPolicy = EnumFoldPolicy::LOGICAL_AND, typename Enum, std::same_as<Enum>... Enums>
	// requires std::is_scoped_enum_v<Enum>
		requires std::is_enum_v<Enum>
	[[nodiscard]] constexpr auto contains(const Enum flag, const Enum e, const Enums... es) noexcept -> bool
	{
		return contains<CheckPolicy, FoldPolicy>(std::to_underlying(flag), e, es...);
	}

	template<EnumCheckPolicy CheckPolicy = EnumCheckPolicy::ALL_BITS, EnumFoldPolicy FoldPolicy = EnumFoldPolicy::LOGICAL_AND, typename EnumType, typename Enum, std::same_as<Enum>... Enums>
	// requires std::is_scoped_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
		requires std::is_enum_v<Enum> and std::is_convertible_v<EnumType, std::underlying_type_t<Enum>>
	[[nodiscard]] constexpr auto exclude(const EnumType flag, const Enum e, const Enums... es) noexcept -> bool
	{
		if constexpr (sizeof...(es) == 0)
		{
			using namespace operators;
			if constexpr (CheckPolicy == EnumCheckPolicy::ALL_BITS)
			{
				return (flag & e) == 0;
			}
			else if constexpr (CheckPolicy == EnumCheckPolicy::ANY_BIT)
			{
				return (flag & e) != e;
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
		else
		{
			if constexpr (FoldPolicy == EnumFoldPolicy::LOGICAL_AND)
			{
				return exclude(flag, e) and exclude(flag, es...);
			}
			else if constexpr (FoldPolicy == EnumFoldPolicy::LOGICAL_OR)
			{
				return exclude(flag, e) or exclude(flag, es...);
			}
			else
			{
				GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
			}
		}
	}

	template<EnumCheckPolicy CheckPolicy = EnumCheckPolicy::ALL_BITS, EnumFoldPolicy FoldPolicy = EnumFoldPolicy::LOGICAL_AND, typename Enum, std::same_as<Enum>... Enums>
	// requires std::is_scoped_enum_v<Enum> 
		requires std::is_enum_v<Enum>
	[[nodiscard]] constexpr auto exclude(const Enum flag, const Enum e, const Enums... es) noexcept -> bool
	{
		return exclude<CheckPolicy, FoldPolicy>(std::to_underlying(flag), e, es...);
	}
}
