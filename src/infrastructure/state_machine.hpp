#pragma once

#include <meta/string.hpp>
#include <functional/functional.hpp>
#include <functional/template_parameter_list.hpp>

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

		constexpr auto state_end = meta::to_char_array<meta::basic_fixed_string{"infrastructure.state_machine.internal_end_state"}>();

		// =====================================
		// transition
		// =====================================

		template<typename Function, typename EventType, typename... Args>
		[[nodiscard]] constexpr auto invoke(Function&& function, const EventType& event, Args&&... args) noexcept(false) -> auto
		{
			if constexpr (requires { std::invoke(std::forward<Function>(function), event, std::forward<Args>(args)...); }) { return std::invoke(std::forward<Function>(function), event, std::forward<Args>(args)...); }
			else if constexpr (requires { std::invoke(std::forward<Function>(function), event); }) { return std::invoke(std::forward<Function>(function), event); }
			else if constexpr (requires { std::invoke(std::forward<Function>(function), std::forward<Args>(args)...); }) { return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...); }
			else if constexpr (requires { std::invoke(std::forward<Function>(function)); }) { return std::invoke(std::forward<Function>(function)); }
			else { GAL_PROMETHEUS_STATIC_UNREACHABLE(); }
		}

		struct nothing {};

		constexpr auto absence = []() noexcept { return true; };
		constexpr auto ignore  = []() noexcept {};
		using ignore_type = decltype(ignore);

		template<bool IsEntryPoint, state From, state To, typename EventType = nothing, typename GuardType = decltype(absence), typename ActionType = ignore_type, typename SentryEntryType = ignore_type, typename SentryExitType = ignore_type>
		struct transition;

		template<typename>
		struct is_transition : std::false_type {};

		template<bool IsEntryPoint, state From, state To, typename EventType, typename GuardType, typename ActionType, typename SentryEntryType, typename SentryExitType>
		struct is_transition<transition<IsEntryPoint, From, To, EventType, GuardType, ActionType, SentryEntryType, SentryExitType>> : std::true_type {};

		template<typename T>
		constexpr auto is_transition_v = is_transition<T>::value;
		template<typename T>
		concept transition_t = is_transition_v<T>;

		template<bool IsEntryPoint, state From, state To, typename EventType, typename GuardType, typename ActionType, typename SentryEntryType, typename SentryExitType>
		struct transition
		{
			friend StateMachine;

			constexpr static auto is_entry_point = IsEntryPoint;
			constexpr static auto from           = From;
			constexpr static auto to             = To;

			using from_type = std::decay_t<decltype(from)>;
			using to_type = std::decay_t<decltype(to)>;
			using event_type = EventType;
			using guard_type = GuardType;
			using action_type = ActionType;
			using sentry_entry_type = SentryEntryType;
			using sentry_exit_type = SentryExitType;

			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS guard_type        guard{};
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS action_type       action{};
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS sentry_entry_type sentry_entry{};
			GAL_PROMETHEUS_NO_UNIQUE_ADDRESS sentry_exit_type  sentry_exit{};

			// =======================
			// entry_state.start_from_here().when<my_event>().iff(guard).then(action).end<end_state>()
			// =======================

			[[nodiscard]] constexpr auto start_from_here() && noexcept -> transition<true, from, to, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type>//
				requires(not is_entry_point)                                                                                                                                  //
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename Event>
			[[nodiscard]] constexpr auto when() && noexcept -> transition<is_entry_point, from, to, Event, guard_type, action_type, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(const Guard& new_guard) && noexcept -> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = new_guard, .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename Guard>
			[[nodiscard]] constexpr auto iff(Guard&& new_guard) && noexcept -> transition<is_entry_point, from, to, event_type, std::decay_t<Guard>, action_type, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::forward<Guard>(new_guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(const Action& new_action) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = new_action, .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename Action>
			[[nodiscard]] constexpr auto then(Action&& new_action) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, std::decay_t<Action>, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::forward<Action>(new_action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end() && noexcept -> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<transition_t EndTransition>
			[[nodiscard]] constexpr auto end(const EndTransition&) && noexcept -> transition<is_entry_point, from, EndTransition::from, event_type, guard_type, action_type, sentry_entry_type, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(const SentryEntry& new_entry) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = new_entry, .sentry_exit = std::move(sentry_exit)};
			}

			template<typename SentryEntry>
			[[nodiscard]] constexpr auto on_entry(SentryEntry&& new_entry) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, action_type, std::decay_t<SentryEntry>, sentry_exit_type>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::forward<SentryEntry>(new_entry), .sentry_exit = std::move(sentry_exit)};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(const SentryExit& new_exit) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = new_exit};
			}

			template<typename SentryExit>
			[[nodiscard]] constexpr auto on_exit(SentryExit&& new_exit) && noexcept -> transition<is_entry_point, from, to, event_type, guard_type, action_type, sentry_entry_type, std::decay_t<SentryExit>>//
			{
				return {.guard = std::move(guard), .action = std::move(action), .sentry_entry = std::move(sentry_entry), .sentry_exit = std::forward<SentryExit>(new_exit)};
			}

		private:
			template<typename StateMachine, typename... Args>
			[[nodiscard]] constexpr auto operator()(StateMachine& state_machine, const event_type& event, Args&&... args) -> bool
			{
				if (state_machine_detail::invoke(guard, event, std::forward<Args>(args)...))
				{
					state_machine_detail::invoke(action, event, std::forward<Args>(args)...);

					if constexpr (to != state_end)
					{
						if constexpr (not std::is_same_v<sentry_exit_type, ignore_type>)//
						{
							state_machine_detail::invoke(sentry_exit, event, std::forward<Args>(args)...);
						}
						state_machine.template transform<to_type>();

						// using to_transition_type = typename StateMachine::template transition_of_state<to_type>;
						// using to_transition_sentry_entry_type = typename to_transition_type::sentry_entry_type;
						// if constexpr (not std::is_same_v<to_transition_sentry_entry_type, ignore_type>)//
						// {
						// 	auto& to_sentry_entry = state_machine.template get_transition<to_transition_type>().sentry_entry;
						// 	state_machine_detail::invoke(to_sentry_entry, event, std::forward<Args>(args)...);
						// }

						using to_transitions_type = typename StateMachine::template transitions_of_state<to_type>;
						[&]<std::size_t... Index>(std::index_sequence<Index...>) noexcept
						{
							const auto do_invoke = [&]<transition_t T>() noexcept
							{
								using to_transition_sentry_entry_type = typename T::sentry_entry_type;
								if constexpr (not std::is_same_v<to_transition_sentry_entry_type, ignore_type>)//
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

		template<transition_t Transition>
		struct transition_traits_entry_point
		{
			constexpr static auto value = Transition::is_entry_point;
		};

		template<transition_t Transition>
		struct transition_traits_from
		{
			using type = typename Transition::from_type;
		};

		template<transition_t Transition>
		struct transition_traits_event
		{
			using type = typename Transition::event_type;
		};

		template<typename From, transition_t Transition>
		struct transition_traits_same_from_state
		{
			constexpr static auto value = std::is_same_v<From, typename Transition::from_type>;
		};

		template<transition_t... Transition>
		using transition_list_type = functional::overloaded<Transition...>;

		template<typename>
		struct is_transition_list : std::false_type {};

		template<transition_t... Transition>
		struct is_transition_list<transition_list_type<Transition...>> : std::true_type {};

		template<typename T>
		constexpr auto is_transition_list_v = is_transition_list<T>::value;
		template<typename T>
		concept transition_list_t = is_transition_list_v<T>;

		template<template<typename...> typename List, typename... Transitions>
		class StateMachine<List<Transitions...>>
		{
			friend transition;

		public:
			constexpr static auto transition_type_list = functional::list<Transitions...>;

			// from states
			constexpr static auto states = transition_type_list.template projection<transition_traits_from>().unique();

			template<typename EventType>
			using transition_of_event = typename functional::list_type<transition_type_list>::template nth_type<transition_type_list.template projection<transition_traits_event>().template index_of<EventType>()>;

			template<typename State>
			using transition_of_state = typename functional::list_type<transition_type_list>::template nth_type<transition_type_list.template projection<transition_traits_from>().template index_of<State>()>;

			template<typename State>
			using transitions_of_state = functional::list_type<transition_type_list.template sub_list<State, transition_traits_same_from_state>()>;

		private:
			List<Transitions...> transitions_;
			std::size_t          current_state_index_;

			[[nodiscard]] constexpr auto entry_index() noexcept -> std::size_t
			{
				constexpr auto target_transition_index = transition_type_list.template index_of<transition_traits_entry_point>();
				using target_transition_type = typename functional::list_type<transition_type_list>::template nth_type<target_transition_index>;
				[[maybe_unused]] constexpr auto target_transition_from = target_transition_type::from;
				using target_transition_from_type = std::decay_t<decltype(target_transition_from)>;

				return states.template index_of<target_transition_from_type>();
			}

			template<typename State>
			constexpr auto transform() noexcept -> void//
			{
				current_state_index_ = states.template index_of<State>();
			}

			// friend transition => operator()
			template<transition_t Transition>
				requires(transition_type_list.template any<Transition>())
			[[nodiscard]] constexpr auto get_transition() noexcept -> Transition& { return static_cast<Transition&>(transitions_); }

		public:
			template<typename L>
			constexpr explicit StateMachine(L&& transitions) noexcept// NOLINT(bugprone-forwarding-reference-overload)
				: transitions_{std::forward<L>(transitions)},
				  current_state_index_{entry_index()} {}

			template<meta::basic_fixed_string State>
			[[nodiscard]] constexpr auto is() const noexcept -> bool
			{
				[[maybe_unused]] constexpr auto state = meta::to_char_array<State>();
				using state_type = std::decay_t<decltype(state)>;

				if constexpr (not states.template any<state_type>()) { return false; }
				else { return states.template index_of<state_type>() == current_state_index_; }
			}

			template<transition_t Transition>
			[[nodiscard]] constexpr auto is(const Transition&) const noexcept -> bool
			{
				[[maybe_unused]] constexpr auto state = Transition::from;
				using state_type = std::decay_t<decltype(state)>;

				if constexpr (not states.template any<state_type>())//
				{
					return false;
				}
				else//
				{
					return states.template index_of<state_type>() == current_state_index_;
				}
			}

			template<typename EventType, transition_t Transition = transition_of_event<EventType>, typename... Args>
			constexpr auto process(const EventType& event, Args&&... args) noexcept(false) -> bool
			{
				[[maybe_unused]] constexpr auto transition_from = Transition::from;
				using transition_from_type = std::decay_t<decltype(transition_from)>;

				if (constexpr auto transition_from_index = states.template index_of<transition_from_type>();
					transition_from_index == current_state_index_) { return get_transition<Transition>()(*this, event, std::forward<Args>(args)...); }

				return false;
			}
		};
	}

	GAL_PROMETHEUS_MODULE_EXPORT_BEGIN

	template<meta::basic_fixed_string State>
	[[nodiscard]] constexpr auto operator""_s() noexcept -> state_machine_detail::transition<false, meta::to_char_array<State>(), state_machine_detail::state_end> { return {}; }

	template<typename... Transitions>
		requires((Transitions::is_entry_point + ...) == 1)
	using transition_list = state_machine_detail::transition_list_type<Transitions...>;

	template<typename Invocable>
		requires std::is_invocable_v<Invocable>
	struct state_machine final : state_machine_detail::StateMachine<decltype(std::declval<Invocable>()())>
	{
		constexpr explicit(false) state_machine(Invocable function) noexcept
			: state_machine_detail::StateMachine<decltype(std::declval<Invocable>()())>{function()} {}
	};

	template<typename Invocable>
	state_machine(Invocable) -> state_machine<Invocable>;

	GAL_PROMETHEUS_MODULE_EXPORT_END
}
