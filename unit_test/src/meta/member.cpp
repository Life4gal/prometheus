// meta::member
#include <meta/meta.hpp>

#include <string>

using namespace gal::prometheus;

namespace
{
	struct const_left_reference
	{
		[[maybe_unused]] int id;

		[[nodiscard]] constexpr auto operator==(const const_left_reference&) const & noexcept -> bool { return true; }
		[[nodiscard]] constexpr auto operator==(const_left_reference&&) const & noexcept -> bool { return false; }
		// ReSharper disable CppMemberFunctionMayBeConst
		[[nodiscard]] constexpr auto operator==(const const_left_reference&) && noexcept -> bool { return false; }
		[[nodiscard]] constexpr auto operator==(const_left_reference&&) && noexcept -> bool { return false; }
		// ReSharper restore CppMemberFunctionMayBeConst
	};

	struct right_reference
	{
		[[maybe_unused]] int id;

		[[nodiscard]] constexpr auto operator==(const right_reference&) const & noexcept -> bool { return false; }
		[[nodiscard]] constexpr auto operator==(right_reference&&) const & noexcept -> bool { return false; }
		// ReSharper disable CppMemberFunctionMayBeConst
		[[nodiscard]] constexpr auto operator==(const right_reference&) && noexcept -> bool { return false; }
		[[nodiscard]] constexpr auto operator==(right_reference&&) && noexcept -> bool { return true; }
		// ReSharper restore CppMemberFunctionMayBeConst
	};

	class MyTupleLike
	{
	public:
		template<std::size_t Index>
		using type = std::conditional_t<
			Index == 0,
			const_left_reference,
			std::conditional_t<
				Index == 1,
				right_reference,
				std::string
			>
		>;

	private:
		const_left_reference a_;
		right_reference b_;
		std::string c_;

		friend struct meta::extern_accessor<MyTupleLike>;
		constexpr MyTupleLike() noexcept = default;

	public:
		constexpr explicit MyTupleLike(const const_left_reference a, const right_reference b, std::string c) noexcept
			: a_{a},
			  b_{b},
			  c_{std::move(c)} {}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto get() & noexcept -> auto&
		{
			if constexpr (Index == 0)
			{
				return a_;
			}
			else if constexpr (Index == 1)
			{
				return b_;
			}
			else if constexpr (Index == 2)
			{
				return c_;
			}
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto get() const & noexcept -> const auto&
		{
			if constexpr (Index == 0)
			{
				return a_;
			}
			else if constexpr (Index == 1)
			{
				return b_;
			}
			else if constexpr (Index == 2)
			{
				return c_;
			}
		}

		template<std::size_t Index>
		[[nodiscard]] constexpr auto get() && noexcept -> auto&&
		{
			if constexpr (Index == 0)
			{
				return std::move(a_);
			}
			else if constexpr (Index == 1)
			{
				return std::move(b_);
			}
			else if constexpr (Index == 2)
			{
				return std::move(c_);
			}
		}
	};

	struct my_aggregate
	{
		[[maybe_unused]] const_left_reference a;
		[[maybe_unused]] right_reference b;
		[[maybe_unused]] std::string c;
	};

	static_assert(std::is_aggregate_v<MyTupleLike> == false);
	static_assert(std::is_aggregate_v<my_aggregate> == true);
}

namespace std
{
	template<std::size_t Index>
	struct tuple_element<Index, MyTupleLike>
	{
		using type = MyTupleLike::type<Index>;
	};

	template<>
	struct tuple_size<MyTupleLike> : std::integral_constant<std::size_t, 3> {};
}

namespace
{
	static_assert(meta::member_size<MyTupleLike>() == 3);
	static_assert(meta::member_size<my_aggregate>() == 3);

	template<typename T, std::size_t Index>
	[[nodiscard]] constexpr auto test_clr() noexcept -> bool
	{
		constexpr const_left_reference a{.id = __LINE__};
		constexpr right_reference b{.id = __LINE__};
		const std::string c{"hello world"};

		const T object{a, b, c};

		if constexpr (Index == 0)
		{
			return meta::member_of_index<0>(object) == a;
		}
		else if constexpr (Index == 1)
		{
			return meta::member_of_index<1>(object) == b;
		}
		else if constexpr (Index == 2)
		{
			return meta::member_of_index<2>(object) == c;
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	template<typename T, std::size_t Index>
	[[nodiscard]] constexpr auto test_rr() noexcept -> bool
	{
		constexpr const_left_reference a{.id = __LINE__};
		constexpr right_reference b{.id = __LINE__};
		const std::string c{"hello world"};

		T object{a, b, c};

		if constexpr (Index == 0)
		{
			return meta::member_of_index<0>(std::move(object)) == const_left_reference{a};
		}
		else if constexpr (Index == 1)
		{
			return meta::member_of_index<1>(std::move(object)) == right_reference{b};
		}
		else if constexpr (Index == 2)
		{
			return meta::member_of_index<2>(std::move(object)) == std::string{c};
		}
		else
		{
			GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
		}
	}

	static_assert(test_clr<MyTupleLike, 0>() == true);
	static_assert(test_clr<MyTupleLike, 1>() == false);
	static_assert(test_clr<MyTupleLike, 2>() == true);
	static_assert(test_rr<MyTupleLike, 0>() == false);
	static_assert(test_rr<MyTupleLike, 1>() == true);
	static_assert(test_rr<MyTupleLike, 2>() == true);

	static_assert(test_clr<my_aggregate, 0>() == true);
	static_assert(test_clr<my_aggregate, 1>() == false);
	static_assert(test_clr<my_aggregate, 2>() == true);
	static_assert(test_rr<my_aggregate, 0>() == false);
	static_assert(test_rr<my_aggregate, 1>() == true);
	static_assert(test_rr<my_aggregate, 2>() == true);

	static_assert(meta::name_of_member<0, MyTupleLike>() == "a_");
	static_assert(meta::name_of_member<1, MyTupleLike>() == "b_");
	static_assert(meta::name_of_member<2, MyTupleLike>() == "c_");

	static_assert(meta::name_of_member<0, my_aggregate>() == "a");
	static_assert(meta::name_of_member<1, my_aggregate>() == "b");
	static_assert(meta::name_of_member<2, my_aggregate>() == "c");

	static_assert(meta::member_index<"a_", MyTupleLike>() == 0);
	static_assert(meta::member_index<"b_", MyTupleLike>() == 1);
	static_assert(meta::member_index<"c_", MyTupleLike>() == 2);

	static_assert(meta::member_index<"a", my_aggregate>() == 0);
	static_assert(meta::member_index<"b", my_aggregate>() == 1);
	static_assert(meta::member_index<"c", my_aggregate>() == 2);

	static_assert(meta::member_index<MyTupleLike>("a_") == 0);
	static_assert(meta::member_index<MyTupleLike>("b_") == 1);
	static_assert(meta::member_index<MyTupleLike>("c_") == 2);

	static_assert(meta::member_index<my_aggregate>("a") == 0);
	static_assert(meta::member_index<my_aggregate>("b") == 1);
	static_assert(meta::member_index<my_aggregate>("c") == 2);

	template<typename T>
	[[nodiscard]] constexpr auto test_walk() noexcept -> bool
	{
		constexpr const_left_reference a{.id = __LINE__};
		constexpr right_reference b{.id = __LINE__};
		const std::string c{"hello world"};

		const T object1{a, b, c};
		const T object2{a, b, c};

		T object{object1};
		meta::member_walk(
			[]<std::size_t Index>(auto& o, const auto& o1, const auto& o2) noexcept -> void
			{
				if constexpr (Index == 0)
				{
					// const_left_reference
					o.id += (o1.id + o2.id);
				}
				else if constexpr (Index == 1)
				{
					// right_reference
					o.id -= (o1.id + o2.id);
				}
				else if constexpr (Index == 2)
				{
					// std::string
					o.append("-");
					o.append_range(o1);
					o.append("-");
					o.append_range(o2);
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			},
			object,
			object1,
			object2
		);

		const auto& [o_a, o_b, o_c] = object;
		std::string s{c};
		s.append("-");
		s.append_range(c);
		s.append("-");
		s.append_range(c);

		return //
				o_a.id == a.id + (a.id + a.id) and
				o_b.id == b.id - (b.id + b.id) and
				o_c == s;
	}

	static_assert(test_walk<MyTupleLike>() == true);
	static_assert(test_walk<my_aggregate>() == true);

	template<typename T>
	[[nodiscard]] constexpr auto test_walk_until() noexcept -> bool
	{
		constexpr const_left_reference a{.id = __LINE__};
		constexpr right_reference b{.id = __LINE__};
		const std::string c{"hello world"};

		const T object1{a, b, c};
		const T object2{a, b, c};

		T object{object1};
		meta::member_walk_until(
			[]<std::size_t Index>(auto& o, const auto& o1, const auto& o2) noexcept -> bool
			{
				if constexpr (Index == 0)
				{
					// const_left_reference
					o.id += (o1.id + o2.id);
					return true;
				}
				else if constexpr (Index == 1)
				{
					// right_reference
					o.id -= (o1.id + o2.id);
					return false;
				}
				else if constexpr (Index == 2)
				{
					// std::string
					o.append("-");
					o.append_range(o1);
					o.append("-");
					o.append_range(o2);
					return true;
				}
				else
				{
					GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE();
				}
			},
			object,
			object1,
			object2
		);

		const auto& [o_a, o_b, o_c] = object;

		return //
				o_a.id == a.id + (a.id + a.id) and
				o_b.id == b.id - (b.id + b.id) and
				o_c == c;
	}

	static_assert(test_walk_until<MyTupleLike>() == true);
	static_assert(test_walk_until<my_aggregate>() == true);
}
