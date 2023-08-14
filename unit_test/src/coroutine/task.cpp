#include <prometheus/macro.hpp>

import std;
import gal.prometheus.test;
import gal.prometheus.coroutine;

namespace
{
	using namespace gal::prometheus;
	using namespace test;
	using namespace coroutine;

	// It's a good idea to read what's in the link before reading the code below.
	// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rcoro-capture

	GAL_PROMETHEUS_NO_DESTROY suite test_coroutine_task = []
	{
		"void"_test = []
		{
			auto return_void = []() -> Task<void> { co_return; }();

			expect((return_void.done() != "not run yet"_b) >> fatal);

			return_void.resume();

			expect((return_void.done() == "done"_b) >> fatal) << "just done";
		};

		"hello_world"_test = []
		{
			using task_type = Task<std::string>;

			auto say_hello = []() -> task_type { co_return "hello"; }();
			auto say_world = []() -> task_type { co_return "world"; }();

			expect((say_hello.promise().result().empty()) >> fatal) << "empty yet";
			expect((say_world.promise().result().empty()) >> fatal) << "empty yet";

			say_hello.resume();
			say_world.resume();

			expect((say_hello.done() == "done"_b) >> fatal) << "just done";
			expect((say_world.done() == "done"_b) >> fatal) << "just done";

			const auto hello = say_hello.promise().result();
			const auto world = std::move(say_world).promise().result();

			expect((hello == "hello") >> fatal);
			expect((world == "world") >> fatal);

			expect((say_hello.promise().result().empty() != "not empty"_b) >> fatal) << "copy the string";
			expect((say_world.promise().result().empty() == "empty"_b) >> fatal) << "move the string";// NOLINT // use-after-move
		};

		"exception"_test = []
		{
			using task_type = Task<std::string>;

			auto throw_exception = []() -> task_type
			{
				throw std::runtime_error{"exception raise!"};
				// ReSharper disable once CppUnreachableCode
				co_return "We'll never get here, but still need this!";
			}();

			expect((throw_exception.promise().has_exception() != "not throw yet"_b) >> fatal);

			throw_exception.resume();

			expect((throw_exception.done() == "done"_b) >> fatal);
			expect((throw_exception.promise().has_exception() == "something wrong!"_b) >> fatal);

			expect((
						[&throw_exception]
						{
							try { [[maybe_unused]] const auto& _ = throw_exception.promise().result(); }
							catch (const std::runtime_error& e)
							{
								expect((e.what() == std::string_view{"exception raise!"}) >> fatal);
								return true;
							}
							return false;
						}() == "throw it!"_b) >>
					fatal);
		};

		"nested_task"_test = []
		{
			constexpr static std::string_view m1{"inner task here~\n"};
			constexpr static std::string_view m2{"outer task waiting...\n"};
			constexpr static std::string_view m3{"outer task finished...\n"};

			std::string message;

			auto outer = [](std::string& outer_message) -> Task<void>
			{
				auto inner = [](std::string& inner_message) -> Task<int>
				{
					inner_message.append(m1);

					co_return 42;
				};

				outer_message.append(m2);
				const auto result = co_await inner(outer_message);
				expect((result == 42_i) >> fatal);
				outer_message.append(m3);
			}(message);

			outer.resume();

			expect((message == std::string{m2}.append(m1).append(m3)) >> fatal);
		};

		"suspend_task"_test = []
		{
			auto suspend_task = []() -> Task<int>
			{
				co_await std::suspend_always{};
				co_await std::suspend_always{};
				co_await std::suspend_never{};
				co_await std::suspend_never{};
				co_await std::suspend_always{};
				co_return 42;
			}();

			expect((suspend_task.done() != "not run yet"_b) >> fatal);

			suspend_task.resume();
			expect((suspend_task.done() != "initial suspend"_b) >> fatal);

			suspend_task.resume();
			expect((suspend_task.done() != "the first suspend_always pass"_b) >> fatal);

			suspend_task.resume();
			expect((suspend_task.done() != "the second suspend_always pass"_b) >> fatal);

			suspend_task.resume();
			expect((suspend_task.done() == "the third suspend_always pass, job done"_b) >> fatal);
			expect((suspend_task.promise().result() == 42_i) >> fatal);
		};

		"coroutine_handle"_test = []
		{
			auto task_1 = []() -> Task<int> { co_return 42; }();

			auto task_2 = []() -> Task<std::string> { co_return "42"; }();

			const auto handle_1 = Task<int>::coroutine_handle::from_promise(task_1.promise());
			const auto handle_2 = Task<std::string>::coroutine_handle::from_promise(task_2.promise());

			handle_1.resume();
			handle_2.resume();

			expect((task_1.done() == "task 1 done"_b) >> fatal);
			expect((task_2.done() == "task 2 done"_b) >> fatal);
			expect((handle_1.done() == "handle 1 done"_b) >> fatal);
			expect((handle_2.done() == "handle 2 done"_b) >> fatal);

			expect((task_1.promise().result() == 42_i) >> fatal);
			expect((task_2.promise().result() == "42") >> fatal);
		};
	};
}// namespace
