// This file is part of prometheus
// Copyright (C) 2022-2024 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if GAL_PROMETHEUS_USE_MODULE
module;

#include <prometheus/macro.hpp>

export module gal.prometheus.infrastructure:state_machine;

import std;
import gal.prometheus.functional;
import gal.prometheus.meta;

#else
#pragma once

#include <type_traits>
#include <utility>

#include <prometheus/macro.hpp>
#include <functional/functional.ixx>
#include <meta/meta.ixx>

#endif

#if defined(GAL_PROMETHEUS_COMPILER_CLANG_CL) or defined(GAL_PROMETHEUS_COMPILER_CLANG) or defined(GAL_PROMETHEUS_COMPILER_GNU)
	#define STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE auto
	#define STATE_MACHINE_WORKAROUND_REQUIRED
#else
#define STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE state
#endif

namespace gal::prometheus::infrastructure
{
	namespace state_machine_detail
	{
		template<typename>
		class StateMachine;

		// =====================================
		// state
		// =====================================

		// We need to determine how many (non-repeating) states there are in total based on `state`, so we can't use `basic_fixed_string` here.
		template<char... Cs>
		using state = meta::basic_char_array<char, Cs...>;

		constexpr auto state_continue = meta::to_char_array<meta::basic_fixed_string{"infrastructure.state_machine.internal_continue_state"}>();
		constexpr auto state_any = meta::to_char_array<meta::basic_fixed_string{"infrastructure.state_machine.internal_any_state"}>();

		// =====================================
		// transition
		// =====================================

		template<typename Function, typename EventType, typename... Args>
		[[nodiscard]] constexpr auto invoke(Function&& function, const EventType& event, Args&&... args) noexcept(false) -> auto
		{
			if constexpr (requires { std::invoke(std::forward<Function>(function), event, std::forward<Args>(args)...); })
			{
				return std::invoke(std::forward<Function>(function), event, std::forward<Args>(args)...);
			}
			else if constexpr (requires { std::invoke(std::forward<Function>(function), event); })
			{
				return std::invoke(std::forward<Function>(function), event);
			}
			else if constexpr (requires { std::invoke(std::forward<Function>(function), std::forward<Args>(args)...); })
			{
				return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
			}
			else if constexpr (requires { std::invoke(std::forward<Function>(function)); }) { return std::invoke(std::forward<Function>(function)); }
			else { GAL_PROMETHEUS_SEMANTIC_STATIC_UNREACHABLE(); }
		}

		struct nothing {};

		constexpr auto absence = []() noexcept { return true; };
		using absence_type = std::decay_t<decltype(absence)>;
		constexpr auto ignore = []() noexcept {};
		using ignore_type = std::decay_t<decltype(ignore)>;

		template<
			bool IsEntryPoint,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE From,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE To,
			typename EventType = nothing,
			typename GuardType = absence_type,
			typename ActionType = ignore_type,
			typename SentryEntryType = ignore_type,
			typename SentryExitType = ignore_type>
			requires(From != state_continue)
		struct transition;

		template<typename>
		struct is_transition : std::false_type {};

		template<
			bool IsEntryPoint,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE From,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE To,
			typename EventType,
			typename GuardType,
			typename ActionType,
			typename SentryEntryType,
			typename SentryExitType
		>
		struct is_transition<transition<IsEntryPoint, From, To, EventType, GuardType, ActionType, SentryEntryType, SentryExitType>> : std::true_type {
		};

		template<typename T>
		constexpr auto is_transition_v = is_transition<T>::value;
		template<typename T>
		concept transition_t = is_transition_v<T>;

		template<
			bool IsEntryPoint,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE From,
			STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE To,
			typename EventType,
			typename GuardType,
			typename ActionType,
			typename SentryEntryType,
			typename SentryExitType
		>
			requires(From != state_continue)
		struct transition
		{
			#if not defined(STATE_MACHINE_WORKAROUND_REQUIRED)
			friend StateMachine;
			#endif

			constexpr static auto is_entry_point = IsEntryPoint;
			constexpr static auto from = From;
			constexpr static auto to = To;

			using from_type = std::decay_t<decltype(from)>;
			using to_type = std::decay_t<decltype(to)>;
			using event_type = EventType;
			using guard_type = GuardType;
			using action_type = ActionType;
			using sentry_entry_type = SentryEntryType;
			using sentry_exit_type = SentryExitType;

			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS guard_type guard{};
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS action_type action{};
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS sentry_entry_type sentry_entry{};
			GAL_PROMETHEUS_COMPILER_NO_UNIQUE_ADDRESS sentry_exit_type sentry_exit{};

			// =======================
			// entry_state.start_from_here().when<my_event>().iff(guard).then(action).end<end_state>().on_entry(entry_action).on_exit(exit_action)
			// =======================

			[[nodiscard]] constexpr auto start_from_here() && noexcept
				-> transition<true, from, to, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type> //
				requires(not is_entry_point and from != state_any) //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename... Event>
			[[nodiscard]] constexpr auto when() const & noexcept
				-> transition<
					is_entry_point,
					from,
					to,
					functional::type_list_type<functional::type_list<Event...>>,
					guard_type,
					action_type,
					sentry_entry_type,
					sentry_exit_type
				> //
			{
				return {.guard = guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<typename... Event>
			[[nodiscard]] constexpr auto when() && noexcept
				-> transition<
					is_entry_point,
					from,
					to,
					functional::type_list_type<functional::type_list<Event...>>,
					guard_type,
					action_type,
					sentry_entry_type,
					sentry_exit_type
				> //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(const Guard& new_guard) const & noexcept
				-> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = new_guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(Guard&& new_guard) const & noexcept
				-> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = std::forward<Guard>(new_guard), .action = action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(const Guard& new_guard) && noexcept
				-> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = new_guard,
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(Guard&& new_guard) && noexcept
				-> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = std::forward<Guard>(new_guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(const Action& new_action) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = guard, .action = new_action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(Action&& new_action) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = guard, .action = std::forward<Action>(new_action), .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(const Action& new_action) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = std::move(guard),
						.action = new_action,
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(Action&& new_action) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = std::move(guard),
						.action = std::forward<Action>(new_action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end() const & noexcept
				-> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end(const EndTransition&) const & noexcept
				-> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {.guard = guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = sentry_exit};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end() && noexcept
				-> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end(const EndTransition&) && noexcept
				-> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type> //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(const SentryEntry& new_entry) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type> //
			{
				return {.guard = guard, .action = action, .sentry_entry = new_entry, .sentry_exit = sentry_exit};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(SentryEntry&& new_entry) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type> //
			{
				return {.guard = guard, .action = action, .sentry_entry = std::forward<SentryEntry>(new_entry), .sentry_exit = sentry_exit};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(const SentryEntry& new_entry) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type> //
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = new_entry, .sentry_exit = std::move(sentry_exit)};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(SentryEntry&& new_entry) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type> //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::forward<SentryEntry>(new_entry),
						.sentry_exit = std::move(sentry_exit)
				};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(const SentryExit& new_exit) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>> //
			{
				return {.guard = guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = new_exit};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(SentryExit&& new_exit) const & noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>> //
			{
				return {.guard = guard, .action = action, .sentry_entry = sentry_entry, .sentry_exit = std::forward<SentryExit>(new_exit)};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(const SentryExit& new_exit) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>> //
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = new_exit};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(SentryExit&& new_exit) && noexcept
				-> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>> //
			{
				return {
						.guard = std::move(guard),
						.action = std::move(action),
						.sentry_entry = std::move(sentry_entry),
						.sentry_exit = std::forward<SentryExit>(new_exit)
				};
			}

			#if not defined(STATE_MACHINE_WORKAROUND_REQUIRED)

		private:
			#endif

			template<std::size_t EntryIndex, typename StateMachine, typename Event, typename... Args>
				requires(event_type{}.template any<Event>())
			[[nodiscard]] constexpr auto operator()(StateMachine& state_machine, const Event& event, Args&&... args) -> bool
			{
				if (state_machine_detail::invoke(guard, event, std::forward<Args>(args)...))
				{
					state_machine_detail::invoke(action, event, std::forward<Args>(args)...);

					if constexpr (to != state_continue)
					{
						if constexpr (not std::is_same_v<sentry_exit_type, ignore_type>) //
						{
							state_machine_detail::invoke(sentry_exit, event, std::forward<Args>(args)...);
						}
						state_machine.template transform<EntryIndex, to_type>();

						using to_transitions_type = typename StateMachine::template transitions_type_of_state<to_type>;
						[&]<std::size_t... Index>(std::index_sequence<Index...>) noexcept
						{
							const auto do_invoke = [&]<transition_t T>() noexcept
							{
								using to_transition_sentry_entry_type = typename T::sentry_entry_type;
								if constexpr (not std::is_same_v<to_transition_sentry_entry_type, ignore_type>) //
								{
									auto& to_sentry_entry = state_machine.template get_transition<T>().sentry_entry;
									state_machine_detail::invoke(to_sentry_entry, event, std::forward<Args>(args)...);
								}
							};

							(do_invoke.template operator()<typename to_transitions_type::template nth_type<Index>>(), ...);
						}(std::make_index_sequence<to_transitions_type::types_size>{});
					}

					return true;
				}

				return false;
			}
		};

		template<transition_t... Transition>
		using transition_list_type = functional::overloaded<Transition...>;

		template<typename>
		struct is_transition_list : std::false_type {};

		template<transition_t... Transition>
		struct is_transition_list<transition_list_type<Transition...>> : std::true_type {};

		#if defined(STATE_MACHINE_WORKAROUND_REQUIRED)
		// @see transition_list below
		template<template<typename...> typename T, transition_t... Transition>
			requires std::is_base_of_v<transition_list_type<Transition...>, T<Transition...>>
		struct is_transition_list<T<Transition...>> : std::true_type {};
		#endif

		template<typename T>
		constexpr auto is_transition_list_v = is_transition_list<T>::value;
		template<typename T>
		concept transition_list_t = is_transition_list_v<T>;

		template<typename... Transitions>
		struct transitions_mapping_type
		{
		private:
			constexpr static auto list = functional::type_list<Transitions...>;

			template<transition_t Transition>
			struct is_entry_point
			{
				constexpr static auto value = Transition::is_entry_point;
			};

			template<transition_t Transition>
			struct projection_from
			{
				using type = typename Transition::from_type;
			};

			template<typename State, transition_t Transition>
			struct contains_state
			{
				constexpr static auto value = std::is_same_v<State, typename Transition::from_type>;
			};

			template<typename EventType, transition_t Transition>
			struct contains_event
			{
				constexpr static auto value = typename Transition::event_type{}.template any<EventType>();
			};

		public:
			constexpr static auto transitions_list = list;
			using transitions_list_type = functional::type_list_type<transitions_list>;

			constexpr static auto entry_point_list = list.template sub_list<is_entry_point>();
			using entry_point_list_type = functional::type_list_type<entry_point_list>;

			constexpr static auto state_list = list.template projection<projection_from>().unique();
			using state_list_type = functional::type_list_type<state_list>;

			template<typename State>
			constexpr static auto state_to_transitions_list = list.template sub_list<State, contains_state>();
			template<typename State>
			using state_to_transitions_list_type = functional::type_list_type<state_to_transitions_list<State>>;

			template<typename EventType>
			constexpr static auto event_to_transitions_list = list.template sub_list<EventType, contains_event>();
			template<typename EventType>
			using event_to_transitions_list_type = functional::type_list_type<event_to_transitions_list<EventType>>;
		};

		template<template<typename...> typename List, typename... Transitions>
		class StateMachine<List<Transitions...>>
		{
			#if not defined(STATE_MACHINE_WORKAROUND_REQUIRED)
			friend transition;
			#endif

		public:
			// mapping list
			using transition_mapping = transitions_mapping_type<Transitions...>;

			// transitions
			constexpr static auto transitions_list = transition_mapping::transitions_list;
			using transitions_list_type = typename transition_mapping::transitions_list_type;

			// entry_point
			constexpr static auto entry_point_list = transition_mapping::entry_point_list;
			using entry_point_list_type = typename transition_mapping::entry_point_list_type;

			// states
			constexpr static auto state_list = transition_mapping::state_list;
			using state_list_type = typename transition_mapping::state_list_type;

			// mapping state=>transition(s)
			template<typename State>
			constexpr static auto transitions_of_state = transition_mapping::template state_to_transitions_list<State>;
			template<typename State>
			using transitions_type_of_state = typename transition_mapping::template state_to_transitions_list_type<State>;

			// mapping event=>transition(s)
			template<typename EventType>
			constexpr static auto transitions_of_event = transition_mapping::template event_to_transitions_list<EventType>;
			template<typename EventType>
			using transitions_type_of_event = typename transition_mapping::template event_to_transitions_list_type<EventType>;

		private:
			using state_index_type = std::array<std::size_t, entry_point_list.size()>;

			List<Transitions...> transitions_;
			state_index_type current_state_index_;

			[[nodiscard]] constexpr auto entry_index() noexcept -> state_index_type
			{
				return []<std::size_t... Index>(std::index_sequence<Index...>) noexcept
				{
					const auto do_get = []<std::size_t I>() noexcept
					{
						using entry_point_transition_type = typename entry_point_list_type::template nth_type<I>;
						using entry_point_transition_from_type = typename entry_point_transition_type::from_type;

						return state_list.template index_of<entry_point_transition_from_type>();
					};

					return state_index_type{{do_get.template operator()<Index>()...}};
				}(std::make_index_sequence<entry_point_list.size()>{});
			}

			#if defined(STATE_MACHINE_WORKAROUND_REQUIRED)

		public:
			#endif

			// friend ==> transition::operator()
			template<std::size_t EntryIndex, typename State>
			constexpr auto transform() noexcept -> void //
			{
				current_state_index_[EntryIndex] = state_list.template index_of<State>();
			}

			// friend ==> transition::operator()
			template<transition_t Transition>
				requires(transitions_list.template any<Transition>())
			[[nodiscard]] constexpr auto get_transition() noexcept -> Transition& { return static_cast<Transition&>(transitions_); }

			#if defined(STATE_MACHINE_WORKAROUND_REQUIRED)

		private:
			#endif

		public:
			template<typename L>
			constexpr explicit StateMachine(L&& transitions) noexcept // NOLINT(bugprone-forwarding-reference-overload)
				: transitions_{std::forward<L>(transitions)},
				  current_state_index_{entry_index()} {}

			template<meta::basic_fixed_string... RequiredStates>
				requires(sizeof...(RequiredStates) == entry_point_list.size())
			[[nodiscard]] constexpr auto is() const noexcept -> bool
			{
				return [this]<std::size_t... Index>(std::index_sequence<Index...>) noexcept
				{
					const auto do_check = [this]<meta::basic_fixed_string S, std::size_t I>() noexcept
					{
						[[maybe_unused]] constexpr auto state = meta::to_char_array<S>();
						using state_type = std::decay_t<decltype(state)>;

						if constexpr (not state_list.template any<state_type>()) //
						{
							return false;
						}
						else { return state_list.template index_of<state_type>() == current_state_index_[I]; }
					};

					return ((do_check.template operator()<RequiredStates, Index>()) and ...);
				}(std::make_index_sequence<entry_point_list.size()>{});
			}

			template<transition_t... RequiredTransitions>
				requires(sizeof...(RequiredTransitions) == entry_point_list.size())
			[[nodiscard]] constexpr auto is(const RequiredTransitions&...) const noexcept -> bool
			{
				return [this]<std::size_t... Index>(std::index_sequence<Index...>) noexcept
				{
					const auto do_check = [this]<transition_t Transition, std::size_t I>() noexcept
					{
						using state_type = typename Transition::from_type;

						if constexpr (not state_list.template any<state_type>()) //
						{
							return false;
						}
						else { return state_list.template index_of<state_type>() == current_state_index_[I]; }
					};

					return ((do_check.template operator()<RequiredTransitions, Index>()) and ...);
				}(std::make_index_sequence<entry_point_list.size()>{});
			}

			// note: Even if multiple entry_points are in the same state, only the first transition (that can handle the target event) will be executed.
			template<typename EventType, typename TransitionsOfEvent = transitions_type_of_event<EventType>, typename... Args>
			constexpr auto process(const EventType& event, Args... args) noexcept(false) -> bool
			{
				return [&]<std::size_t... EntriesIndex>(std::index_sequence<EntriesIndex...>) noexcept
				{
					const auto do_check_each_state =
							[&]<std::size_t EntryIndex, std::size_t... TransitionsIndex>(std::index_sequence<TransitionsIndex...>) noexcept
					{
						const auto do_check_each_transition = [&]<std::size_t TransitionIndex>() noexcept
						{
							using this_transition_type = typename TransitionsOfEvent::template nth_type<TransitionIndex>;
							using this_transition_from_type = typename this_transition_type::from_type;

							constexpr auto this_transition_from = this_transition_type::from;
							constexpr auto this_transition_from_index = state_list.template index_of<this_transition_from_type>();

							if ((this_transition_from == state_any) or (this_transition_from_index == current_state_index_[EntryIndex])) //
							{
								return get_transition<this_transition_type>().template operator()<EntryIndex>(
										*this,
										event,
										std::forward<Args>(args)...
										);
							}

							return false;
						};

						return (do_check_each_transition.template operator()<TransitionsIndex>() or ...);
					};

					#if not defined(GAL_PROMETHEUS_COMPILER_GNU)
					return (
						do_check_each_state.template operator()<EntriesIndex>(std::make_index_sequence<transitions_of_event<EventType>.size()>{})
						or ...
					);
					#else
					return (do_check_each_state.template operator()<EntriesIndex>(std::make_index_sequence<transitions_type_of_event<EventType>::types_size>{}) or ...);
					#endif
				}(std::make_index_sequence<entry_point_list.size()>{});
			}
		};
	}

	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_BEGIN
		template<meta::basic_fixed_string State>
		[[nodiscard]] constexpr auto operator""_s() noexcept
			-> state_machine_detail::transition<false, meta::to_char_array<State>(), state_machine_detail::state_continue> { return {}; }

		// note: `any_state` will only change the state of the first entry_point when an event is processed.
		// transition_list{
		//	s1.start_from_here()...,
		//	s2.start_from_here()...,
		//	s3...,
		//	s4...,
		//	any_state.when<e>().end(s4)...,
		// }
		// state_machine.is(s1, s2);
		// state_machine.process(e{});
		// state_machine.is(s4, s2); // <== only the first entry_point changes state.
		constexpr auto any_state = state_machine_detail::transition<false, state_machine_detail::state_any, state_machine_detail::state_continue>{};

		template<state_machine_detail::transition_t... Transitions>
			requires((Transitions::is_entry_point + ...) >= 1)
		#if defined(STATE_MACHINE_WORKAROUND_REQUIRED)
		struct transition_list : state_machine_detail::transition_list_type<Transitions...>
		{
			using state_machine_detail::transition_list_type<Transitions...>::transition_list_type;
		};

		template<state_machine_detail::transition_t... Transitions>
		transition_list(Transitions && ...) -> transition_list<Transitions...>;
		#else
		using transition_list = state_machine_detail::transition_list_type<Transitions...>;
		#endif

		template<typename Invocable>
			requires std::is_invocable_v<Invocable> and state_machine_detail::is_transition_list_v<decltype(std::declval<Invocable>()())>
		struct state_machine final : state_machine_detail::StateMachine<decltype(std::declval<Invocable>()())>
		{
			constexpr explicit(false) state_machine(Invocable function) noexcept // NOLINT(*-explicit-constructor)
				: state_machine_detail::StateMachine<decltype(std::declval<Invocable>()())>{function()} {}
		};

		template<typename Invocable>
		state_machine(Invocable) -> state_machine<Invocable>;
	GAL_PROMETHEUS_COMPILER_MODULE_EXPORT_END
}

#undef STATE_MACHINE_WORKAROUND_TEMPLATE_STATE_TYPE
#if defined(STATE_MACHINE_WORKAROUND_REQUIRED)
#undef STATE_MACHINE_WORKAROUND_REQUIRED
#endif
