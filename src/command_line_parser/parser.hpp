// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <algorithm>
#include <memory>
#include <unordered_map>
#include <print>

#include <platform/environment.hpp>
#include <string/string_pool.hpp>

#include <command_line_parser/error.hpp>
#include <command_line_parser/option.hpp>

namespace gal::prometheus::clp
{
	template<regex::string_type StringType>
	class CommandLineOptionParser
	{
	public:
		using string_type = StringType;
		using string_pool_type = string::StringPool<typename string_type::value_type, false>;
		using string_view_type = typename string_pool_type::view_type;

		using option_type = CommandLineOption<string_type, string_view_type>;
		using option_list_type = std::unordered_map<string_view_type, std::shared_ptr<option_type>>;
		using option_list_size_type = typename option_list_type::size_type;

		[[nodiscard]] constexpr static auto default_value(const string_view_type value) noexcept -> typename option_type::default_value_type
		{
			return option_type::default_value(value);
		}

		[[nodiscard]] constexpr static auto implicit_value(const string_view_type value) noexcept -> typename option_type::implicit_value_type
		{
			return option_type::implicit_value(value);
		}

	private:
		string_pool_type string_pool_;

		option_list_type option_list_;

		bool allow_unrecognized_;

		constexpr auto do_add_option(const std::shared_ptr<option_type>& option) -> void
		{
			option->respawn([this](const string_view_type value) -> string_view_type
			{
				return string_pool_.add(value);
			});

			auto add = [this](const string_view_type name, std::shared_ptr<option_type> o) -> void
			{
				const auto [_, inserted] = option_list_.emplace(name, std::move(o));
				if (not inserted)
				{
					CommandLineOptionAlreadyExistsError::panic(name);
				}
			};

			if (not option->option_short_format_.empty())
			{
				add(option->option_short_format_, option);
			}
			add(option->option_long_format_, option);
		}

		constexpr auto do_add_alias(const string_view_type alias_name, const string_view_type target_option_name) -> void
		{
			const auto target_option_it = option_list_.find(target_option_name);
			if (target_option_it == option_list_.end())
			{
				CommandLineOptionRequiredNotPresentError::panic(target_option_name);
			}

			if (const auto it = option_list_.find(alias_name);
				it != option_list_.end())
			{
				CommandLineOptionAlreadyExistsError::panic(alias_name);
			}

			option_list_.emplace(alias_name, target_option_it->second);
		}

	public:
		constexpr explicit CommandLineOptionParser(const bool allow_unrecognized = false) noexcept
			: allow_unrecognized_{allow_unrecognized} {}

		constexpr auto add_option(const string_view_type option, const typename option_type::value_type value) -> CommandLineOptionParser&
		{
			const auto option_names = regex::parse_list(option);
			if (not option_names.has_value())
			{
				CommandLineOptionNameFormatError::panic(option);
			}

			const auto& option_view = *option_names;
			const auto option_size = std::ranges::distance(option_view);

			if (option_size != 1 and option_size != 2)
			{
				CommandLineOptionNameFormatError::panic(option);
			}

			const auto o1 = string_view_type{*std::ranges::begin(option_view)};
			const auto o2 = option_size == 2
				                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
				                : string_view_type{};

			const auto short_format = o1.size() < o2.size() ? o1 : o2;
			const auto long_format = o1.size() < o2.size() ? o2 : o1;

			auto o = std::make_shared<option_type>(short_format, long_format, value);
			this->do_add_option(o);

			return *this;
		}

		constexpr auto add_option(const string_view_type option, const typename option_type::implicit_value_type value) -> CommandLineOptionParser&
		{
			return this->add_option(option, value + typename option_type::default_value_type{});
		}

		constexpr auto add_option(const string_view_type option, const typename option_type::default_value_type value) -> CommandLineOptionParser&
		{
			return this->add_option(option, value + typename option_type::implicit_value_type{});
		}

		constexpr auto add_option(const string_view_type option) -> CommandLineOptionParser&
		{
			return this->add_option(option, typename option_type::value_type{});
		}

		constexpr auto add_alias(const string_view_type alias_name, const string_view_type target_option_name) -> CommandLineOptionParser&
		{
			const auto option_names = regex::parse_list(alias_name);
			if (not option_names.has_value())
			{
				CommandLineOptionNameFormatError::panic(alias_name);
			}

			const auto& option_view = *option_names;
			const auto option_size = std::ranges::distance(option_view);

			if (option_size != 1 and option_size != 2)
			{
				CommandLineOptionNameFormatError::panic(alias_name);
			}

			const auto o1 = string_view_type{*std::ranges::begin(option_view)};
			const auto o2 = option_size == 2
				                ? string_view_type{*std::ranges::next(std::ranges::begin(option_view), 1)}
				                : string_view_type{};

			this->do_add_alias(o1, target_option_name);
			if (not o2.empty())
			{
				this->do_add_alias(o2, target_option_name);
			}

			return *this;
		}

		template<std::input_iterator Begin, std::sentinel_for<Begin> End>
		constexpr auto parse(const Begin begin, const End end) -> void
		{
			std::vector<string_view_type> unmatched{};

			std::ranges::for_each(
				begin,
				end,
				[this, &unmatched](const auto& string) -> void
				{
					const auto option = regex::parse_option(string);

					if (not option.has_value())
					{
						unmatched.emplace_back(string);
						return;
					}

					auto it = option_list_.find(option->name);
					if (it == option_list_.end())
					{
						unmatched.emplace_back(option->name);
						return;
					}

					// Since we allow `--option`, we are not sure here whether the string is `--option` or `-option`.
					if (option->value.empty())
					{
						#if __cpp_lib_ranges_starts_ends_with >= 202106L
						if (std::ranges::starts_with(string, string_view_type{"--"}))
						#else
						if (std::ranges::equal(string_view_type{"--"}, string))
						#endif
						{
							// --option
							it->second->set_value_default();
						}
						else
						{
							// -option
							it->second->set_value();
						}
					}
					else
					{
						const auto allocated_value = string_pool_.add(option->value);
						it->second->set_value(allocated_value);
					}
				}
			);

			if (not unmatched.empty())
			{
				string_type options{};

				std::ranges::for_each(
					unmatched,
					[&options](const string_view_type option)
					{
						options.push_back('\t');
						options.append(" - ");
						options.append(option);
						options.push_back('\n');
					}
				);

				if (not allow_unrecognized_)
				{
					CommandLineOptionUnrecognizedError::panic(options);
				}

				std::println(stderr, "Unrecognized option:\n {}", options);
			}
		}

		template<std::ranges::input_range Range>
		constexpr auto parse(const Range& range) -> void
		{
			this->parse(std::ranges::begin(range), std::ranges::end(range));
		}

		auto parse() -> void
		{
			const auto args = platform::command_args();
			this->parse(
				args |
				std::views::drop(1) | // skip argv[0]
				std::views::transform(
					[](const auto& p) noexcept -> std::string_view
					{
						return {p};
					}
				)
			);
		}

		[[nodiscard]] constexpr auto contains(const string_view_type arg_name) const -> bool
		{
			return option_list_.contains(arg_name);
		}

		[[nodiscard]] constexpr auto count(const string_view_type arg_name) const -> option_list_size_type
		{
			return option_list_.count(arg_name);
		}

		[[nodiscard]] constexpr auto operator[](const string_view_type arg_name) const -> const option_type&
		{
			const auto it = option_list_.find(arg_name);
			if (it == option_list_.end())
			{
				CommandLineOptionRequiredNotPresentError::panic(arg_name);
			}

			return *it->second;
		}
	};
}
