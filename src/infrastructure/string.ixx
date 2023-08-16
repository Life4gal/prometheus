// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:string;

import std;

namespace gal::prometheus::infrastructure
{
	template<typename Derived, typename ValueType, typename SizeType>
	struct string_base
	{
		using derived_type = std::remove_cvref_t<Derived>;

		using value_type = ValueType;
		using size_type = SizeType;

		// clang-format off
		template<typename Container, typename Getter>
		struct is_getter : std::conditional_t<std::is_invocable_v<Getter, const Container&, size_type>, std::true_type, std::false_type> {};

		template<typename Container, typename Getter>
		constexpr static auto is_getter_v = is_getter<Container, Getter>::value;

		template<typename Container, typename Getter, typename Comparator>
		struct is_comparator : std::conditional_t<std::is_invocable_v<Comparator, std::invoke_result_t<Getter, const Container&, size_type>, value_type>, std::true_type, std::false_type> {};

		template<typename Container, typename Getter, typename Comparator>
		constexpr static auto is_comparator_v = is_comparator<Container, Getter, Comparator>::value;
		// clang-format on

		template<typename Container>
			requires requires(const Container& c)
			{
				c[std::declval<size_type>()];
			}
		struct default_getter
		{
			[[nodiscard]] constexpr auto operator()(const Container& c, const size_type index) noexcept(noexcept(c[index])) -> decltype(auto) { return c[index]; }
		};

		template<typename Container>
		struct default_comparator
		{
			using element = decltype(default_getter<Container>{}(std::declval<const Container&>(), std::declval<size_type>()));

			[[nodiscard]] constexpr auto operator()(element lhs, const value_type rhs) noexcept(noexcept(lhs == rhs)) -> bool { return lhs == rhs; }
		};

		[[nodiscard]] constexpr explicit(false) operator std::basic_string_view<value_type>() const noexcept { return std::basic_string_view<value_type>{derived_type::value, derived_type::size}; }

		[[nodiscard]] constexpr static auto as_string_view() noexcept -> std::basic_string_view<value_type> { return std::basic_string_view<value_type>{derived_type::value, derived_type::size}; }

		[[nodiscard]] constexpr explicit operator std::basic_string<value_type>() const noexcept { return std::basic_string<value_type>{derived_type::value, derived_type::size}; }

		[[nodiscard]] constexpr static auto as_string() noexcept -> std::basic_string<value_type> { return std::basic_string<value_type>{derived_type::value, derived_type::size}; }

		template<typename Char = value_type>
		[[nodiscard]] constexpr static auto match(const Char* string) noexcept -> bool requires requires
		{
			derived_type::size;

			{
				std::char_traits<value_type>::length(string)
			} -> std::same_as<size_type>;
			std::char_traits<value_type>::compare(derived_type::value, string, derived_type::size);
		}
		{
			return std::char_traits<value_type>::length(string) == derived_type::size and
					std::char_traits<value_type>::compare(derived_type::value, string, derived_type::size) == 0;
		}

		template<typename Char = value_type>
		[[nodiscard]] constexpr auto match(const Char* string) const noexcept -> bool
			//
			requires std::is_member_function_pointer_v<decltype(&derived_type::size)> and
					requires
					{
						{
							std::char_traits<value_type>::length(string)
						} -> std::same_as<size_type>;
						std::char_traits<value_type>::compare(std::declval<const value_type*>(), string, derived_type::size);
					}
		{
			return std::char_traits<value_type>::length(string) == static_cast<const derived_type&>(*this).size() and
					std::char_traits<value_type>::compare(static_cast<const derived_type&>(*this).value, string, static_cast<const derived_type&>(*this).size()) == 0;
		}

		template<typename Container, typename Getter, typename Comparator>
			requires is_getter_v<Container, Getter> and is_comparator_v<Container, Getter, Comparator> and
					requires(const Container& container)
					{
						{
							container.size()
						} -> std::same_as<size_type>;
					}
		[[nodiscard]] constexpr static auto match(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(
				noexcept(container.size()) and
				std::is_nothrow_invocable_v<Getter, const Container&, size_type> and
				std::is_nothrow_invocable_v<Comparator, std::invoke_result_t<Getter, const Container&, size_type>, value_type>
			) -> bool
		{
			return
					container.size() == derived_type::size and
					[&]<std::size_t... Index>(std::index_sequence<Index...>)
					{
						//
						return ((comparator(getter(container, Index), derived_type::value[Index])) and ...);
					}(std::make_index_sequence<derived_type::size>{});
		}

		template<typename Container, typename Getter, typename Comparator>
			requires is_getter_v<Container, Getter> and is_comparator_v<Container, Getter, Comparator> and
					std::is_member_function_pointer_v<decltype(&derived_type::size)> and
					requires(const Container& container)
					{
						{
							container.size()
						} -> std::same_as<size_type>;
					}
		[[nodiscard]] constexpr auto match(
				const Container& container,
				Getter           getter,
				Comparator       comparator) const
		//
			noexcept(
				noexcept(container.size()) and
				std::is_nothrow_invocable_v<Getter, const Container&, size_type> and
				std::is_nothrow_invocable_v<Comparator, std::invoke_result_t<Getter, const Container&, size_type>, value_type>
			) -> bool
		{
			if (container.size() != static_cast<const derived_type&>(*this).size()) { return false; }

			for (decltype(container.size()) i = 0; i < container.size(); ++i) { if (not comparator(getter(container, i), static_cast<const derived_type&>(*this).value[i])) { return false; } }

			return true;
		}

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&string_base::match<Container, default_getter<Container>, Comparator>),
				const Container&,
				default_getter<Container>,
				Comparator>
		[[nodiscard]] constexpr static auto match(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
				decltype(&string_base::match<Container, default_getter<Container>, Comparator>),
				const Container&,
				default_getter<Container>,
				Comparator>) -> bool
		{
			return string_base::match(
					container,
					default_getter<Container>{},
					comparator);
		}

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&string_base::match<Container, default_getter<Container>, Comparator>),
				const string_base&,
				const Container&,
				default_getter<Container>,
				Comparator>
		[[nodiscard]] constexpr auto match(
				const Container& container,
				Comparator       comparator) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&string_base::match<Container, default_getter<Container>, Comparator>),
					const string_base&,
					const Container&,
					default_getter<Container>,
					Comparator>
			) -> bool
		{
			return this->match(
					container,
					default_getter<Container>{},
					comparator);
		}

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&string_base::match<Container, default_getter<Container>, default_comparator<Container>>),
				const Container&,
				default_getter<Container>,
				default_comparator<Container>>
		[[nodiscard]] constexpr static auto match(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&string_base::match<Container, default_getter<Container>, default_comparator<Container>>),
					const Container&,
					default_getter<Container>,
					default_comparator<Container>>
			) -> bool
		{
			return string_base::match(
					container,
					default_comparator<Container>{});
		}

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&string_base::match<Container, default_getter<Container>, default_comparator<Container>>),
				const string_base&,
				const Container&,
				default_getter<Container>,
				default_comparator<Container>>
		[[nodiscard]] constexpr auto match(const Container& container) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&string_base::match<Container, default_getter<Container>, default_comparator<Container>>),
					const string_base&,
					const Container&,
					default_getter<Container>,
					default_comparator<Container>>
			) -> bool
		{
			return this->match(
					container,
					default_comparator<Container>{});
		}
	};

	template<typename Derived, typename ValueType, typename SizeType, typename LeftType, typename RightType>
		requires std::is_base_of_v<string_base<LeftType, ValueType, SizeType>, LeftType> and std::is_base_of_v<string_base<RightType, ValueType, SizeType>, RightType>
	struct bilateral_string_base
	{
		using derived_type = Derived;

		using left_type = LeftType;
		using right_type = RightType;

		static_assert(std::is_same_v<ValueType, typename left_type::value_type>);
		static_assert(std::is_same_v<ValueType, typename right_type::value_type>);
		static_assert(std::is_same_v<SizeType, typename left_type::size_type>);
		static_assert(std::is_same_v<SizeType, typename right_type::size_type>);

		using value_type = ValueType;
		using size_type = SizeType;

		template<typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Char>),
				const Char*>
		[[nodiscard]] constexpr static auto match_left(const Char* string)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Char>),
					const Char*>
			) -> bool { return left_type::template match<Char>(string); }

		template<typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Char>),
				const Char*>
		[[nodiscard]] constexpr static auto match_right(const Char* string)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Char>),
					const Char*>
			) -> bool { return right_type::template match<Char>(string); }

		template<typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Char>),
				const left_type&,
				const Char*>
		[[nodiscard]] constexpr auto match_left(const Char* string) const
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Char>),
					const left_type&,
					const Char*>
			) -> bool { return static_cast<const derived_type&>(*this).left_value.template match<Char>(string); }

		template<typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Char>),
				const right_type&,
				const Char*>
		[[nodiscard]] constexpr auto match_right(const Char* string) const
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Char>),
					const right_type&,
					const Char*>
			) -> bool { return static_cast<const derived_type&>(*this).right_value.template match<Char>(string); }

		template<typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container, Getter, Comparator>),
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr static auto match_left(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container, Getter, Comparator>),
					const Container&,
					Getter,
					Comparator>
			) -> bool { return left_type::template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container, Getter, Comparator>),
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr static auto match_right(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container, Getter, Comparator>),
					const Container&,
					Getter,
					Comparator>
			) -> bool { return right_type::template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container, Getter, Comparator>),
				const left_type&,
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr auto match_left(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container, Getter, Comparator>),
					const left_type&,
					const Container&,
					Getter,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).left_value.template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container, Getter, Comparator>),
				const right_type&,
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr auto match_right(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container, Getter, Comparator>),
					const right_type&,
					const Container&,
					Getter,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).right_value.template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container, Comparator>),
				const Container&,
				Comparator>
		[[nodiscard]] constexpr static auto match_left(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container, Comparator>),
					const Container&,
					Comparator>
			) -> bool { return left_type::template match<Container, Comparator>(container, comparator); }

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container, Comparator>),
				const Container&,
				Comparator>
		[[nodiscard]] constexpr static auto match_right(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container, Comparator>),
					const Container&,
					Comparator>
			) -> bool { return right_type::template match<Container, Comparator>(container, comparator); }

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container, Comparator>),
				const left_type&,
				const Container&,
				Comparator>
		[[nodiscard]] constexpr auto match_left(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container, Comparator>),
					const left_type&,
					const Container&,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).left_value.template match<Container, Comparator>(container, comparator); }

		template<typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container, Comparator>),
				const right_type&,
				const Container&,
				Comparator>
		[[nodiscard]] constexpr auto match_right(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container, Comparator>),
					const right_type&,
					const Container&,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).right_value.template match<Container, Comparator>(container, comparator); }

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container>),
				const Container&>
		[[nodiscard]] constexpr static auto match_left(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container>),
					const Container&>
			) -> bool { return left_type::template match<Container>(container); }

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container>),
				const Container&>
		[[nodiscard]] constexpr static auto match_right(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container>),
					const Container&>
			) -> bool { return right_type::template match<Container>(container); }

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&left_type::template match<Container>),
				const left_type&,
				const Container&>
		[[nodiscard]] constexpr auto match_left(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&left_type::template match<Container>),
					const left_type&,
					const Container&>
			) -> bool { return static_cast<const derived_type&>(*this).left_value.template match<Container>(container); }

		template<typename Container>
			requires std::is_invocable_v<
				decltype(&right_type::template match<Container>),
				const right_type&,
				const Container&>
		[[nodiscard]] constexpr auto match_right(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&right_type::template match<Container>),
					const right_type&,
					const Container&>
			) -> bool { return static_cast<const derived_type&>(*this).right_value.template match<Container>(container); }
	};

	template<typename Derived, typename ValueType, typename SizeType, typename... Strings>
		requires(sizeof...(Strings) >= 1) and (std::is_base_of_v<string_base<Strings, ValueType, SizeType>, Strings> and ...)
	struct multiple_string_base
	{
		using derived_type = Derived;

		using multiple_type = std::tuple<Strings...>;
		using multiple_size_type = std::tuple_size<multiple_type>;
		constexpr static std::size_t multiple_size = multiple_size_type::value;

		template<std::size_t Index>
			requires(Index < multiple_size)
		using subtype = std::tuple_element_t<Index, multiple_type>;

		using value_type = ValueType;
		using size_type = SizeType;

		static_assert((std::is_same_v<ValueType, typename Strings::value_type> and ...));
		static_assert((std::is_same_v<SizeType, typename Strings::size_type> and ...));

		template<std::size_t Index, typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Char>),
				const Char*>
		[[nodiscard]] constexpr static auto match(const Char* string)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Char>),
					const Char*>
			) -> bool { return subtype<Index>::template match<Char>(string); }

		template<std::size_t Index, typename Char = value_type>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Char>),
				const subtype<Index>&,
				const Char*>
		[[nodiscard]] constexpr auto match(const Char* string) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Char>),
					const subtype<Index>&,
					const Char*>
			) -> bool { return static_cast<const derived_type&>(*this).template value<Index>().template match<Char>(string); }

		template<std::size_t Index, typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container, Getter, Comparator>),
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr static auto match(
				const Container& container,
				Getter           getter,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container, Getter, Comparator>),
					const Container&,
					Getter,
					Comparator>
			) -> bool { return subtype<Index>::template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<std::size_t Index, typename Container, typename Getter, typename Comparator>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container, Getter, Comparator>),
				const subtype<Index>&,
				const Container&,
				Getter,
				Comparator>
		[[nodiscard]] constexpr auto match(
				const Container& container,
				Getter           getter,
				Comparator       comparator) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container, Getter, Comparator>),
					const subtype<Index>&,
					const Container&,
					Getter,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).template value<Index>().template match<Container, Getter, Comparator>(container, getter, comparator); }

		template<std::size_t Index, typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container, Comparator>),
				const Container&,
				Comparator>
		[[nodiscard]] constexpr static auto match(
				const Container& container,
				Comparator       comparator)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container, Comparator>),
					const Container&,
					Comparator>
			) -> bool { return subtype<Index>::template match<Container, Comparator>(container, comparator); }

		template<std::size_t Index, typename Container, typename Comparator>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container, Comparator>),
				const subtype<Index>&,
				const Container&,
				Comparator>
		[[nodiscard]] constexpr auto match(
				const Container& container,
				Comparator       comparator) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container, Comparator>),
					const subtype<Index>&,
					const Container&,
					Comparator>
			) -> bool { return static_cast<const derived_type&>(*this).template value<Index>().template match<Container, Comparator>(container, comparator); }

		template<std::size_t Index, typename Container>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container>),
				const Container&>
		[[nodiscard]] constexpr static auto match(const Container& container)
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container>),
					const Container&>
			) -> bool { return subtype<Index>::template match<Container>(container); }

		template<std::size_t Index, typename Container>
			requires std::is_invocable_v<
				decltype(&subtype<Index>::template match<Container>),
				const subtype<Index>&,
				const Container&>
		[[nodiscard]] constexpr auto match(const auto& container) const
		//
			noexcept(std::is_nothrow_invocable_v<
					decltype(&subtype<Index>::template match<Container>),
					const subtype<Index>&,
					const Container&>
			) -> bool { return static_cast<const derived_type&>(*this).template value<Index>().template match<Container>(container); }
	};

	// basic_fixed_string
	export
	{
		template<typename T>
		using basic_fixed_string_view = std::basic_string_view<T>;

		template<typename T, std::size_t N>
		struct basic_fixed_string;

		template<typename>
		struct is_fixed_string : std::false_type {};

		template<typename T, std::size_t N>
		struct is_fixed_string<basic_fixed_string<T, N>> : std::true_type {};

		template<typename T, std::size_t N>
		struct basic_fixed_string : string_base<basic_fixed_string<T, N>, T, std::size_t>
		{
			using value_type = T;
			using size_type = std::size_t;

			using iterator = const value_type*;
			using const_iterator = const value_type*;

			constexpr static size_type max_size{N};
			constexpr static size_type size{max_size};

			value_type value[max_size];

			constexpr basic_fixed_string() noexcept
				: value{} {}

			template<std::size_t M>
			constexpr explicit(false) basic_fixed_string(const value_type (&string)[M]) noexcept
				requires (M >= N) { std::ranges::copy(std::ranges::begin(string), std::ranges::begin(string) + N, value); }

			constexpr explicit(false) operator basic_fixed_string_view<value_type>() const noexcept { return basic_fixed_string_view<value_type>{value, size}; }

			template<size_type L, size_type R>
			friend constexpr auto operator==(const basic_fixed_string<value_type, L>& lhs, const basic_fixed_string<value_type, R>& rhs) noexcept -> bool { return lhs.operator basic_fixed_string_view<value_type>() == rhs.operator basic_fixed_string_view<value_type>(); }

			template<typename String>
				requires (not is_fixed_string<String>::value) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			friend constexpr auto operator==(const basic_fixed_string& lhs, const String& rhs) noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> bool { return lhs.operator basic_fixed_string_view<value_type>() == basic_fixed_string_view<value_type>{rhs}; }

			template<typename String>
				requires (not is_fixed_string<String>::value) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			friend constexpr auto operator==(const String& lhs, const basic_fixed_string& rhs) noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> bool { return basic_fixed_string_view<value_type>{lhs} == rhs.operator basic_fixed_string_view<value_type>(); }

			template<size_type L, size_type R>
			friend constexpr auto operator<=>(const basic_fixed_string<value_type, L>& lhs, const basic_fixed_string<value_type, R>& rhs) noexcept -> auto { return lhs.operator basic_fixed_string_view<value_type>() <=> rhs.operator basic_fixed_string_view<value_type>(); }

			template<typename String>
				requires (not is_fixed_string<String>::value) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			friend constexpr auto operator<=>(const basic_fixed_string& lhs, const String& rhs) noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> auto { return lhs.operator basic_fixed_string_view<value_type>() <=> basic_fixed_string_view<value_type>{rhs}; }

			template<typename String>
				requires (not is_fixed_string<String>::value) and std::is_constructible_v<basic_fixed_string_view<value_type>, String>
			friend constexpr auto operator<=>(const String& lhs, const basic_fixed_string& rhs) noexcept(std::is_nothrow_constructible_v<basic_fixed_string_view<value_type>, String>) -> auto { return basic_fixed_string_view<value_type>{lhs} <=> rhs.operator basic_fixed_string_view<value_type>(); }
		};

		template<typename, typename>
		struct basic_bilateral_fixed_string;

		template<typename T, std::size_t Left, std::size_t Right>
		struct basic_bilateral_fixed_string<basic_fixed_string<T, Left>, basic_fixed_string<T, Right>>
				: bilateral_string_base<
					basic_bilateral_fixed_string<basic_fixed_string<T, Left>, basic_fixed_string<T, Right>>,
					typename basic_fixed_string<T, Left>::value_type,
					typename basic_fixed_string<T, Left>::size_type,
					basic_fixed_string<T, Left>,
					basic_fixed_string<T, Right>
				>
		{
			using left_type = basic_fixed_string<T, Left>;
			using right_type = basic_fixed_string<T, Right>;

			using value_type = typename left_type::value_type;
			using size_type = typename left_type::size_type;

			using iterator = typename left_type::iterator;
			using const_iterator = typename left_type::const_iterator;

			left_type  left_value;
			right_type right_value;

			[[nodiscard]] constexpr const_iterator left_begin() noexcept { return left_value.begin(); }

			[[nodiscard]] constexpr const_iterator left_end() noexcept { return left_value.end(); }

			[[nodiscard]] constexpr const_iterator right_begin() noexcept { return right_value.begin(); }

			[[nodiscard]] constexpr const_iterator right_end() noexcept { return right_value.end(); }
		};

		template<typename, typename>
		struct basic_bilateral_fixed_string_view;

		template<typename T>
		struct basic_bilateral_fixed_string_view<basic_fixed_string_view<T>, basic_fixed_string_view<T>>
				: bilateral_string_base<
					basic_bilateral_fixed_string_view<basic_fixed_string_view<T>, basic_fixed_string_view<T>>,
					typename basic_fixed_string_view<T>::value_type,
					typename basic_fixed_string_view<T>::size_type,
					basic_fixed_string_view<T>,
					basic_fixed_string_view<T>
				>
		{
			using left_type = basic_fixed_string_view<T>;
			using right_type = basic_fixed_string_view<T>;

			using value_type = typename left_type::value_type;
			using size_type = typename left_type::size_type;
			using const_iterator = typename left_type::const_iterator;

			left_type  left_value;
			right_type right_value;

			[[nodiscard]] constexpr const_iterator left_begin() noexcept { return left_value.begin(); }

			[[nodiscard]] constexpr const_iterator left_end() noexcept { return left_value.end(); }

			[[nodiscard]] constexpr const_iterator right_begin() noexcept { return right_value.begin(); }

			[[nodiscard]] constexpr const_iterator right_end() noexcept { return right_value.end(); }
		};

		template<std::size_t N>
		using fixed_string = basic_fixed_string<char, N>;
		template<std::size_t N>
		using fixed_wstring = basic_fixed_string<wchar_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u8string = basic_fixed_string<char8_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u16string = basic_fixed_string<char16_t, N>;
		template<std::size_t N>
		// ReSharper disable once CppInconsistentNaming
		using fixed_u32string = basic_fixed_string<char32_t, N>;

		using fixed_string_view = basic_fixed_string_view<char>;
		using fixed_wstring_view = basic_fixed_string_view<wchar_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u8string_view = basic_fixed_string_view<char8_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u16string_view = basic_fixed_string_view<char16_t>;
		// ReSharper disable once CppInconsistentNaming
		using fixed_u32string_view = basic_fixed_string_view<char32_t>;

		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
		using fixed_bilateral_string = basic_bilateral_fixed_string<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
		using fixed_bilateral_wstring = basic_bilateral_fixed_string<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u8string = basic_bilateral_fixed_string<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u16string = basic_bilateral_fixed_string<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u32string = basic_bilateral_fixed_string<Left, Right>;

		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
		using fixed_bilateral_string_view = basic_bilateral_fixed_string_view<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
		using fixed_bilateral_wstring_view = basic_bilateral_fixed_string_view<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u8string_view = basic_bilateral_fixed_string_view<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u16string_view = basic_bilateral_fixed_string_view<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
		// ReSharper disable once CppInconsistentNaming
		using fixed_bilateral_u32string_view = basic_bilateral_fixed_string_view<Left, Right>;
	}

	// basic_char_array
	export
	{
		template<typename T, T... Cs>
		struct basic_char_array : string_base<basic_char_array<T, Cs...>, T, std::size_t>
		{
			using value_type = T;
			using size_type = std::size_t;

			using iterator = const value_type*;
			using const_iterator = const value_type*;

			constexpr static size_type  max_size = sizeof...(Cs);
			constexpr static value_type value[max_size]{Cs...};
			constexpr static size_type  size = max_size - (value[max_size - 1] == '\0');

			[[nodiscard]] constexpr static const_iterator begin() noexcept { return std::ranges::begin(value); }

			[[nodiscard]] constexpr static const_iterator end() noexcept { return std::ranges::end(value); }
		};

		template<typename, typename>
		struct basic_bilateral_char_array;

		template<typename T, T... Left, T... Right>
		struct basic_bilateral_char_array<basic_char_array<T, Left...>, basic_char_array<T, Right...>>
				: bilateral_string_base<
					basic_bilateral_char_array<basic_char_array<T, Left...>, basic_char_array<T, Right...>>,
					typename basic_char_array<T, Left...>::value_type,
					typename basic_char_array<T, Left...>::size_type,
					basic_char_array<T, Left...>,
					basic_char_array<T, Right...>
				>
		{
			using left_type = basic_char_array<T, Left...>;
			using right_type = basic_char_array<T, Right...>;

			using value_type = typename left_type::value_type;
			using size_type = typename left_type::size_type;

			using iterator = typename left_type::iterator;
			using const_iterator = typename left_type::const_iterator;

			constexpr static size_type left_max_size  = left_type::template max_size;
			constexpr static size_type left_size      = left_type::template size;
			constexpr static size_type right_max_size = right_type::template max_size;
			constexpr static size_type right_size     = right_type::template size;

			[[nodiscard]] constexpr static const_iterator left_begin() noexcept { return left_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator left_end() noexcept { return left_type::template end(); }

			[[nodiscard]] constexpr static const_iterator right_begin() noexcept { return right_type::template begin(); }

			[[nodiscard]] constexpr static const_iterator right_end() noexcept { return right_type::template end(); }
		};

		template<typename...>
		struct basic_multiple_char_array;

		template<typename BasicCharArray, typename... BasicCharArrays>
			requires(std::is_same_v<typename BasicCharArray::value_type, typename BasicCharArrays::value_type> && ...)
		struct basic_multiple_char_array<BasicCharArray, BasicCharArrays...>
				: multiple_string_base<
					basic_multiple_char_array<BasicCharArray, BasicCharArrays...>,
					typename BasicCharArray::value_type,
					typename BasicCharArray::size_type,
					BasicCharArray,
					BasicCharArrays...>
		{
			using base_type = multiple_string_base<basic_multiple_char_array, typename BasicCharArray::value_type, typename BasicCharArray::size_type, BasicCharArray, BasicCharArrays...>;

			using value_type = typename BasicCharArray::value_type;
			using size_type = typename BasicCharArray::size_type;

			using iterator = typename BasicCharArray::iterator;
			using const_iterator = typename BasicCharArray::const_iterator;

			template<std::size_t Index>
			[[nodiscard]] constexpr static const_iterator begin() noexcept { return typename base_type::template subtype<Index>::begin(); }

			template<std::size_t Index>
			[[nodiscard]] constexpr static const_iterator end() noexcept { return typename base_type::template subtype<Index>::end(); }
		};

		template<char... Chars>
		using char_array = basic_char_array<char, Chars...>;
		template<wchar_t... Chars>
		using wchar_array = basic_char_array<wchar_t, Chars...>;
		template<char8_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u8char_array = basic_char_array<char8_t, Chars...>;
		template<char16_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u16char_array = basic_char_array<char16_t, Chars...>;
		template<char32_t... Chars>
		// ReSharper disable once CppInconsistentNaming
		using u32char_array = basic_char_array<char32_t, Chars...>;

		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char>
		using char_bilateral_array = basic_bilateral_char_array<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, wchar_t>
		using wchar_bilateral_array = basic_bilateral_char_array<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char8_t>
		// ReSharper disable once CppInconsistentNaming
		using u8char_bilateral_array = basic_bilateral_char_array<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char16_t>
		// ReSharper disable once CppInconsistentNaming
		using u16char_bilateral_array = basic_bilateral_char_array<Left, Right>;
		template<typename Left, typename Right>
			requires std::is_same_v<typename Left::value_type, typename Right::value_type> && std::is_same_v<typename Left::value_type, char32_t>
		// ReSharper disable once CppInconsistentNaming
		using u32char_bilateral_array = basic_bilateral_char_array<Left, Right>;

		template<typename FirstString, typename... RemainingStrings>
			requires(std::is_same_v<typename FirstString::value_type, typename RemainingStrings::value_type> && ...) && std::is_same_v<typename FirstString::value_type, char>
		using char_multiple_array = basic_multiple_char_array<FirstString, RemainingStrings...>;
		template<typename FirstString, typename... RemainingStrings>
			requires(std::is_same_v<typename FirstString::value_type, typename RemainingStrings::value_type> && ...) && std::is_same_v<typename FirstString::value_type, wchar_t>
		using wchar_multiple_array = basic_multiple_char_array<FirstString, RemainingStrings...>;
		template<typename FirstString, typename... RemainingStrings>
			requires(std::is_same_v<typename FirstString::value_type, typename RemainingStrings::value_type> && ...) && std::is_same_v<typename FirstString::value_type, char8_t>
		// ReSharper disable once CppInconsistentNaming
		using u8char_multiple_array = basic_multiple_char_array<FirstString, RemainingStrings...>;
		template<typename FirstString, typename... RemainingStrings>
			requires(std::is_same_v<typename FirstString::value_type, typename RemainingStrings::value_type> && ...) && std::is_same_v<typename FirstString::value_type, char16_t>
		// ReSharper disable once CppInconsistentNaming
		using u16char_multiple_array = basic_multiple_char_array<FirstString, RemainingStrings...>;
		template<typename FirstString, typename... RemainingStrings>
			requires(std::is_same_v<typename FirstString::value_type, typename RemainingStrings::value_type> && ...) && std::is_same_v<typename FirstString::value_type, char32_t>
		// ReSharper disable once CppInconsistentNaming
		using u32char_multiple_array = basic_multiple_char_array<FirstString, RemainingStrings...>;
	}
}

export namespace std
{
	// for `std::totally_ordered_with`
	template<typename FixedString, typename String, template<typename> typename Q1, template<typename> typename Q2>
		requires
		gal::prometheus::infrastructure::is_fixed_string<FixedString>::value and
		std::is_constructible_v<gal::prometheus::infrastructure::basic_fixed_string_view<typename FixedString::value_type>, String>
	struct basic_common_reference<FixedString, String, Q1, Q2>
	{
		using type = gal::prometheus::infrastructure::basic_fixed_string_view<typename FixedString::value_type>;
	};

	template<typename String, typename FixedString, template<typename> typename Q1, template<typename> typename Q2>
		requires gal::prometheus::infrastructure::is_fixed_string<FixedString>::value and
				std::is_constructible_v<gal::prometheus::infrastructure::basic_fixed_string_view<typename FixedString::value_type>, String>
	struct basic_common_reference<String, FixedString, Q1, Q2>
	{
		using type = gal::prometheus::infrastructure::basic_fixed_string_view<typename FixedString::value_type>;
	};
}
