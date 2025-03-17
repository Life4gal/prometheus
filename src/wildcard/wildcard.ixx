// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#include <prometheus/macro.hpp>

export module gal.prometheus:wildcard;

import std;

#endif not GAL_PROMETHEUS_MODULE_FRAGMENT_DEFINED

#if not GAL_PROMETHEUS_USE_MODULE

#pragma once

#include <string>
#include <iterator>
#include <ranges>
#include <algorithm>

#include <prometheus/macro.hpp>

#endif

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: wildcard
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(wildcard)
#endif
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
	};
}

GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_INTERNAL(wildcard)
{
		enum class ResultDetail : std::uint8_t
		{
			SUCCESS,

			MISMATCH,
			ERROR
		};

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		struct [[nodiscard]] full_match_result
		{
			ResultDetail result;

			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS SequenceIterator sequence_begin;
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS SequenceIterator sequence_end;

			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS PatternIterator pattern_begin;
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS PatternIterator pattern_end;

			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS SequenceIterator match_result_sequence;
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS PatternIterator match_result_pattern;

			[[nodiscard]] constexpr explicit(false) operator bool() const noexcept { return result == ResultDetail::SUCCESS; }
		};

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		struct [[nodiscard]] match_result
		{
			ResultDetail result;

			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS SequenceIterator sequence;
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS PatternIterator pattern;

			[[nodiscard]] constexpr explicit(false) operator bool() const noexcept { return result == ResultDetail::SUCCESS; }
		};

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		constexpr auto make_full_match_result(
			SequenceIterator sequence_begin,
			SequenceIterator sequence_end,
			PatternIterator pattern_begin,
			PatternIterator pattern_end,
			match_result<SequenceIterator, PatternIterator> match_result //
		) noexcept -> full_match_result<SequenceIterator, PatternIterator>
		{
			return
			{
					.result = match_result.result,
					.sequence_begin = sequence_begin,
					.sequence_end = sequence_end,
					.pattern_begin = pattern_begin,
					.pattern_end = pattern_end,
					.match_result_sequence = match_result.sequence,
					.match_result_pattern = match_result.pattern
			};
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator>
		constexpr auto make_match_result(
			const ResultDetail result,
			SequenceIterator sequence,
			PatternIterator pattern //
		) noexcept -> match_result<SequenceIterator, PatternIterator>
		{
			return
			{
					.result = result,
					.sequence = sequence,
					.pattern = pattern
			};
		}

		template<std::input_iterator Iterator>
		[[nodiscard]] constexpr auto make_invalid_sentinel_for(const Iterator begin, const Iterator end) noexcept -> Iterator
		{
			GAL_PROMETHEUS_SEMANTIC_IF_CONSTANT_EVALUATED
			{
				return std::ranges::prev(begin);
			}

			// In the case of compile-time computation, the compiler won't allow us to get an out-of-bounds iterator, even if we can guarantee that we'll never dereference the iterator.
			return std::ranges::next(end);
		}

		template<std::input_iterator Iterator>
		[[nodiscard]] constexpr auto is_invalid_sentinel_for(const Iterator begin, const Iterator end, const Iterator current) noexcept -> bool
		{
			return make_invalid_sentinel_for(begin, end) == current;
		}

		enum class CheckSetState : std::uint8_t
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

		enum class MatchSetState : std::uint8_t
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

		template<std::input_iterator PatternIterator>
		[[nodiscard]] constexpr auto check_set_exist(
			PatternIterator begin,
			PatternIterator end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			CheckSetState state = CheckSetState::OPEN //
		) noexcept -> bool
		{
			while (begin != end)
			{
				switch (state)
				{
					case CheckSetState::OPEN: // -> `[`
					{
						if (*begin != wildcard.set_open)
						{
							return false;
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
							return true;
						}
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(begin, 1);
			}

			return false;
		}

		template<std::input_iterator PatternIterator, std::sentinel_for<PatternIterator> SentinelIterator>
		[[nodiscard]] constexpr auto find_set_end(
			const PatternIterator begin,
			const SentinelIterator end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			CheckSetState state = CheckSetState::OPEN //
		) noexcept -> PatternIterator
		{
			for (auto current = begin; current != end;)
			{
				switch (state)
				{
					case CheckSetState::OPEN: // -> `[`
					{
						if (*current != wildcard.set_open)
						{
							return make_invalid_sentinel_for(begin, end);
						}

						// `[` is detected, then the next character should be `!` or the first option
						state = CheckSetState::NOT_OR_FIRST;
						break;
					}
					case CheckSetState::NOT_OR_FIRST: // -> '!'
					{
						if (*current == wildcard.set_not)
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
						if (*current == wildcard.set_close)
						{
							return std::ranges::next(current);
						}
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(current, 1);
			}

			return make_invalid_sentinel_for(begin, end);
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator>
		constexpr auto match_set(
			const SequenceIterator sequence_begin,
			const SequenceIterator sequence_end,
			const PatternIterator pattern_begin,
			const PatternIterator pattern_end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			const Comparator& comparator,
			MatchSetState state = MatchSetState::OPEN //
		) noexcept -> match_result<SequenceIterator, PatternIterator>
		{
			auto pattern_current = pattern_begin;
			while (pattern_current != pattern_end)
			{
				switch (state)
				{
					case MatchSetState::OPEN: // -> '[`
					{
						if (*pattern_current != wildcard.set_open)
						{
							return make_match_result(
								ResultDetail::ERROR,
								sequence_begin,
								pattern_current
							);
						}

						// `[` is detected, then the next character should be `!` or the first option
						state = MatchSetState::NOT_OR_FIRST_IN;
						break;
					}
					case MatchSetState::NOT_OR_FIRST_IN: // -> `!`
					{
						if (*pattern_current == wildcard.set_not)
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
									ResultDetail::MISMATCH,
									sequence_begin,
									pattern_current
								);
							}
							// match succeed
							if (comparator(*sequence_begin, *pattern_current))
							{
								return make_match_result(
									ResultDetail::SUCCESS,
									sequence_begin,
									pattern_current
								);
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
							comparator(*sequence_begin, *pattern_current)
						)
						{
							return make_match_result(
								ResultDetail::MISMATCH,
								sequence_begin,
								pattern_current
							);
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
							*pattern_current == wildcard.set_close
						)
						{
							return make_match_result(
								ResultDetail::MISMATCH,
								sequence_begin,
								pattern_current
							);
						}
						// match succeed
						if (comparator(*sequence_begin, *pattern_current))
						{
							return make_match_result(
								ResultDetail::SUCCESS,
								sequence_begin,
								pattern_current
							);
						}
						break;
					}
					case MatchSetState::NEXT_OUT:
					{
						// pattern just ended
						if (*pattern_current == wildcard.set_close)
						{
							return make_match_result(
								ResultDetail::SUCCESS,
								sequence_begin,
								pattern_current
							);
						}
						if (
							// not a valid sequence
							sequence_begin == sequence_end or
							// the match should not succeed here
							comparator(*sequence_begin, *pattern_current)
						)
						{
							return make_match_result(
								ResultDetail::MISMATCH,
								sequence_begin,
								pattern_current
							);
						}
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(pattern_current, 1);
			}

			return make_match_result(
				ResultDetail::ERROR,
				sequence_begin,
				pattern_current
			);
		}

		enum class CheckAltState : std::uint8_t
		{
			// -> `(`
			OPEN,
			// not the first
			NEXT,
			// the next option after the `\\` does not have a special meaning (represented as the token we specified) and skip it (in short, treat it as a normal option)
			ESCAPE
		};

		enum class MatchAltSubState : std::uint8_t
		{
			// not the first
			NEXT,
			// the next option after the `\\` does not have a special meaning (represented as the token we specified) and skip it (in short, treat it as a normal option)
			ESCAPE
		};

		template<std::input_iterator PatternIterator>
		constexpr auto check_alt_exist(
			const PatternIterator begin,
			const PatternIterator end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			CheckAltState state = CheckAltState::OPEN,
			std::size_t depth = 0
		) noexcept -> bool
		{
			for (auto current = begin; current != end;)
			{
				switch (state)
				{
					case CheckAltState::OPEN: // -> `(`
					{
						// this character not equal to `(`
						if (*current != wildcard.alt_open)
						{
							return false;
						}

						// `(` is detected, then the next character should the first option
						state = CheckAltState::NEXT;
						depth += 1;
						break;
					}
					case CheckAltState::NEXT:
					{
						if (*current == wildcard.escape)
						{
							state = CheckAltState::ESCAPE;
						}
						else if (
							*current == wildcard.set_open and
							check_set_exist(
								std::ranges::next(current),
								end,
								wildcard,
								CheckSetState::NOT_OR_FIRST
							)
						)
						{
							const auto result = find_set_end(std::ranges::next(current), end, wildcard, CheckSetState::NOT_OR_FIRST);
							if (is_invalid_sentinel_for(std::ranges::next(current), end, result))
							{
								return false;
							}

							current = std::ranges::prev(result);
						}
						else if (*current == wildcard.alt_open)
						{
							// another nested alt
							depth += 1;
						}
						else if (*current == wildcard.alt_close)
						{
							// current alt finished
							depth -= 1;
							// all possible nested alts are matched
							if (depth == 0)
							{
								return true;
							}
						}

						break;
					}
					case CheckAltState::ESCAPE:
					{
						state = CheckAltState::NEXT;
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(current, 1);
			}

			return false;
		}

		template<std::input_iterator PatternIterator>
		constexpr auto find_alt_end(
			const PatternIterator begin,
			const PatternIterator end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			CheckAltState state = CheckAltState::OPEN,
			std::size_t depth = 0
		) noexcept -> PatternIterator
		{
			for (auto current = begin; current != end;)
			{
				switch (state)
				{
					case CheckAltState::OPEN: // -> `(`
					{
						// this character not equal to `(`
						if (*current != wildcard.alt_open)
						{
							return make_invalid_sentinel_for(begin, end);
						}

						// `(` is detected, then the next character should the first option
						state = CheckAltState::NEXT;
						depth += 1;
						break;
					}
					case CheckAltState::NEXT:
					{
						if (*current == wildcard.escape)
						{
							state = CheckAltState::ESCAPE;
						}
						else if (
							*current == wildcard.set_open and
							check_set_exist(
								std::ranges::next(current),
								end,
								wildcard,
								CheckSetState::NOT_OR_FIRST
							)
						)
						{
							const auto result = find_set_end(std::ranges::next(current), end, wildcard, CheckSetState::NOT_OR_FIRST);
							if (is_invalid_sentinel_for(std::ranges::next(current), end, result))
							{
								return make_invalid_sentinel_for(begin, end);
							}

							current = std::ranges::prev(result);
						}
						else if (*current == wildcard.alt_open)
						{
							// another nested alt
							depth += 1;
						}
						else if (*current == wildcard.alt_close)
						{
							// current alt finished
							depth -= 1;
							// all possible nested alts are matched
							if (depth == 0)
							{
								return std::ranges::next(current);
							}
						}

						break;
					}
					case CheckAltState::ESCAPE:
					{
						state = CheckAltState::NEXT;
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(current, 1);
			}

			return make_invalid_sentinel_for(begin, end);
		}

		template<std::input_iterator PatternIterator>
		constexpr auto find_sub_alt_end(
			const PatternIterator begin,
			const PatternIterator end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			CheckAltState state = CheckAltState::NEXT,
			std::size_t depth = 1
		) noexcept -> PatternIterator
		{
			for (auto current = begin; current != end;)
			{
				switch (state)
				{
					case CheckAltState::OPEN: // -> `(`
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
					case CheckAltState::NEXT:
					{
						if (*current == wildcard.escape)
						{
							state = CheckAltState::ESCAPE;
						}
						else if (
							*current == wildcard.set_open and
							check_set_exist(
								std::ranges::next(current),
								end,
								wildcard,
								CheckSetState::NOT_OR_FIRST
							)
						)
						{
							const auto result = find_set_end(std::ranges::next(current), end, wildcard, CheckSetState::NOT_OR_FIRST);
							if (is_invalid_sentinel_for(std::ranges::next(current), end, result))
							{
								return make_invalid_sentinel_for(begin, end);
							}

							current = std::ranges::prev(result);
						}
						else if (*current == wildcard.alt_open)
						{
							// another nested alt
							depth += 1;
						}
						else if (*current == wildcard.alt_close)
						{
							// current alt finished
							depth -= 1;
							// all possible nested alts are matched
							if (depth == 0)
							{
								return current;
							}
						}
						else if (*current == wildcard.alt_or)
						{
							if (depth == 1)
							{
								return current;
							}
						}

						break;
					}
					case CheckAltState::ESCAPE:
					{
						state = CheckAltState::NEXT;
						break;
					}
					default: // NOLINT(clang-diagnostic-covered-switch-default)
					{
						GAL_PROMETHEUS_ERROR_UNREACHABLE();
					}
				}

				std::ranges::advance(current, 1);
			}

			return make_invalid_sentinel_for(begin, end);
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator>
		constexpr auto match(
			SequenceIterator sequence_begin,
			SequenceIterator sequence_end,
			PatternIterator pattern_begin,
			PatternIterator pattern_end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			const Comparator& comparator,
			bool partial = false,
			bool escape = false //
		) noexcept -> match_result<SequenceIterator, PatternIterator>;

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator>
		constexpr auto match_alt(
			const SequenceIterator sequence_begin,
			const SequenceIterator sequence_end,
			const PatternIterator pattern1_begin,
			const PatternIterator pattern1_end,
			const PatternIterator pattern2_begin,
			const PatternIterator pattern2_end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			const Comparator& comparator,
			bool partial = false //
		) noexcept -> match_result<SequenceIterator, PatternIterator>
		{
			// is the target sequence partial matches pattern1
			if (auto result1 = match(
					sequence_begin,
					sequence_end,
					pattern1_begin,
					pattern1_end,
					wildcard,
					comparator,
					true
				);
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
						partial
					);
					result2) { return result2; }
			}

			const auto pattern1_current = std::ranges::next(pattern1_end);

			// pattern1 and pattern2 are two connected parts
			if (pattern1_current == pattern2_begin)
			{
				return make_match_result(
					ResultDetail::MISMATCH,
					sequence_begin,
					pattern1_end
				);
			}

			const auto sub_alt_end = find_sub_alt_end(pattern1_current, pattern2_begin, wildcard);
			if (is_invalid_sentinel_for(pattern1_current, pattern2_begin, sub_alt_end))
			{
				return make_match_result(
					ResultDetail::ERROR,
					sequence_begin,
					pattern1_current
				);
			}

			// the current position is relatively successful, continue to compare the next position
			return match_alt(
				sequence_begin,
				sequence_end,
				pattern1_current,
				sub_alt_end,
				pattern2_begin,
				pattern2_end,
				wildcard,
				comparator,
				partial
			);
		}

		template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator>
		constexpr auto match(
			const SequenceIterator sequence_begin,
			const SequenceIterator sequence_end,
			const PatternIterator pattern_begin,
			const PatternIterator pattern_end,
			const wildcard_type<typename std::iterator_traits<PatternIterator>::value_type>& wildcard,
			const Comparator& comparator,
			bool partial,
			const bool escape //
		) noexcept -> match_result<SequenceIterator, PatternIterator>
		{
			// not a valid pattern
			if (pattern_begin == pattern_end)
			{
				const auto result = partial or sequence_begin == sequence_end;

				return make_match_result(
					// if it is a partial match or is not a valid sequence, the match is considered successful
					result ? ResultDetail::SUCCESS : ResultDetail::MISMATCH,
					sequence_begin,
					pattern_begin
				);
			}

			if (escape)
			{
				if (
					// not a valid sequence
					sequence_begin == sequence_end or
					// the match not succeed here
					not comparator(*sequence_begin, *pattern_begin)
				)
				{
					return make_match_result(
						ResultDetail::MISMATCH,
						sequence_begin,
						pattern_begin
					);
				}

				// the current position is relatively successful, continue to compare the next position
				return match(
					std::ranges::next(sequence_begin),
					sequence_end,
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					comparator,
					partial
				);
			}

			if (*pattern_begin == wildcard.anything)
			{
				// if the current position of the pattern is `*`, try to match the next position of the pattern
				// if the match is still successful, skip the current `*`
				if (auto result = match(
						sequence_begin,
						sequence_end,
						std::ranges::next(pattern_begin),
						pattern_end,
						wildcard,
						comparator,
						partial
					);
					result) { return result; }

				// not a valid sequence
				if (sequence_begin == sequence_end)
				{
					return make_match_result(
						ResultDetail::MISMATCH,
						sequence_begin,
						pattern_begin
					);
				}

				// if the match is not successful, skip the current position of sequence
				return match(
					std::ranges::next(sequence_begin),
					sequence_end,
					pattern_begin,
					pattern_end,
					wildcard,
					comparator,
					partial
				);
			}

			if (*pattern_begin == wildcard.single)
			{
				// if the current position of the pattern is `?`

				// not a valid sequence
				if (sequence_begin == sequence_end)
				{
					return make_match_result(
						ResultDetail::MISMATCH,
						sequence_begin,
						pattern_begin
					);
				}

				// try to match the next position of the pattern and sequence
				return match(
					std::ranges::next(sequence_begin),
					sequence_end,
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					comparator,
					partial
				);
			}

			if (*pattern_begin == wildcard.escape)
			{
				// match the next position of the pattern
				return match(
					sequence_begin,
					sequence_end,
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					comparator,
					partial,
					true
				);
			}

			if (
				*pattern_begin == wildcard.set_open and
				check_set_exist(
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					CheckSetState::NOT_OR_FIRST
				)
			)
			{
				// if the nested set does not match successfully, the result will be returned directly (the match failed)
				if (auto result = match_set(
						sequence_begin,
						sequence_end,
						std::ranges::next(pattern_begin),
						pattern_end,
						wildcard,
						comparator,
						MatchSetState::NOT_OR_FIRST_IN
					);
					not result) { return result; }

				const auto pattern_set_end = find_set_end(std::ranges::next(pattern_begin), pattern_end, wildcard, CheckSetState::NOT_OR_FIRST);
				if (is_invalid_sentinel_for(std::ranges::next(pattern_begin), pattern_end, pattern_set_end))
				[[unlikely]]
				{
					// assert?

					return make_match_result(
						ResultDetail::ERROR,
						sequence_begin,
						pattern_begin
					);
				}

				// after the match is successful, skip this nested set to continue matching
				return match(
					std::ranges::next(sequence_begin),
					sequence_end,
					pattern_set_end,
					pattern_end,
					wildcard,
					comparator,
					partial
				);
			}

			if (
				*pattern_begin == wildcard.alt_open and
				check_alt_exist(
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					CheckAltState::NEXT,
					1
				)
			)
			{
				const auto pattern_alt_end = find_alt_end(
					std::ranges::next(pattern_begin),
					pattern_end,
					wildcard,
					CheckAltState::NEXT,
					1
				);
				if (is_invalid_sentinel_for(std::ranges::next(pattern_begin), pattern_end, pattern_alt_end))
				[[unlikely]]
				{
					// assert?

					return make_match_result(
						ResultDetail::ERROR,
						sequence_begin,
						pattern_begin
					);
				}

				const auto pattern_sub_alt_end = find_sub_alt_end(std::ranges::next(pattern_begin), pattern_alt_end, wildcard);
				if (is_invalid_sentinel_for(std::ranges::next(pattern_begin), pattern_alt_end, pattern_sub_alt_end))
				{
					// assert?

					return make_match_result(
						ResultDetail::ERROR,
						sequence_begin,
						pattern_begin
					);
				}

				return match_alt(
					sequence_begin,
					sequence_end,
					std::ranges::next(pattern_begin),
					pattern_sub_alt_end,
					pattern_alt_end,
					pattern_end,
					wildcard,
					comparator,
					partial
				);
			}

			if (
				// not a valid sequence
				sequence_begin == sequence_end or
				// the match not succeed here
				not comparator(*sequence_begin, *pattern_begin))
			{
				return make_match_result(
					ResultDetail::MISMATCH,
					sequence_begin,
					pattern_begin
				);
			}

			// the current position is relatively successful, continue to compare the next position
			return match(
				std::ranges::next(sequence_begin),
				sequence_end,
				std::ranges::next(pattern_begin),
				pattern_end,
				wildcard,
				comparator,
				partial
			);
		}
	}

#if GAL_PROMETHEUS_INTELLISENSE_WORKING
namespace GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_PREFIX :: wildcard
#else
GAL_PROMETHEUS_COMPILER_MODULE_NAMESPACE_EXPORT(wildcard)
#endif
{
	template<std::input_iterator SequenceIterator, std::input_iterator PatternIterator, typename Comparator = std::ranges::equal_to>
	constexpr auto match(
		const SequenceIterator sequence_begin,
		const SequenceIterator sequence_end,
		const PatternIterator pattern_begin,
		const PatternIterator pattern_end,
		const wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>& wildcard = {},
		const Comparator& comparator = {} //
	) noexcept
		-> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<SequenceIterator, PatternIterator>
	{
		return GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::make_full_match_result(
			sequence_begin,
			sequence_end,
			pattern_begin,
			pattern_end,
			GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::match(
				sequence_begin,
				sequence_end,
				pattern_begin,
				pattern_end,
				wildcard,
				comparator
			)
		);
	}

	template<std::ranges::input_range Sequence, std::input_iterator PatternIterator, typename Comparator = std::ranges::equal_to>
	constexpr auto match(
		const Sequence& sequence,
		const PatternIterator pattern_begin,
		const PatternIterator pattern_end,
		const wildcard_type<std::remove_cvref_t<decltype(*std::declval<PatternIterator>())>>& wildcard = {},
		const Comparator& comparator = {} //
	) noexcept
		-> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<decltype(std::ranges::cbegin(std::declval<const Sequence&>())), PatternIterator>
	{
		return wildcard::match(
			std::ranges::begin(sequence),
			std::ranges::end(sequence),
			pattern_begin,
			pattern_end,
			wildcard,
			comparator
		);
	}

	template<std::input_iterator SequenceIterator, std::ranges::input_range Pattern, typename Comparator = std::ranges::equal_to>
	constexpr auto match(
		const SequenceIterator sequence_begin,
		const SequenceIterator sequence_end,
		const Pattern& pattern,
		const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard = {},
		const Comparator& comparator = {} //
	) noexcept
		-> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<SequenceIterator, decltype(std::ranges::cbegin(std::declval<const Pattern&>()))>
	{
		return wildcard::match(
			sequence_begin,
			sequence_end,
			std::ranges::begin(pattern),
			std::ranges::end(pattern),
			wildcard,
			comparator
		);
	}

	template<std::ranges::input_range Sequence, std::ranges::input_range Pattern, typename Comparator = std::ranges::equal_to>
	constexpr auto match(
		const Sequence& sequence,
		const Pattern& pattern,
		const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard = {},
		const Comparator& comparator = {} //
	) noexcept
		-> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<
			decltype(std::ranges::cbegin(std::declval<const Sequence&>())),
			decltype(std::ranges::cbegin(std::declval<const Pattern&>()))
		>
	{
		return wildcard::match(
			std::ranges::begin(sequence),
			std::ranges::end(sequence),
			std::ranges::begin(pattern),
			std::ranges::end(pattern),
			wildcard,
			comparator
		);
	}

	template<std::ranges::range Pattern, typename Comparator = std::ranges::equal_to>
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

		wildcard current_wildcard_;
		GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS comparator current_comparator_;

	public:
		constexpr explicit WildcardMatcher(const pattern_type& pattern, const wildcard& w = wildcard{}, comparator c = comparator{}) noexcept
			: borrow_pattern_begin_{std::ranges::cbegin(pattern)},
			  borrow_pattern_end_{std::ranges::cend(pattern)},
			  current_wildcard_{w},
			  current_comparator_{c} {}

		constexpr auto replace(wildcard& new_wildcard) noexcept -> WildcardMatcher&
		{
			using std::swap;
			swap(current_wildcard_, new_wildcard);
			return *this;
		}

		constexpr auto set(const wildcard& new_wildcard) noexcept -> WildcardMatcher&
		{
			current_wildcard_ = new_wildcard;
			return *this;
		}

		constexpr auto replace(comparator& new_comparator) noexcept -> WildcardMatcher&
		{
			using std::swap;
			swap(current_comparator_, new_comparator);
			return *this;
		}

		constexpr auto set(const comparator& new_comparator) noexcept -> WildcardMatcher&
		{
			current_comparator_ = new_comparator;
			return *this;
		}

		template<std::input_iterator SequenceIterator>
		constexpr auto operator()(
			SequenceIterator sequence_begin,
			SequenceIterator sequence_end //
		) const noexcept -> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<SequenceIterator, const_iterator>
		{
			return prometheus::wildcard::match(sequence_begin, sequence_end, borrow_pattern_begin_, borrow_pattern_end_, current_wildcard_, current_comparator_);
		}

		template<std::ranges::range Sequence>
		constexpr auto operator()(const Sequence& sequence) const noexcept
			-> GAL_PROMETHEUS_COMPILER_MODULE_INTERNAL::full_match_result<decltype(std::ranges::cbegin(std::declval<const Sequence&>())), const_iterator>
		{
			return prometheus::wildcard::match(sequence, borrow_pattern_begin_, borrow_pattern_end_, current_wildcard_, current_comparator_);
		}
	};

	template<std::ranges::range Pattern, typename Comparator>
	constexpr auto make_wildcard_matcher(
		const Pattern& pattern,
		const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard = {},
		Comparator comparator = {}
	) noexcept
		-> WildcardMatcher<Pattern, Comparator>
	{
		return WildcardMatcher<Pattern, Comparator>{
				pattern,
				wildcard,
				comparator
		};
	}

	template<std::ranges::range Pattern, typename Comparator>
	[[nodiscard]] constexpr auto make_wildcard_matcher(
		const Pattern& pattern,
		Comparator comparator = Comparator{},
		const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard = {}
	) noexcept
		-> WildcardMatcher<Pattern, Comparator>
	{
		return WildcardMatcher<Pattern, Comparator>{
				pattern,
				wildcard,
				comparator
		};
	}

	template<std::ranges::range Pattern>
	[[nodiscard]] constexpr auto make_wildcard_matcher(
		const Pattern& pattern,
		const wildcard_type<std::ranges::range_value_t<Pattern>>& wildcard = {}
	) noexcept
		-> WildcardMatcher<Pattern>
	{
		return WildcardMatcher<Pattern>{pattern, wildcard};
	}

	namespace literals
	{
		constexpr auto operator""_wm(
			const char* str,
			const std::size_t size
		) noexcept
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1}))
		{
			return make_wildcard_matcher(std::basic_string_view{str, size + 1});
		}

		constexpr auto operator""_wm(
			const wchar_t* str,
			const std::size_t size
		) noexcept
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1}))
		{
			return make_wildcard_matcher(std::basic_string_view{str, size + 1});
		}

		constexpr auto operator""_wm(
			const char8_t* str,
			const std::size_t size
		) noexcept
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1}))
		{
			return make_wildcard_matcher(std::basic_string_view{str, size + 1});
		}

		constexpr auto operator""_wm(
			const char16_t* str,
			const std::size_t size
		) noexcept
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1}))
		{
			return make_wildcard_matcher(std::basic_string_view{str, size + 1});
		}

		constexpr auto operator""_wm(
			const char32_t* str,
			const std::size_t size
		) noexcept
			-> decltype(make_wildcard_matcher(std::basic_string_view{str, size + 1}))
		{
			return make_wildcard_matcher(std::basic_string_view{str, size + 1});
		}
	} // namespace literals
}
