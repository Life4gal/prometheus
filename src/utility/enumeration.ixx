// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.utility:enumeration;

import std;
import gal.prometheus.error;

export namespace gal::prometheus::utility
{
	template<typename EnumType, typename NameType>
		requires std::is_enum_v<EnumType>
	struct enumeration_pair
	{
		using value_type = EnumType;
		using name_type = NameType;

		value_type value;
		name_type  name;
	};

	namespace enumeration_detail
	{
		template<typename NameType>
		struct enumeration_name
		{
			using type = std::decay_t<NameType>;
		};

		template<>
		struct enumeration_name<char*>
		{
			using type = std::basic_string_view<char>;
		};

		template<>
		struct enumeration_name<const char*>
		{
			using type = std::basic_string_view<char>;
		};

		template<std::size_t N>
		struct enumeration_name<char[N]>
		{
			using type = std::basic_string_view<char>;
		};

		template<std::size_t N>
		struct enumeration_name<const char[N]>
		{
			using type = std::basic_string_view<char>;
		};

		template<>
		struct enumeration_name<char8_t*>
		{
			using type = std::basic_string_view<char8_t>;
		};

		template<>
		struct enumeration_name<const char8_t*>
		{
			using type = std::basic_string_view<char8_t>;
		};

		template<std::size_t N>
		struct enumeration_name<char8_t[N]>
		{
			using type = std::basic_string_view<char8_t>;
		};

		template<std::size_t N>
		struct enumeration_name<const char8_t[N]>
		{
			using type = std::basic_string_view<char8_t>;
		};

		template<>
		struct enumeration_name<char16_t*>
		{
			using type = std::basic_string_view<char16_t>;
		};

		template<>
		struct enumeration_name<const char16_t*>
		{
			using type = std::basic_string_view<char16_t>;
		};

		template<std::size_t N>
		struct enumeration_name<char16_t[N]>
		{
			using type = std::basic_string_view<char16_t>;
		};

		template<std::size_t N>
		struct enumeration_name<const char16_t[N]>
		{
			using type = std::basic_string_view<char16_t>;
		};

		template<>
		struct enumeration_name<char32_t*>
		{
			using type = std::basic_string_view<char32_t>;
		};

		template<>
		struct enumeration_name<const char32_t*>
		{
			using type = std::basic_string_view<char32_t>;
		};

		template<std::size_t N>
		struct enumeration_name<char32_t[N]>
		{
			using type = std::basic_string_view<char32_t>;
		};

		template<std::size_t N>
		struct enumeration_name<const char32_t[N]>
		{
			using type = std::basic_string_view<char32_t>;
		};

		template<>
		struct enumeration_name<wchar_t*>
		{
			using type = std::basic_string_view<wchar_t>;
		};

		template<>
		struct enumeration_name<const wchar_t*>
		{
			using type = std::basic_string_view<wchar_t>;
		};

		template<std::size_t N>
		struct enumeration_name<wchar_t[N]>
		{
			using type = std::basic_string_view<wchar_t>;
		};

		template<std::size_t N>
		struct enumeration_name<const wchar_t[N]>
		{
			using type = std::basic_string_view<wchar_t>;
		};

		template<typename NameType>
		using enumeration_name_t = typename enumeration_name<NameType>::type;
	}

	template<typename EnumType, typename NameType>
	enumeration_pair(const EnumType&, const NameType&) -> enumeration_pair<EnumType, enumeration_detail::enumeration_name_t<NameType>>;

	template<typename EnumType, typename NameType, std::size_t Total>
		requires std::is_enum_v<EnumType> and (Total != 0)
	class Enumerations
	{
	public:
		using pair_type = enumeration_pair<EnumType, NameType>;

		using value_type = typename pair_type::value_type;
		using name_type = typename pair_type::name_type;

		constexpr static auto size = Total;

	private:
		using data_type = std::array<pair_type, size>;
		using size_type = typename data_type::size_type;

		[[nodiscard]] constexpr static auto to_underlying(const value_type e) noexcept -> std::underlying_type_t<value_type> { return static_cast<std::underlying_type_t<value_type>>(e); }

		bool                        contiguous_;
		std::array<pair_type, size> pairs_;

		template<std::size_t Index, typename... Reset>
		constexpr auto register_meta(value_type value, name_type name, const Reset&... reset_meta_data) noexcept -> void
		{
			pairs_[Index] = {.value = value, .name = std::move(name)};

			if constexpr (sizeof...(reset_meta_data) > 0) { this->template register_meta<Index + 1>(reset_meta_data...); }
		}

		[[nodiscard]] constexpr auto check_contiguous() const noexcept -> bool
		{
			return std::ranges::all_of(
					pairs_,
					[init = Enumerations::to_underlying(min())](const value_type e) mutable -> bool { return Enumerations::to_underlying(e) == init++; },
					[](const pair_type&                                          meta_data) -> value_type { return meta_data.value; });
		}

		[[nodiscard]] constexpr auto find(const value_type e) const noexcept -> const name_type*
		{
			if (contiguous_)
			{
				const auto index = Enumerations::to_underlying(e) - Enumerations::to_underlying(pairs_.front().value);
				return (index >= 0 && std::cmp_less(index, size)) ? &pairs_[static_cast<size_type>(index)].name : nullptr;
			}

			if (const auto it = std::ranges::find(
						pairs_,
						e,
						[](const pair_type& meta_data) -> value_type { return meta_data.value; });

				it != pairs_.end()) { return &it->name; }

			return nullptr;
		}

		template<typename Name>
		[[nodiscard]] constexpr auto find(const Name& name) const noexcept -> const value_type*
		{
			const auto it = std::ranges::find_if(
					pairs_,
					[&name](const name_type& n) -> bool { return name == n; },
					[](const pair_type&      meta_data) -> const name_type& { return meta_data.name; });

			if (it != pairs_.end()) { return &it->value; }

			return nullptr;
		}

	public:
		template<typename... Args>
		constexpr explicit(false) Enumerations(const Args&... meta_data) noexcept
			requires requires { this->template register_meta<0>(meta_data...); }
		{
			this->template register_meta<0>(meta_data...);

			std::ranges::sort(
					pairs_,
					[](const auto&      lhs, const auto& rhs) -> bool { return Enumerations::to_underlying(lhs) < Enumerations::to_underlying(rhs); },
					[](const pair_type& pair) -> value_type { return pair.value; });

			contiguous_ = check_contiguous();
		}

		[[nodiscard]] constexpr auto min() const noexcept -> value_type { return pairs_.front().value; }

		[[nodiscard]] constexpr auto max() const noexcept -> value_type { return pairs_.back().value; }

		[[nodiscard]] constexpr auto contiguous() const noexcept -> bool { return contiguous_; }

		/** 
	     * @brief Check if the enum has a value.
	     * @param e The value to lookup for the enum.
	     * @return True if the value is found.
	     */
		[[nodiscard]] constexpr auto contains(const value_type e) const noexcept -> bool { return this->find(e) != nullptr; }

		/** 
	     * @brief Check if the enum has a name.
	     * @param name The name to lookup in the enum.
	     * @return True if the name is found.
	     */
		[[nodiscard]] constexpr auto contains(const name_type& name) const noexcept -> bool { return this->find(name); }

		/** 
	     * @brief Check if the enum has a name.
	     * @param name The name to lookup in the enum.
	     * @return True if the name is found.
	     */
		template<typename Name>
			requires requires(const name_type& lhs, const Name& rhs)
			{
				{
					lhs == rhs
				} -> std::same_as<bool>;
			}
		[[nodiscard]] constexpr auto contains(const Name& name) const noexcept -> bool { return this->find(name) != nullptr; }

		/** 
	     * @brief Check if the enum has a name.
	     * @param name The name to lookup in the enum.
	     * @return True if the name is found.
	     */
		[[nodiscard]] constexpr auto contains(std::convertible_to<name_type> auto&& name) const -> bool
		{
			const name_type n{std::forward<decltype(name)>(name)};
			return this->contains(n);
		}

		/** 
	     * @brief Get a name from an enum-value.
	     * @param e The enum value to lookup.
	     * @return The name belonging with the enum value.
	     * @throws error::OutOfRangeError When the value does not exist.
	     */
		[[nodiscard]] constexpr auto at(const value_type e) const -> const name_type&
		{
			if (auto* it = this->find(e);
				it != nullptr) { return *it; }

			throw error::OutOfRangeError{"Enumerations::at"};
		}

		/** 
	     * @brief Get a name from an enum-value.
	     * @param e The enum value to lookup.
	     * @param default_name The default name to return when value is not found.
	     * @return The name belonging with the enum value.
	     */
		[[nodiscard]] constexpr auto at(const value_type e, const name_type& default_name) const noexcept -> const name_type&
		{
			if (auto* it = this->find(e);
				it != nullptr) { return *it; }

			return default_name;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     * @throws error::OutOfRangeError When the name does not exist.
	     */
		[[nodiscard]] constexpr auto at(const name_type& name) const -> value_type
		{
			if (auto* it = this->find(name);
				it != nullptr) { return *it; }

			throw error::OutOfRangeError{"Enumerations::at"};
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     * @throws error::OutOfRangeError When the name does not exist.
	     */
		template<typename Name>
			requires requires(const name_type& lhs, const Name& rhs)
			{
				{
					lhs == rhs
				} -> std::same_as<bool>;
			}
		[[nodiscard]] constexpr auto at(const Name& name) const -> value_type
		{
			if (auto* it = this->find(name);
				it != nullptr) { return *it; }

			throw error::OutOfRangeError{"Enumerations::at"};
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     * @throws OutOfRangeError When the name does not exist.
	     */
		template<typename Name>
			requires std::is_constructible_v<name_type, Name>
		[[nodiscard]] constexpr auto at(Name&& name) const -> value_type
		{
			const name_type n{std::forward<Name>(name)};
			return this->at(n);
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @param default_value The default value to return when the name is not found.
	     * @return The enum-value belonging with the name.
	     */
		[[nodiscard]] constexpr auto at(const name_type& name, const value_type default_value) const noexcept -> value_type
		{
			if (auto* it = this->find(name);
				it != nullptr) { return *it; }

			return default_value;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @param default_value The default value to return when the name is not found.
	     * @return The enum-value belonging with the name.
	     */
		template<typename Name>
			requires requires(const name_type& lhs, const Name& rhs)
			{
				{
					lhs == rhs
				} -> std::same_as<bool>;
			}
		[[nodiscard]] constexpr auto at(const Name& name, const value_type default_value) const noexcept -> value_type
		{
			if (auto* it = this->find(name);
				it != nullptr) { return *it; }

			return default_value;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @param name The name to lookup in the enum.
	     * @param default_value The default value to return when the name is not found.
	     * @return The enum-value belonging with the name.
	     */
		template<typename Name>
			requires std::is_constructible_v<name_type, Name>
		[[nodiscard]] constexpr auto at(Name&& name, const value_type default_value) const noexcept(std::is_nothrow_constructible_v<name_type, Name>) -> value_type
		{
			const name_type n{std::forward<Name>(name)};
			return this->at(n, default_value);
		}

		/** 
	     * @brief Get a name from an enum-value.
	     * @note It is undefined-behavior to lookup a value that does not exist in the table.
	     * @param e The enum value to lookup.
	     * @return The name belonging with the enum value.
	     */
		[[nodiscard]] constexpr auto operator[](const value_type e) const noexcept -> const name_type&
		{
			auto* it = this->find(e);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(it);
			return *it;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @note It is undefined-behavior to lookup a name that does not exist in the table.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     */
		[[nodiscard]] constexpr auto operator[](const name_type& name) const noexcept -> value_type
		{
			auto* it = this->find(name);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(it);
			return *it;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @note It is undefined-behavior to lookup a name that does not exist in the table.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     */
		template<typename Name>
			requires requires(const name_type& lhs, const Name& rhs)
			{
				{
					lhs == rhs
				} -> std::same_as<bool>;
			}
		[[nodiscard]] constexpr auto operator[](const Name& name) const noexcept -> value_type
		{
			auto* it = this->find(name);
			GAL_PROMETHEUS_DEBUG_NOT_NULL(it);
			return *it;
		}

		/** 
	     * @brief Get an enum-value from a name.
	     * @note It is undefined-behavior to lookup a name that does not exist in the table.
	     * @param name The name to lookup in the enum.
	     * @return The enum-value belonging with the name.
	     */
		template<typename Name>
			requires std::is_constructible_v<Name, name_type>
		[[nodiscard]] constexpr auto operator[](Name&& name) const noexcept(std::is_nothrow_constructible_v<name_type, Name>) -> value_type
		{
			const name_type n{std::forward<Name>(name)};
			return this->operator[](n);
		}
	};

	template<typename EnumType, typename NameType, typename... Reset>
	Enumerations(const EnumType&, const NameType&, const Reset&...) -> Enumerations<EnumType, enumeration_detail::enumeration_name_t<NameType>, (sizeof...(Reset) + 2) / 2>;
}// namespace gal::prometheus::utility
