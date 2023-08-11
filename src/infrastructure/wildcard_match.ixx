// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:wildcard_match;

import std;
import :runtime_error.exception;

export namespace gal::prometheus::infrastructure
{
	// https://en.cppreference.com/w/cpp/language/string_literal
	/**
	* default wildcard:
	*
	* basic part:
	* anything -> *
	* single -> ?
	* escape -> \
	*
	* extend part:
	*
	* [] + ! -> the content in [] must appear (at least one) or not appear at all, depending on whether it has `!`
	* example:
	*		[abc] means that one of `abc` appears at least once
	*		[!def] means that none of `def` appears
	*
	* () + | -> the content in () must appear (at least one of all the alternatives)
	* example:
	*		(a|b|c) means that one of `a`, `b`, `c` appears
	*		(|d|e|f) means that one of ``, `d`, `e`, `f` appears (`` is means empty, which means empty is acceptable)
	*
	*
	* users can specialize the template type they need, as long as the specified wildcards have operator==
	*/
	template<typename T>
	struct wildcard_type;

	template<>
	struct wildcard_type<char>
	{
		using value_type = char;

		// =============================
		// STANDARD
		// =============================
		value_type anything{'*'};
		value_type single{'?'};
		value_type escape{'\\'};

		// =============================
		// EXTENDED
		// =============================
		value_type set_open{'['};
		value_type set_close{']'};
		value_type set_not{'!'};

		value_type alt_open{'('};
		value_type alt_close{')'};
		value_type alt_or{'|'};

		constexpr wildcard_type() noexcept = default;

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape} {}

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape,
				const value_type in_set_open,
				const value_type in_set_close,
				const value_type in_set_not,
				const value_type in_alt_open,
				const value_type in_alt_close,
				const value_type in_alt_or) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape},
			set_open{in_set_open},
			set_close{in_set_close},
			set_not{in_set_not},
			alt_open{in_alt_open},
			alt_close{in_alt_close},
			alt_or{in_alt_or} {}
	};

	template<>
	struct wildcard_type<wchar_t>
	{
		using value_type = wchar_t;

		// =============================
		// STANDARD
		// =============================
		value_type anything{L'*'};
		value_type single{L'?'};
		value_type escape{L'\\'};

		// =============================
		// EXTENDED
		// =============================
		value_type set_open{L'['};
		value_type set_close{L']'};
		value_type set_not{L'!'};

		value_type alt_open{L'('};
		value_type alt_close{L')'};
		value_type alt_or{L'|'};

		constexpr wildcard_type() noexcept = default;

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape} {}

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape,
				const value_type in_set_open,
				const value_type in_set_close,
				const value_type in_set_not,
				const value_type in_alt_open,
				const value_type in_alt_close,
				const value_type in_alt_or) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape},
			set_open{in_set_open},
			set_close{in_set_close},
			set_not{in_set_not},
			alt_open{in_alt_open},
			alt_close{in_alt_close},
			alt_or{in_alt_or} {}
	};

	template<>
	struct wildcard_type<char8_t>
	{
		using value_type = char8_t;

		// =============================
		// STANDARD
		// =============================
		value_type anything{u8'*'};
		value_type single{u8'?'};
		value_type escape{u8'\\'};

		// =============================
		// EXTENDED
		// =============================
		value_type set_open{u8'['};
		value_type set_close{u8']'};
		value_type set_not{u8'!'};

		value_type alt_open{u8'('};
		value_type alt_close{u8')'};
		value_type alt_or{u8'|'};

		constexpr wildcard_type() noexcept = default;

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape} {}

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape,
				const value_type in_set_open,
				const value_type in_set_close,
				const value_type in_set_not,
				const value_type in_alt_open,
				const value_type in_alt_close,
				const value_type in_alt_or) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape},
			set_open{in_set_open},
			set_close{in_set_close},
			set_not{in_set_not},
			alt_open{in_alt_open},
			alt_close{in_alt_close},
			alt_or{in_alt_or} {}
	};

	template<>
	struct wildcard_type<char16_t>
	{
		using value_type = char16_t;

		// =============================
		// STANDARD
		// =============================
		value_type anything{u'*'};
		value_type single{u'?'};
		value_type escape{u'\\'};

		// =============================
		// EXTENDED
		// =============================
		value_type set_open{u'['};
		value_type set_close{u']'};
		value_type set_not{u'!'};

		value_type alt_open{u'('};
		value_type alt_close{u')'};
		value_type alt_or{u'|'};

		constexpr wildcard_type() noexcept = default;

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape} {}

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape,
				const value_type in_set_open,
				const value_type in_set_close,
				const value_type in_set_not,
				const value_type in_alt_open,
				const value_type in_alt_close,
				const value_type in_alt_or) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape},
			set_open{in_set_open},
			set_close{in_set_close},
			set_not{in_set_not},
			alt_open{in_alt_open},
			alt_close{in_alt_close},
			alt_or{in_alt_or} {}
	};

	template<>
	struct wildcard_type<char32_t>
	{
		using value_type = char32_t;

		// =============================
		// STANDARD
		// =============================
		value_type anything{U'*'};
		value_type single{U'?'};
		value_type escape{U'\\'};

		// =============================
		// EXTENDED
		// =============================
		value_type set_open{U'['};
		value_type set_close{U']'};
		value_type set_not{U'!'};

		value_type alt_open{U'('};
		value_type alt_close{U')'};
		value_type alt_or{U'|'};

		constexpr wildcard_type() noexcept = default;

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape} {}

		constexpr wildcard_type(
				const value_type in_anything,
				const value_type in_single,
				const value_type in_escape,
				const value_type in_set_open,
				const value_type in_set_close,
				const value_type in_set_not,
				const value_type in_alt_open,
				const value_type in_alt_close,
				const value_type in_alt_or) noexcept
			: anything{in_anything},
			single{in_single},
			escape{in_escape},
			set_open{in_set_open},
			set_close{in_set_close},
			set_not{in_set_not},
			alt_open{in_alt_open},
			alt_close{in_alt_close},
			alt_or{in_alt_or} {}
	};

	namespace wildcard_match_detail
	{
		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		struct [[nodiscard]] full_match_result
		{
			bool result;

			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS SequenceIterator sequence_begin;
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS SequenceIterator sequence_end;

			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS PatternIterator pattern_begin;
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS PatternIterator pattern_end;

			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS SequenceIterator match_result_sequence;
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS PatternIterator  match_result_pattern;

			[[nodiscard]] constexpr explicit(false) operator bool() const noexcept { return result; }
		};

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		struct [[nodiscard]] match_result
		{
			bool result;

			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS SequenceIterator sequence;
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS PatternIterator  pattern;

			[[nodiscard]] constexpr explicit(false) operator bool() const noexcept { return result; }
		};

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		constexpr auto make_full_match_result(
				SequenceIterator                                sequence_begin,
				SequenceIterator                                sequence_end,
				PatternIterator                                 pattern_begin,
				PatternIterator                                 pattern_end,
				match_result<SequenceIterator, PatternIterator> match_result) noexcept -> full_match_result<SequenceIterator, PatternIterator>
		{
			return {
					.result = match_result.result,
					.sequence_begin = sequence_begin,
					.sequence_end = sequence_end,
					.pattern_begin = pattern_begin,
					.pattern_end = pattern_end,
					.match_result_sequence = match_result.sequence,
					.match_result_pattern = match_result.pattern};
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		constexpr auto make_match_result(
				bool             result,
				SequenceIterator sequence,
				PatternIterator  pattern) noexcept -> match_result<SequenceIterator, PatternIterator>
		{
			return {
					.result = result,
					.sequence = sequence,
					.pattern = pattern};
		}

		// todo: this shouldn't be needed
		// gcc&clang: well, we do not restrict you to use throw in constexpr functions, as long as it is not actually executed
		// MSVC: NO WAY!!!
		template<typename T = std::nullptr_t>
		constexpr auto just_throw_it(const char* message, T t = T{}) -> T
		{
			return message == nullptr
						? t
						: throw InvalidArgumentError(message);
		}

		enum class CheckSetState
		{
			// -> `[`
			OPEN,
			// -> `!` the first option
			NOT_OR_FIRST,
			// the first option
			FIRST,
			// other options
			NEXT,
		};

		enum class MatchSetState
		{
			// -> `[`
			OPEN,
			// -> `!` or the first option
			NOT_OR_FIRST_IN,
			// the first option that should be excluded
			FIRST_OUT,
			// other options that should be excluded
			NEXT_IN,
			// other options that should be included
			NEXT_OUT
		};

		/**
		* @brief determine whether the target pattern has a legal set
		* @note after detecting the first `[`, we always think that the next position is either the first option or `!` (further, the same is true after detecting `!`)
		* so the following `]` will always be ignored (so `[]` and `[!]` Is not a complete set)
		* and ignore all the `[` contained in it (so nested sets are not supported)
		* @tparam EmitError if true, throw a InvalidArgumentError instead return end
		* @tparam PatternIterator iterator type
		* @param begin pattern begin
		* @param end pattern end
		* @param wildcard wildcard
		* @param state users do not need to know the state, and generally start from the beginning (`[`) to parse
		* @return if it is, return the position after the last valid position (`]`) of the set, otherwise it returns begin (perhaps returning end is more intuitive, but in fact it may happen to reach end and the match is successful, so returning end is not necessarily a failure, but it is absolutely impossible to return begin when it succeeds)
		*/
		template<bool EmitError, std::input_iterator PatternIterator>
		constexpr auto check_set_exist(
				PatternIterator                                                                  begin,
				PatternIterator                                                                  end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{},
				CheckSetState                                                                    state    = CheckSetState::OPEN) -> PatternIterator
		{
			auto rollback = begin;
			while (begin not_eq end)
			{
				switch (state)
				{
					case CheckSetState::OPEN: // -> `[`
					{
						// this character not_eq `[`
						if (*begin not_eq wildcard.set_open)
						{
							// emit error or not
							if constexpr (EmitError)
							{
								// throw InvalidArgumentError{"the given pattern is not a valid set"};
								return just_throw_it("the given pattern is not a valid set", rollback);
							}
							else { return rollback; }
						}
						// `[` is detected, then the next character should be `!` or the first option
						state = CheckSetState::NOT_OR_FIRST;
						break;
					}
					case CheckSetState::NOT_OR_FIRST: // -> '!'
					{
						if (*begin == wildcard.set_not)
						{
							// `!` is detected, then the next character should be the first option
							state = CheckSetState::FIRST;
						}
						else
						{
							// the first option is detected, then the next character should be the next option
							state = CheckSetState::NEXT;
						}
						break;
					}
					case CheckSetState::FIRST:
					{
						// the first option is detected, then the next character should be the next option
						state = CheckSetState::NEXT;
						break;
					}
					case CheckSetState::NEXT: // -> `]`
					{
						if (*begin == wildcard.set_close)
						{
							// `]` is detected, then the next character should be the next option
							return std::next(begin);
						}
						break;
					}
				}

				std::advance(begin, 1);
			}

			if constexpr (EmitError)
			{
				// throw InvalidArgumentError{"the given pattern is not a valid set"};
				return just_throw_it("the given pattern is not a valid set", rollback);
			}
			else { return rollback; }
		}

		/**
		* @brief Determine whether the target pattern can match the sequence
		* @tparam SequenceIterator sequence iterator type
		* @tparam PatternIterator pattern iterator type
		* @tparam Comparator used to determine whether two wildcards are equal, and can also be customized
		* @param sequence_begin sequence iterator begin
		* @param sequence_end sequence iterator end
		* @param pattern_begin pattern iterator begin
		* @param pattern_end pattern iterator end
		* @param wildcard wildcard
		* @param comparator equal
		* @param state state users do not need to know the state, and generally start from the beginning (`[`) to parse
		* @return return a match_result, if the match is successful, the match_result has the position where the sequence matches the pattern, otherwise there is no guarantee
		*/
		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator = std::equal_to<>>
		constexpr auto match_set(
				SequenceIterator                                                                 sequence_begin,
				SequenceIterator                                                                 sequence_end,
				PatternIterator                                                                  pattern_begin,
				PatternIterator                                                                  pattern_end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard   = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{},
				const Comparator&                                                                comparator = Comparator{},
				MatchSetState                                                                    state      = MatchSetState::OPEN) -> match_result<SequenceIterator, PatternIterator>
		{
			while (pattern_begin not_eq pattern_end)
			{
				switch (state)
				{
					case MatchSetState::OPEN: // -> '[`
					{
						// this character not_eq `[`
						if (*pattern_begin not_eq wildcard.set_open)
						{
							// throw InvalidArgumentError{"the given pattern is not a valid set"};
							return just_throw_it("the given pattern is not a valid set", make_match_result(false, sequence_begin, pattern_begin));
						}

						// `[` is detected, then the next character should be `!` or the first option
						state = MatchSetState::NOT_OR_FIRST_IN;
						break;
					}
					case MatchSetState::NOT_OR_FIRST_IN: // -> `!`
					{
						if (*pattern_begin == wildcard.set_not)
						{
							// `!` is detected, then the next character should be the first option
							state = MatchSetState::FIRST_OUT;
						}
						else
						{
							// the first option is detected, then the next character should be the next option
							// not a valid sequence
							if (sequence_begin == sequence_end)
							{
								// just return match failed
								return make_match_result(
										false,
										sequence_begin,
										pattern_begin);
							}
							// match succeed
							if (comparator(*sequence_begin, *pattern_begin))
							{
								return make_match_result(
										true,
										sequence_begin,
										pattern_begin);
							}

							// sequence is valid but match failed, continue to parse
							state = MatchSetState::NEXT_IN;
						}
						break;
					}
					case MatchSetState::FIRST_OUT:
					{
						if (
							// not a valid sequence
							sequence_begin == sequence_end or
							// the match should not succeed here
							comparator(*sequence_begin, *pattern_begin))
						{
							return make_match_result(
									false,
									sequence_begin,
									pattern_begin);
						}

						state = MatchSetState::NEXT_OUT;
						break;
					}
					case MatchSetState::NEXT_IN:
					{
						if (
							// not a valid sequence
							sequence_begin == sequence_end or
							// pattern ended early
							*pattern_begin == wildcard.set_close)
						{
							return make_match_result(
									false,
									sequence_begin,
									pattern_begin);
						}
						// match succeed
						if (comparator(*sequence_begin, *pattern_begin))
						{
							return make_match_result(
									true,
									sequence_begin,
									pattern_begin);
						}
						break;
					}
					case MatchSetState::NEXT_OUT:
					{
						// pattern just ended
						if (*pattern_begin == wildcard.set_close)
						{
							return make_match_result(
									true,
									sequence_begin,
									pattern_begin);
						}
						if (
							// not a valid sequence
							sequence_begin == sequence_end or
							// the match should not succeed here
							comparator(*sequence_begin, *pattern_begin))
						{
							return make_match_result(
									false,
									sequence_begin,
									pattern_begin);
						}
						break;
					}
				}

				std::advance(pattern_begin, 1);
			}

			// throw InvalidArgumentError{"the given pattern is not a valid set"};
			return just_throw_it("the given pattern is not a valid set", make_match_result(false, sequence_begin, pattern_begin));
		}

		enum class CheckAltState
		{
			// -> `(`
			OPEN,
			// not the first
			NEXT,
			// the next option after the `\\` does not have a special meaning (represented as the token we specified) and skip it (in short, treat it as a normal option)
			ESCAPE
		};

		enum class MatchAltSubState
		{
			// not the first
			NEXT,
			// the next option after the `\\` does not have a special meaning (represented as the token we specified) and skip it (in short, treat it as a normal option)
			ESCAPE
		};

		/**
		* @brief determine whether the target pattern has a legal alt
		* @note the detection will ignore the nested set and alt, and return the position of the terminator corresponding to the outermost alt
		* @tparam EmitError if true, throw a InvalidArgumentError instead return end
		* @tparam PatternIterator iterator type
		* @param begin pattern begin
		* @param end pattern end
		* @param wildcard wildcard
		* @param state users do not need to know the state, and generally start from the beginning (`(`) to parse
		* @param depth users do not need to know the depth, this is just to confirm whether we have processed all the nested content
		* @param is_sub users do not need to know the is_sub, this just indicates whether it is currently detecting sub-alts (`|`)
		* @return if it is, return the position after the last valid position (`)`) of the alt, otherwise it returns begin (perhaps returning end is more intuitive, but in fact it may happen to reach end and the match is successful, so returning end is not necessarily a failure, but it is absolutely impossible to return begin when it succeeds)
		*/
		template<bool EmitError, std::input_iterator PatternIterator>
		constexpr auto check_alt_exist(
				PatternIterator                                                                  begin,
				PatternIterator                                                                  end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{},
				CheckAltState                                                                    state    = CheckAltState::OPEN,
				std::size_t                                                                      depth    = 0,
				bool                                                                             is_sub   = false) -> PatternIterator
		{
			auto rollback = begin;
			while (begin not_eq end)
			{
				switch (state)
				{
					case CheckAltState::OPEN: // -> `(`
					{
						// this character not_eq `(`
						if (*begin not_eq wildcard.alt_open)
						{
							// emit error or not
							if constexpr (EmitError)
							{
								// throw InvalidArgumentError{"the given pattern is not a valid alternative"};
								return just_throw_it("the given pattern is not a valid alternative", rollback);
							}
							else { return rollback; }
						}

						// `(` is detected, then the next character should the first option
						state = CheckAltState::NEXT;
						++depth;
						break;
					}
					case CheckAltState::NEXT:
					{
						if (*begin == wildcard.escape) { state = CheckAltState::ESCAPE; }
						else if (*begin == wildcard.set_open)
						{
							// if there is a nested set
							if (auto pattern_set_end = check_set_exist<false>(
										std::next(begin),
										end,
										wildcard,
										CheckSetState::NOT_OR_FIRST);
								pattern_set_end not_eq std::next(begin))
							{
								// and it really exist
								// reposition begin to a position where `]` is located after skipping this set
								// why take prev one step back? because we will go one step further below
								begin = std::prev(pattern_set_end);
							}
						}
						else if (*begin == wildcard.alt_open)
						{
							// another nested alt
							++depth;
						}
						else if (*begin == wildcard.alt_close)
						{
							// current alt finished
							--depth;
							// all possible nested alts are matched
							if (depth == 0)
							{
								if (is_sub) { return begin; }
								return std::next(begin);
							}
						}
						else if (is_sub and *begin == wildcard.alt_or)
						{
							// current alt finished
							if (depth == 1) { return begin; }
						}
						break;
					}
					case CheckAltState::ESCAPE:
					{
						state = CheckAltState::NEXT;
						break;
					}
				}

				std::advance(begin, 1);
			}

			if constexpr (EmitError)
			{
				// throw InvalidArgumentError{"the use of sets is disabled"};
				return just_throw_it("the use of sets is disabled", rollback);
			}
			else { return rollback; }
		}

		/**
		* @brief determine whether the target pattern has sub-alternatives
		* @tparam PatternIterator iterator type
		* @param begin pattern begin
		* @param end pattern end
		* @param wildcard wildcard
		* @return guarantee to return the end position of the sub-alternative, and throw an exception if it does not exist
		*/
		template<std::input_iterator PatternIterator>
		constexpr auto check_sub_alt_exist(
				PatternIterator                                                                  begin,
				PatternIterator                                                                  end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{}) -> PatternIterator { return check_alt_exist<true>(begin, end, wildcard, CheckAltState::NEXT, 1, true); }

		/**
		* @brief determine whether the target sequence can match the pattern
		* @tparam SequenceIterator sequence iterator type
		* @tparam PatternIterator pattern iterator type
		* @tparam Comparator equal to type
		* @param sequence_begin sequence begin
		* @param sequence_end sequence end
		* @param pattern_begin pattern begin
		* @param pattern_end pattern end
		* @param wildcard wildcard
		* @param comparator equal to
		* @param partial it is a partial match
		* @param escape escape means that the next content we will directly compare without considering it as a possible token we defined
		* @return does the target sequence can match the pattern?
		*/
		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator = std::equal_to<>>
		constexpr auto match(
				SequenceIterator                                                                 sequence_begin,
				SequenceIterator                                                                 sequence_end,
				PatternIterator                                                                  pattern_begin,
				PatternIterator                                                                  pattern_end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard   = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{},
				const Comparator&                                                                comparator = Comparator{},
				bool                                                                             partial    = false,
				bool                                                                             escape     = false) -> match_result<SequenceIterator, PatternIterator>;

		/**
		* @brief determine whether the target sequence can match the pattern of the two parts
		* @tparam SequenceIterator sequence iterator type
		* @tparam PatternIterator pattern iterator type
		* @tparam Comparator equal to type
		* @param sequence_begin sequence begin
		* @param sequence_end sequence end
		* @param pattern1_begin pattern1 begin
		* @param pattern1_end pattern1 end
		* @param pattern2_begin pattern2 begin
		* @param pattern2_end pattern2 end
		* @param wildcard wildcard
		* @param comparator equal to
		* @param partial it is a partial match
		* @return does the target sequence can match the pattern of the two parts?
		*/
		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator = std::equal_to<>>
		constexpr auto match_alt(
				SequenceIterator                                                                 sequence_begin,
				SequenceIterator                                                                 sequence_end,
				PatternIterator                                                                  pattern1_begin,
				PatternIterator                                                                  pattern1_end,
				PatternIterator                                                                  pattern2_begin,
				PatternIterator                                                                  pattern2_end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard   = wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>{},
				const Comparator&                                                                comparator = Comparator{},
				bool                                                                             partial    = false) -> match_result<SequenceIterator, PatternIterator>
		{
			// is the target sequence partial matches pattern1
			if (auto result1 = match(
						sequence_begin,
						sequence_end,
						pattern1_begin,
						pattern1_end,
						wildcard,
						comparator,
						true);
				result1)
			{
				// is the target sequence matches pattern2
				if (auto result2 = match(
							result1.sequence,
							sequence_end,
							pattern2_begin,
							pattern2_end,
							wildcard,
							comparator,
							partial);
					result2) { return result2; }
			}

			pattern1_begin = std::next(pattern1_end);

			// pattern1 and pattern2 are two connected parts
			if (pattern1_begin == pattern2_begin)
			{
				return make_match_result(
						false,
						sequence_begin,
						pattern1_end);
			}

			// the current position is relatively successful, continue to compare the next position
			return match_alt(
					sequence_begin,
					sequence_end,
					pattern1_begin,
					check_sub_alt_exist(pattern1_begin, pattern2_begin, wildcard),
					pattern2_begin,
					pattern2_end,
					wildcard,
					comparator,
					partial);
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator>
		constexpr auto match(
				SequenceIterator                                                                 sequence_begin,
				SequenceIterator                                                                 sequence_end,
				PatternIterator                                                                  pattern_begin,
				PatternIterator                                                                  pattern_end,
				const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
				const Comparator&                                                                comparator,
				bool                                                                             partial,
				const bool                                                                       escape) -> match_result<SequenceIterator, PatternIterator>
		{
			// not a valid pattern
			if (pattern_begin == pattern_end)
			{
				return make_match_result(
						// if it is a partial match or is not a valid sequence, the match is considered successful
						partial or sequence_begin == sequence_end,
						sequence_begin,
						pattern_begin);
			}

			if (escape)
			{
				if (
					// not a valid sequence
					sequence_begin == sequence_end or
					// the match not succeed here
					not comparator(*sequence_begin, *pattern_begin))
				{
					return make_match_result(
							false,
							sequence_begin,
							pattern_begin);
				}

				// the current position is relatively successful, continue to compare the next position
				return match(
						std::next(sequence_begin),
						sequence_end,
						std::next(pattern_begin),
						pattern_end,
						wildcard,
						comparator,
						partial);
			}

			if (*pattern_begin == wildcard.anything)
			{
				// if the current position of the pattern is `*`, try to match the next position of the pattern
				// if the match is still successful, skip the current `*`
				if (auto result = match(
							sequence_begin,
							sequence_end,
							std::next(pattern_begin),
							pattern_end,
							wildcard,
							comparator,
							partial);
					result) { return result; }

				// not a valid sequence
				if (sequence_begin == sequence_end)
				{
					return make_match_result(
							false,
							sequence_begin,
							pattern_begin);
				}

				// if the match is not successful, skip the current position of sequence
				return match(
						std::next(sequence_begin),
						sequence_end,
						pattern_begin,
						pattern_end,
						wildcard,
						comparator,
						partial);
			}

			if (*pattern_begin == wildcard.single)
			{
				// if the current position of the pattern is `?`
				// not a valid sequence
				if (sequence_begin == sequence_end)
				{
					return make_match_result(
							false,
							sequence_begin,
							pattern_begin);
				}

				// try to match the next position of the pattern and sequence
				return match(
						std::next(sequence_begin),
						sequence_end,
						std::next(pattern_begin),
						pattern_end,
						wildcard,
						comparator,
						partial);
			}

			if (*pattern_begin == wildcard.escape)
			{
				// match the next position of the pattern
				return match(
						sequence_begin,
						sequence_end,
						std::next(pattern_begin),
						pattern_end,
						wildcard,
						comparator,
						partial,
						true);
			}

			if (*pattern_begin == wildcard.set_open)
			{
				// if there is a nested set
				if (auto pattern_set_end = check_set_exist<false>(
							std::next(pattern_begin),
							pattern_end,
							wildcard,
							CheckSetState::NOT_OR_FIRST);
					pattern_set_end not_eq std::next(pattern_begin))
				{
					// and it really exist

					// if the nested set does not match successfully, the result will be returned directly (the match failed)
					if (auto result = match_set(
								sequence_begin,
								sequence_end,
								std::next(pattern_begin),
								pattern_end,
								wildcard,
								comparator,
								MatchSetState::NOT_OR_FIRST_IN);
						not result) { return result; }

					// after the match is successful, skip this nested set to continue matching
					return match(
							std::next(sequence_begin),
							sequence_end,
							pattern_set_end,
							pattern_end,
							wildcard,
							comparator,
							partial);
				}
			}

			if (*pattern_begin == wildcard.alt_open)
			{
				// if there is a nested alt
				if (auto pattern_alt_end = check_alt_exist<false>(
							std::next(pattern_begin),
							pattern_end,
							wildcard,
							CheckAltState::NEXT,
							1);
					pattern_alt_end not_eq std::next(pattern_begin))
				{
					// and it really exist

					// match one of the alternatives
					return match_alt(
							sequence_begin,
							sequence_end,
							std::next(pattern_begin),
							check_sub_alt_exist(std::next(pattern_begin), pattern_alt_end, wildcard),
							pattern_alt_end,
							pattern_end,
							wildcard,
							comparator,
							partial);
				}
			}

			if (
				// not a valid sequence
				sequence_begin == sequence_end or
				// the match not succeed here
				not comparator(*sequence_begin, *pattern_begin))
			{
				return make_match_result(
						false,
						sequence_begin,
						pattern_begin);
			}

			// the current position is relatively successful, continue to compare the next position
			return match(
					std::next(sequence_begin),
					sequence_end,
					std::next(pattern_begin),
					pattern_end,
					wildcard,
					comparator,
					partial);
		}
	}// namespace wildcard_match_detail

	template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator = std::equal_to<>>
	constexpr auto match(
			SequenceIterator                                                                      sequence_begin,
			SequenceIterator                                                                      sequence_end,
			PatternIterator                                                                       pattern_begin,
			PatternIterator                                                                       pattern_end,
			const wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>& wildcard   = wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>{},
			const Comparator&                                                                     comparator = Comparator{})
		-> wildcard_match_detail::full_match_result<SequenceIterator, PatternIterator>
	{
		return wildcard_match_detail::make_full_match_result(
				sequence_begin,
				sequence_end,
				pattern_begin,
				pattern_end,
				wildcard_match_detail::match(
						sequence_begin,
						sequence_end,
						pattern_begin,
						pattern_end,
						wildcard,
						comparator));
	}

	template<std::ranges::range Sequence, std::input_iterator PatternIterator, typename Comparator = std::equal_to<>>
	constexpr auto
	match(
			Sequence&&                                                                            sequence,
			PatternIterator                                                                       pattern_begin,
			PatternIterator                                                                       pattern_end,
			const wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>& wildcard   = wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>{},
			const Comparator&                                                                     comparator = Comparator{})
		-> wildcard_match_detail::full_match_result<decltype(std::ranges::cbegin(std::declval<const Sequence&>())), PatternIterator>
	{
		return match(
				std::cbegin(std::forward<Sequence>(sequence)),
				std::cend(std::forward<Sequence>(sequence)),
				pattern_begin,
				pattern_end,
				wildcard,
				comparator);
	}

	template<std::input_iterator SequenceIterator, std::ranges::range Pattern, typename Comparator = std::equal_to<>>
	constexpr auto
	match(
			SequenceIterator                                          sequence_begin,
			SequenceIterator                                          sequence_end,
			Pattern&&                                                 pattern,
			const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard   = wildcard_type<std::ranges::range_value_t<Pattern>>{},
			const Comparator&                                         comparator = Comparator{})
		-> wildcard_match_detail::full_match_result<SequenceIterator, decltype(std::ranges::cbegin(std::declval<const Pattern&>()))>
	{
		return match(
				sequence_begin,
				sequence_end,
				std::cbegin(std::forward<Pattern>(pattern)),
				std::cend(std::forward<Pattern>(pattern)),
				wildcard,
				comparator);
	}

	template<std::ranges::range Sequence, std::ranges::range Pattern, typename Comparator = std::equal_to<>>
	constexpr auto
	match(
			Sequence&&                                                sequence,
			Pattern&&                                                 pattern,
			const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard   = wildcard_type<std::ranges::range_value_t<Pattern>>{},
			const Comparator&                                         comparator = Comparator{})
		-> wildcard_match_detail::full_match_result<decltype(std::ranges::cbegin(std::declval<const Sequence&>())), decltype(std::ranges::cbegin(std::declval<const Pattern&>()))>
	{
		return match(
				std::cbegin(std::forward<Sequence>(sequence)),
				std::cend(std::forward<Sequence>(sequence)),
				std::cbegin(std::forward<Pattern>(pattern)),
				std::cend(std::forward<Pattern>(pattern)),
				wildcard,
				comparator);
	}

	template<std::ranges::range Pattern, typename Comparator = std::equal_to<>>
		requires std::is_invocable_v<Comparator, const std::ranges::range_value_t<Pattern>&, const std::ranges::range_value_t<Pattern>&>
	class [[nodiscard]] WildcardMatcher final
	{
	public:
		using pattern_type = Pattern;

		using value_type = std::ranges::range_value_t<pattern_type>;
		using const_iterator = decltype(std::ranges::cbegin(std::declval<const pattern_type&>()));

		using wildcard = wildcard_type<value_type>;
		using comparator = std::decay_t<Comparator>;

	private:
		const_iterator borrow_pattern_begin_;
		const_iterator borrow_pattern_end_;

		wildcard                                    current_wildcard_;
		GAL_PROMETHEUS_NO_UNIQUE_ADDRESS comparator current_comparator_;

	public:
		constexpr explicit WildcardMatcher(
				const pattern_type& pattern,
				wildcard&&          w = wildcard{},
				comparator          c = comparator{})
			: borrow_pattern_begin_{std::ranges::cbegin(pattern)},
			borrow_pattern_end_{std::ranges::cend(pattern)},
			current_wildcard_{std::forward<wildcard>(w)},
			current_comparator_{c} {}

		constexpr auto replace(wildcard&& new_wildcard) noexcept -> Comparator&
		{
			using std::swap;
			swap(current_wildcard_, new_wildcard);
			return *this;
		}

		constexpr auto replace(comparator new_comparator) noexcept -> Comparator&
		{
			using std::swap;
			swap(current_comparator_, new_comparator);
			return *this;
		}

		template<std::input_iterator SequenceIterator>
		constexpr auto
		operator()(
				SequenceIterator sequence_begin,
				SequenceIterator sequence_end) const -> wildcard_match_detail::full_match_result<SequenceIterator, const_iterator>
		{
			return match(
					sequence_begin,
					sequence_end,
					borrow_pattern_begin_,
					borrow_pattern_end_,
					current_wildcard_,
					current_comparator_);
		}

		template<std::ranges::range Sequence>
		constexpr auto
		operator()(const Sequence& sequence) const -> wildcard_match_detail::full_match_result<decltype(std::ranges::cbegin(std::declval<const Sequence&>())), const_iterator>
		{
			return match(
					sequence,
					borrow_pattern_begin_,
					borrow_pattern_end_,
					current_wildcard_,
					current_comparator_);
		}
	};

	template<std::ranges::range Pattern, typename Comparator>
	constexpr auto make_wildcard_matcher(
			const Pattern&                                       pattern,
			wildcard_type<std::ranges::range_value_t<Pattern>>&& wildcard   = wildcard_type<std::ranges::range_value_t<Pattern>>{},
			Comparator                                           comparator = Comparator{}) noexcept(noexcept(WildcardMatcher<Pattern, Comparator>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard), comparator}))
		-> WildcardMatcher<Pattern, Comparator> { return WildcardMatcher<Pattern, Comparator>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard), comparator}; }

	template<std::ranges::range Pattern, typename Comparator>
	[[nodiscard]] constexpr auto make_wildcard_matcher(
			const Pattern&                                       pattern,
			Comparator                                           comparator = Comparator{},
			wildcard_type<std::ranges::range_value_t<Pattern>>&& wildcard   = wildcard_type<std::ranges::range_value_t<Pattern>>{}) noexcept(noexcept(WildcardMatcher<Pattern, Comparator>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard), comparator}))
		-> WildcardMatcher<Pattern, Comparator> { return WildcardMatcher<Pattern, Comparator>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard), comparator}; }

	template<std::ranges::range Pattern>
	[[nodiscard]] constexpr auto make_wildcard_matcher(
			const Pattern&                                       pattern,
			wildcard_type<std::ranges::range_value_t<Pattern>>&& wildcard = wildcard_type<std::ranges::range_value_t<Pattern>>{}) noexcept(noexcept(WildcardMatcher<Pattern>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard)}))
		-> WildcardMatcher<Pattern> { return WildcardMatcher<Pattern>{pattern, std::forward<wildcard_type<std::ranges::range_value_t<Pattern>>>(wildcard)}; }

	namespace literals
	{
		constexpr auto operator""_wm(
				const char*       str,
				const std::size_t size) noexcept(noexcept(make_wildcard_matcher(std::basic_string_view{str, size + 1})))
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1})) { return make_wildcard_matcher(std::basic_string_view{str, size + 1}); }

		constexpr auto operator""_wm(
				const wchar_t*    str,
				const std::size_t size) noexcept(noexcept(make_wildcard_matcher(std::basic_string_view{str, size + 1})))
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1})) { return make_wildcard_matcher(std::basic_string_view{str, size + 1}); }

		constexpr auto operator""_wm(
				const char8_t*    str,
				const std::size_t size) noexcept(noexcept(make_wildcard_matcher(std::basic_string_view{str, size + 1})))
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1})) { return make_wildcard_matcher(std::basic_string_view{str, size + 1}); }

		constexpr auto operator""_wm(
				const char16_t*   str,
				const std::size_t size) noexcept(noexcept(make_wildcard_matcher(std::basic_string_view{str, size + 1})))
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1})) { return make_wildcard_matcher(std::basic_string_view{str, size + 1}); }

		constexpr auto operator""_wm(
				const char32_t*   str,
				const std::size_t size) noexcept(noexcept(make_wildcard_matcher(std::basic_string_view{str, size + 1})))
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1})) { return make_wildcard_matcher(std::basic_string_view{str, size + 1}); }
	}// namespace literals
}
