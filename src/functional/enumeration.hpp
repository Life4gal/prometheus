// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <type_traits>
#include <utility>

#include <prometheus/macro.hpp>

// user_defined::enum_is_flag
#include <meta/enumeration.hpp>

// =============================================================================
// operator|

// flag | value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator|(const EnumType lhs, const ValueType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) | rhs; }
{
	return static_cast<EnumType>(std::to_underlying(lhs) | rhs);
}

// value | flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator|(const ValueType lhs, const EnumType rhs) noexcept -> ValueType //
	requires requires { lhs | std::to_underlying(rhs); }
{
	return static_cast<ValueType>(lhs | std::to_underlying(rhs));
}

// flag | flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator|(const EnumType lhs, const EnumType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) | std::to_underlying(rhs); }
{
	return static_cast<EnumType>(std::to_underlying(lhs) | std::to_underlying(rhs));
}

// flag |= value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator|=(EnumType& lhs, const ValueType rhs) noexcept -> EnumType& //
	requires requires { lhs | rhs; }
{
	lhs = lhs | rhs;
	return lhs;
}

// value |= flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator|=(ValueType& lhs, const EnumType rhs) noexcept -> ValueType& //
	requires requires { lhs | rhs; }
{
	lhs = lhs | rhs;
	return lhs;
}

// flag |= flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator|=(EnumType& lhs, const EnumType rhs) noexcept -> EnumType& //
	requires requires { lhs | rhs; }
{
	lhs = lhs | rhs;
	return lhs;
}

// =============================================================================
// operator&

// flag & value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator&(const EnumType lhs, const ValueType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) & rhs; }
{
	return static_cast<EnumType>(std::to_underlying(lhs) & rhs);
}

// value & flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator&(const ValueType lhs, const EnumType rhs) noexcept -> ValueType //
	requires requires { lhs & std::to_underlying(rhs); }
{
	return static_cast<ValueType>(lhs & std::to_underlying(rhs));
}

// flag & flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator&(const EnumType lhs, const EnumType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) & std::to_underlying(rhs); }
{
	return static_cast<EnumType>(std::to_underlying(lhs) & std::to_underlying(rhs));
}

// flag &= value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator&=(EnumType& lhs, const ValueType rhs) noexcept -> EnumType& //
	requires requires { lhs & rhs; }
{
	lhs = lhs & rhs;
	return lhs;
}

// value &= flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator&=(ValueType& lhs, const EnumType rhs) noexcept -> ValueType& //
	requires requires { lhs & rhs; }
{
	lhs = lhs & rhs;
	return lhs;
}

// flag &= flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator&=(EnumType& lhs, const EnumType rhs) noexcept -> EnumType& //
	requires requires { lhs & rhs; }
{
	lhs = lhs & rhs;
	return lhs;
}

// =============================================================================
// operator^

// flag ^ value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator^(const EnumType lhs, const ValueType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) ^ rhs; }
{
	return static_cast<EnumType>(std::to_underlying(lhs) ^ rhs);
}

// value ^ flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator^(const ValueType lhs, const EnumType rhs) noexcept -> ValueType //
	requires requires { lhs ^ std::to_underlying(rhs); }
{
	return static_cast<ValueType>(lhs ^ std::to_underlying(rhs));
}

// flag ^ flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator^(const EnumType lhs, const EnumType rhs) noexcept -> EnumType //
	requires requires { std::to_underlying(lhs) ^ std::to_underlying(rhs); }
{
	return static_cast<EnumType>(std::to_underlying(lhs) ^ std::to_underlying(rhs));
}

// flag ^= value => flag
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator^=(EnumType& lhs, const ValueType rhs) noexcept -> EnumType& //
	requires requires { lhs ^ rhs; }
{
	lhs = lhs ^ rhs;
	return lhs;
}

// value ^= flag => value
template<typename EnumType, std::integral ValueType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator^=(ValueType& lhs, const EnumType rhs) noexcept -> ValueType& //
	requires requires { lhs ^ rhs; }
{
	lhs = lhs ^ rhs;
	return lhs;
}

// flag ^= flag => flag
template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
constexpr auto operator^=(EnumType& lhs, const EnumType rhs) noexcept -> EnumType& //
	requires requires { lhs ^ rhs; }
{
	lhs = lhs ^ rhs;
	return lhs;
}

// =============================================================================
// operator~

template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator~(const EnumType e) noexcept -> EnumType //
	requires requires { ~std::to_underlying(e); }
{
	return static_cast<EnumType>(~std::to_underlying(e));
}

// =============================================================================
// operator!

template<typename EnumType>
	requires std::is_scoped_enum_v<EnumType> and gal::prometheus::meta::user_defined::enum_is_flag<EnumType>::value
[[nodiscard]] constexpr auto operator!(const EnumType e) noexcept -> bool //
	requires requires { !std::to_underlying(e); }
{
	return !std::to_underlying(e);
}

namespace gal::prometheus::functional
{
	template<typename EnumType>
		requires std::is_scoped_enum_v<EnumType>
	class EnumWrapper
	{
	public:
		using type = EnumType;
		using underlying_type = std::underlying_type_t<type>;

	private:
		constexpr static auto is_flag = meta::user_defined::enum_is_flag<type>::value;

		underlying_type value_;

	public:
		constexpr explicit (false) EnumWrapper(const underlying_type value) noexcept
			: value_{value} {}

		constexpr explicit (false) EnumWrapper(const type e) noexcept
			: EnumWrapper{std::to_underlying(e)} {}

		[[nodiscard]] constexpr explicit (false) operator underlying_type() const noexcept
		{
			return value_;
		}

		[[nodiscard]] constexpr explicit (false) operator type() const noexcept
		{
			return static_cast<type>(value_);
		}

		// =============================================================================
		// operator|

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator|(const EnumWrapper lhs, const ValueType rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() | rhs};
			}
			else
			{
				return {lhs.operator underlying_type() | rhs};
			}
		}

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator|(const ValueType lhs, const EnumWrapper rhs) noexcept -> ValueType
		{
			if constexpr (is_flag)
			{
				return lhs | rhs.operator type();
			}
			else
			{
				return lhs | rhs.operator underlying_type();
			}
		}

		[[nodiscard]] friend constexpr auto operator|(const EnumWrapper lhs, const type rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() | rhs};
			}
			else
			{
				return {lhs.operator underlying_type() | rhs};
			}
		}

		[[nodiscard]] friend constexpr auto operator|(const type lhs, const EnumWrapper rhs) noexcept -> type
		{
			if constexpr (is_flag)
			{
				return lhs | rhs.operator type();
			}
			else
			{
				return lhs | rhs.operator underlying_type();
			}
		}

		// =============================================================================
		// operator|=

		template<std::integral ValueType>
		friend constexpr auto operator|=(EnumWrapper& lhs, const ValueType rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs | rhs;
			return lhs;
		}

		template<std::integral ValueType>
		friend constexpr auto operator|=(ValueType& lhs, const EnumWrapper rhs) noexcept -> ValueType&
		{
			lhs = lhs | rhs;
			return lhs;
		}

		friend constexpr auto operator|=(EnumWrapper& lhs, const type rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs | rhs;
			return lhs;
		}

		friend constexpr auto operator|=(type& lhs, const EnumWrapper rhs) noexcept -> type&
		{
			lhs = lhs | rhs;
			return lhs;
		}

		// =============================================================================
		// operator&

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator&(const EnumWrapper lhs, const ValueType rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() & rhs};
			}
			else
			{
				return {lhs.operator underlying_type() & rhs};
			}
		}

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator&(const ValueType lhs, const EnumWrapper rhs) noexcept -> ValueType
		{
			if constexpr (is_flag)
			{
				return lhs & rhs.operator type();
			}
			else
			{
				return lhs & rhs.operator underlying_type();
			}
		}

		[[nodiscard]] friend constexpr auto operator&(const EnumWrapper lhs, const type rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() & rhs};
			}
			else
			{
				return {lhs.operator underlying_type() & rhs};
			}
		}

		[[nodiscard]] friend constexpr auto operator&(const type lhs, const EnumWrapper rhs) noexcept -> type
		{
			if constexpr (is_flag)
			{
				return lhs & rhs.operator type();
			}
			else
			{
				return lhs & rhs.operator underlying_type();
			}
		}

		// =============================================================================
		// operator&=

		template<std::integral ValueType>
		friend constexpr auto operator&=(EnumWrapper& lhs, const ValueType rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs & rhs;
			return lhs;
		}

		template<std::integral ValueType>
		friend constexpr auto operator&=(ValueType& lhs, const EnumWrapper rhs) noexcept -> ValueType&
		{
			lhs = lhs & rhs;
			return lhs;
		}

		friend constexpr auto operator&=(EnumWrapper& lhs, const type rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs & rhs;
			return lhs;
		}

		friend constexpr auto operator&=(type& lhs, const EnumWrapper rhs) noexcept -> type&
		{
			lhs = lhs & rhs;
			return lhs;
		}

		// =============================================================================
		// operator^

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator^(const EnumWrapper lhs, const ValueType rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() ^ rhs};
			}
			else
			{
				return {lhs.operator underlying_type() ^ rhs};
			}
		}

		template<std::integral ValueType>
		[[nodiscard]] friend constexpr auto operator^(const ValueType lhs, const EnumWrapper rhs) noexcept -> ValueType
		{
			if constexpr (is_flag)
			{
				return lhs ^ rhs.operator type();
			}
			else
			{
				return lhs ^ rhs.operator underlying_type();
			}
		}

		[[nodiscard]] friend constexpr auto operator^(const EnumWrapper lhs, const type rhs) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {lhs.operator type() ^ rhs};
			}
			else
			{
				return {lhs.operator underlying_type() ^ rhs};
			}
		}

		[[nodiscard]] friend constexpr auto operator^(const type lhs, const EnumWrapper rhs) noexcept -> type
		{
			if constexpr (is_flag)
			{
				return lhs ^ rhs.operator type();
			}
			else
			{
				return lhs ^ rhs.operator underlying_type();
			}
		}

		// =============================================================================
		// operator^=

		template<std::integral ValueType>
		friend constexpr auto operator^=(EnumWrapper& lhs, const ValueType rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs ^ rhs;
			return lhs;
		}

		template<std::integral ValueType>
		friend constexpr auto operator^=(ValueType& lhs, const EnumWrapper rhs) noexcept -> ValueType&
		{
			lhs = lhs ^ rhs;
			return lhs;
		}

		friend constexpr auto operator^=(EnumWrapper& lhs, const type rhs) noexcept -> EnumWrapper&
		{
			lhs = lhs ^ rhs;
			return lhs;
		}

		friend constexpr auto operator^=(type& lhs, const EnumWrapper rhs) noexcept -> type&
		{
			lhs = lhs ^ rhs;
			return lhs;
		}

		// =============================================================================
		// operator~

		[[nodiscard]] friend constexpr auto operator~(const EnumWrapper self) noexcept -> EnumWrapper
		{
			if constexpr (is_flag)
			{
				return {~self.operator type()};
			}
			else
			{
				return {~self.operator underlying_type()};
			}
		}

		// =============================================================================
		// operator~

		[[nodiscard]] friend constexpr auto operator!(const EnumWrapper self) noexcept -> bool
		{
			if constexpr (is_flag)
			{
				return !self.operator type();
			}
			else
			{
				return !self.operator underlying_type();
			}
		}
	};
}

namespace std
{
	template<typename EnumType>
	struct underlying_type<gal::prometheus::functional::EnumWrapper<EnumType>> // NOLINT(cert-dcl58-cpp)
	{
		using type = typename gal::prometheus::functional::EnumWrapper<EnumType>::underlying_type;
	};
}
