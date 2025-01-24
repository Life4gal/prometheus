// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <thread>
#include <mutex>
#include <vector>

#include <prometheus/macro.hpp>

#include <functional/enumeration.hpp>
#include <platform/os.hpp>

#include <unit_test/def.hpp>
#include <unit_test/events.hpp>

namespace gal::prometheus::unit_test::executor
{
	/**
	 * Executor:
	 *  size_t max_worker = std::thread::hardware_concurrency()
	 *  thread workers[max_worker] 
	 *  vector<events::EventSuite> suites
	 *  vector<suite_result_type> suite_results
	 *
	 *  worker(thread:0)       => (results[      0 + 0 * max_worker], results[      0 + 1 * max_worker], ..., results[     0 + (n-1) * max_worker])
	 *  worker(thread:1)       => (results[      1 + 0 * max_worker], results[      1 + 1 * max_worker], ..., results[     1 + (n-1) * max_worker])
	 *  worker(thread:2)       => (results[      2 + 0 * max_worker], results[      2 + 1 * max_worker], ..., results[     2 + (n-1) * max_worker])
	 *  ...
	 *  worker(thread:n-1)   => (results[(n-1) + 0 * max_worker], results[(n-1) + 1 * max_worker], ..., results[(n-1) + (n-1) * max_worker])
	 */

	struct internal_tag {};

	class Worker;

	class Executor
	{
		// report_failure()
		// config_xxx(...)
		friend class Worker;

	public:
		using worker_pool_type = std::vector<std::thread>;
		using suites_type = std::vector<events::EventSuite>;

	private:
		config_type config_;

		worker_pool_type worker_pool_;
		// for print
		constexpr static auto worker_job_done = std::numeric_limits<std::size_t>::max();
		std::vector<std::size_t> worker_job_tracer_;

		suites_type suites_;
		suite_results_type suite_results_;

		// The total number of errors that have been reported,
		// the number of errors that will be reported by each worker at the end of their work
		std::size_t total_fails_;
		// When a worker reports the number of failures, temporarily block all threads to create a new worker,
		// because the error may have reached its limit, and all subsequent suites should be skipped
		std::mutex mutex_reporting_;

		// =========================================
		// ERROR
		// =========================================

		[[nodiscard]] auto is_executor_fatal_error() const noexcept -> bool
		{
			return total_fails_ == std::numeric_limits<std::size_t>::max();
		}

		auto make_executor_fatal_error() noexcept -> void
		{
			total_fails_ = std::numeric_limits<std::size_t>::max();
		}

		// called by worker
		// CONTINUE: false
		// TERMINATE: true
		// todo:
		// There is no easy and safe way to simply terminate all workers when one of them reports an error and reaches a specified threshold.
		// We currently have to wait for each worker to report an error or finish the job.
		auto report_failure() noexcept -> bool
		{
			if (const auto terminate = [this]() noexcept -> bool
				{
					if (is_executor_fatal_error())
					{
						return true;
					}

					std::scoped_lock lock{mutex_reporting_};
					total_fails_ += 1;
					return total_fails_ >= config_n_failures_abort();
				}();
				terminate)
			[[unlikely]]
			{
				make_executor_fatal_error();

				return true;
			}

			return false;
		}

		// =========================================
		// CONFIG
		// =========================================

		[[nodiscard]] auto config() const noexcept -> const config_type&
		{
			return config_;
		}

		[[nodiscard]] auto config_output_color() const noexcept -> const config_type::color_type&
		{
			return config().color;
		}

		[[nodiscard]] auto config_get_ident_size(const std::size_t nested_level) const noexcept -> std::size_t
		{
			return nested_level * config().tab_width;
		}

		[[nodiscard]] auto config_output_prefix() const noexcept -> std::string_view
		{
			return config().prefix;
		}

		auto config_out() noexcept -> void
		{
			config().out(std::move(suite_results_));
		}

		template<config_type::ReportLevel RequiredLevel>
		[[nodiscard]] auto config_check_report_level() const noexcept -> bool
		{
			return (RequiredLevel & config().report_level) == RequiredLevel;
		}

		[[nodiscard]] auto config_dry_run() const noexcept -> bool
		{
			return config().dry_run;
		}

		template<config_type::BreakPointLevel RequiredLevel>
		[[nodiscard]] auto config_check_break_point() const noexcept -> bool
		{
			return static_cast<bool>(RequiredLevel & config().break_point_level);
		}

		[[nodiscard]] auto config_n_failures_abort() const noexcept -> std::size_t
		{
			return config().abort_after_n_failures;
		}

		[[nodiscard]] auto config_check_suite_execute(const suite_node_type& node) const noexcept -> bool
		{
			return config().filter_suite(node);
		}

		[[nodiscard]] auto config_check_test_execute(const test_node_type& node) const noexcept -> bool
		{
			return config().filter_test(node);
		}

		// =========================================
		// SUITE
		// =========================================

		// Create a new worker in the current thread, execute the given suite, and write the result to suite_result
		[[nodiscard]] auto worker_work(const events::EventSuite& suite, suite_result_type& suite_result) noexcept -> bool;

		// Each thread processes the suite in the corresponding bucket
		auto worker_thread_func(std::size_t thread_index, std::size_t step) noexcept -> void;

		// =========================================
		// SUMMARY
		// =========================================

		auto on(const events::EventSummary&) noexcept -> void
		{
			struct total_result
			{
				std::size_t test_passed;
				std::size_t test_failed;
				std::size_t test_skipped;

				std::size_t assertion_passed;
				std::size_t assertion_failed;

				constexpr auto operator+(const total_result& other) const noexcept -> total_result
				{
					total_result new_one{*this};

					new_one.test_passed += other.test_passed;
					new_one.test_failed += other.test_failed;
					new_one.test_skipped += other.test_skipped;

					new_one.assertion_passed += other.assertion_passed;
					new_one.assertion_failed += other.assertion_failed;

					return new_one;
				}
			};

			const auto get_result_of_test = [](this const auto& self, const test_result_type& test_result) noexcept -> total_result
			{
				const auto passed = static_cast<std::size_t>(
					+(
						test_result.status == test_result_type::Status::PASSED
					)
				);
				const auto failed = static_cast<std::size_t>(
					+(
						test_result.status == test_result_type::Status::FAILED or
						test_result.status == test_result_type::Status::INTERRUPTED or
						test_result.status == test_result_type::Status::TERMINATED
					)
				);
				const auto skipped = static_cast<std::size_t>(
					+(
						test_result.status == test_result_type::Status::SKIPPED_NO_ASSERTION or
						test_result.status == test_result_type::Status::SKIPPED_FILTERED
					)
				);

				return std::ranges::fold_left(
					test_result.children,
					total_result
					{
							.test_passed = passed,
							.test_failed = failed,
							.test_skipped = skipped,
							.assertion_passed = test_result.total_assertions_passed,
							.assertion_failed = test_result.total_assertions_failed
					},
					[self](const total_result& total, const test_result_type& nested_test_result) noexcept -> total_result
					{
						return total + self(nested_test_result);
					}
				);
			};

			constexpr auto get_result_of_suite = [get_result_of_test](const suite_result_type& suite_result) noexcept -> total_result
			{
				return std::ranges::fold_left(
					suite_result.results,
					total_result
					{
							.test_passed = 0,
							.test_failed = 0,
							.test_skipped = 0,
							.assertion_passed = 0,
							.assertion_failed = 0
					},
					[get_result_of_test](const total_result& total, const test_result_type& test_result) noexcept -> total_result
					{
						return total + get_result_of_test(test_result);
					}
				);
			};

			std::ranges::for_each(
				suite_results_,
				[this, get_result_of_suite](suite_result_type& suite_result) noexcept -> void
				{
					const auto color = config_output_color();

					// ReSharper disable once CppUseStructuredBinding
					if (const auto result = get_result_of_suite(suite_result);
						result.assertion_failed == 0)
					[[likely]]
					{
						if (result.assertion_passed == 0)
						{
							// skip empty suite report
						}
						else
						{
							std::format_to(
								std::back_inserter(suite_result.report_string),
								"\n==========================================\n"
								"Suite {}{}{} -> {}all tests passed{}({} assertions in {} tests), {} tests skipped."
								"\n==========================================\n",
								color.suite,
								suite_result.name,
								color.none,
								color.pass,
								color.none,
								result.assertion_passed,
								result.test_passed,
								result.test_skipped
							);
						}
					}
					else
					[[unlikely]]
					{
						std::format_to(
							std::back_inserter(suite_result.report_string),
							"\n==========================================\n"
							"Suite {}{}{}\n"
							"tests {} | {} {}passed({:.6g}%){} | {} {}failed({:.6g}%){} | {} {}skipped({:.6g}%){}\n"
							"assertions {} | {} {}passed({:.6g}%){} | {} {}failed({:.6g}%){}"
							"\n==========================================\n",
							color.suite,
							suite_result.name,
							color.none,
							// test
							result.test_passed + result.test_failed + result.test_skipped,
							// passed
							result.test_passed,
							color.pass,
							static_cast<double>(result.test_passed) /
							static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
							* 100.0,
							color.none,
							// failed
							result.test_failed,
							color.failure,
							static_cast<double>(result.test_failed) /
							static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
							* 100.0,
							color.none,
							// skipped
							result.test_skipped,
							color.skip,
							static_cast<double>(result.test_skipped) /
							static_cast<double>(result.test_passed + result.test_failed + result.test_skipped)
							* 100.0,
							color.none,
							// assertion
							result.assertion_passed + result.assertion_failed,
							// passed
							result.assertion_passed,
							color.pass,
							static_cast<double>(result.assertion_passed) /
							static_cast<double>(result.assertion_passed + result.assertion_failed)
							* 100.0,
							color.none,
							// failed
							result.assertion_failed,
							color.failure,
							static_cast<double>(result.assertion_failed) /
							static_cast<double>(result.assertion_passed + result.assertion_failed)
							* 100.0,
							color.none
						);
					}
				}
			);

			config_out();
		}

		explicit Executor() noexcept
			:
			total_fails_{0} {}

	public:
		Executor(const Executor&) noexcept = delete;
		Executor(Executor&&) noexcept = delete;
		auto operator=(const Executor&) noexcept -> Executor& = delete;
		auto operator=(Executor&&) noexcept -> Executor& = delete;

		~Executor() noexcept
		{
			if (not config_.dry_run)
			{
				// filter suite
				const auto [removed_begin, removed_end] = std::ranges::remove_if(
					suites_,
					[this](const auto& suite) noexcept -> bool
					{
						return not this->config_check_suite_execute(suite_node_type{.name = suite.name});
					}
				);
				suites_.erase(removed_begin, removed_end);

				// preallocate all suite result, avoid synchronise
				suite_results_.resize(suites_.size());

				// create thread
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(worker_pool_.empty());
				// - 1 => one thread for print
				const auto max_worker = static_cast<std::size_t>(std::thread::hardware_concurrency() - 1);
				const auto real_workers = std::ranges::min(max_worker, suites_.size());
				worker_pool_.reserve(real_workers);
				worker_job_tracer_.resize(real_workers, 0);
				for (const auto index: std::views::iota(static_cast<std::size_t>(0), real_workers))
				{
					worker_pool_.emplace_back(&Executor::worker_thread_func, this, index, real_workers);
				}

				#if defined(GAL_PROMETHEUS_PLATFORM_WINDOWS)
				#define SYSTEM_CLEAR_CONSOLE std::system("cls"); // NOLINT(concurrency-mt-unsafe)
				#else
				#define SYSTEM_CLEAR_CONSOLE std::system("clear"); // NOLINT(concurrency-mt-unsafe)
				#endif

				// join
				std::atomic job_done = false;
				std::thread{
						[this, &job_done]() noexcept -> void
						{
							struct info_type
							{
								std::size_t thread_index;

								std::size_t result_size;
								std::string message;
							};

							std::vector<info_type> current_infos{};
							current_infos.resize(
								worker_job_tracer_.size(),
								{
										.thread_index = std::numeric_limits<std::size_t>::max(),
										.result_size = std::numeric_limits<std::size_t>::max(),
										.message = ""
								}
							);

							while (not job_done)
							{
								// @see Executor::worker_thread_func
								for (const auto [i, index]: std::views::enumerate(worker_job_tracer_))
								{
									const auto& color = config_output_color();

									if (auto& [thread_index, result_size, message] = current_infos[i];
										index == worker_job_done)
									{
										message = std::format(
											"{}WORKER[{:>2}]{}: {}job done!{}",
											color.fatal,
											i,
											color.none,
											color.pass,
											color.none
										);
									}
									else
									{
										if (const auto& suite_result = suite_results_[index];
											thread_index != index or result_size != suite_result.results.size())
										{
											if (suite_result.results.empty())
											{
												message = std::format(
													"{}WORKER[{:>2}]{}: {}pending...{}",
													color.fatal,
													i,
													color.none,
													color.skip,
													color.none
												);
											}
											else
											{
												thread_index = index;
												result_size = suite_result.results.size();
												message = std::format(
													"{}WORKER[{:>2}]{}: running test {}[{}] {}{}",
													color.fatal,
													i,
													color.none,
													color.test,
													suite_result.name,
													suite_result.results[result_size - 1].name,
													color.none
												);
											}
										}
										else
										{
											message.push_back('.');
										}
									}
								}

								// const auto total_length = std::ranges::fold_left(
								// 	current_infos,
								// 	// '\n'
								// 	current_infos.size(),
								// 	[](const std::size_t total, const info_type& info) noexcept -> std::size_t
								// 	{
								// 		return total + info.message.size();
								// 	}
								// );
								//
								// std::string output{};
								// output.reserve(total_length);
								// std::ranges::for_each(
								// 	current_infos,
								// 	[&output](const info_type& info) noexcept -> void
								// 	{
								// 		output.append(info.message);
								// 		output.push_back('\n');
								// 	}
								// );
								// std::print(stdout, "{}", output);

								std::ranges::for_each(
									current_infos,
									[](const info_type& info) noexcept -> void
									{
										std::println(stdout, "{}", info.message);
									}
								);

								using namespace std::chrono_literals;
								std::this_thread::sleep_for(1s);

								SYSTEM_CLEAR_CONSOLE
							}
						}
				}.detach();
				std::ranges::for_each(
					worker_pool_,
					[](auto& thread) noexcept -> void
					{
						GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(thread.joinable());
						thread.join();
					}
				);

				// ALL THREAD FINISHED
				job_done = true;
				SYSTEM_CLEAR_CONSOLE

				on(events::EventSummary{});
			}
		}

		// =========================================
		// INSTANCE
		// =========================================

		[[nodiscard]] static auto instance() noexcept -> Executor&
		{
			static Executor executor{};
			return executor;
		}

		// =========================================
		// CONFIG
		// =========================================

		auto set_config(config_type&& config) noexcept -> void
		{
			config_ = std::move(config);
		}

		// =========================================
		// SUITE
		// =========================================

		auto on(const events::EventSuite& suite) noexcept -> void
		{
			suites_.push_back(suite);
		}
	};

	class Worker
	{
		// run(suite, suite_result)
		friend class Executor;

		// on(const events::EventAssertionFatal&)
		class EndThisTest final {};

		// check_total_failures()
		class EndThisSuite final {};

	public:
		struct test_data_type
		{
			test_name_view_type name;
			test_categories_view_type categories;
		};

		using test_data_stack_type = std::vector<test_data_type>;

		// nullable
		using test_results_iterator_type = test_results_type::pointer;

	private:
		constexpr static auto worker_off_work = reinterpret_cast<test_results_iterator_type>(0xbad'c0ffee);

		suite_result_type* suite_;

		test_data_stack_type test_data_stack_;

		test_results_iterator_type current_test_result_;

		// =========================================

		[[nodiscard]] auto executor() const noexcept -> Executor&
		{
			std::ignore = this;
			return Executor::instance();
		}

		[[nodiscard]] auto suite() noexcept -> suite_result_type&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(suite_ != nullptr);
			return *suite_;
		}

		[[nodiscard]] auto suite() const noexcept -> const suite_result_type&
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(suite_ != nullptr);
			return *suite_;
		}

		// =========================================

		enum class IdentType : std::uint8_t
		{
			TEST,
			ASSERTION,
		};

		template<IdentType Type>
		[[nodiscard]] auto nested_level_of_current_test() const noexcept -> std::size_t
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not suite().results.empty());

			if constexpr (Type == IdentType::ASSERTION)
			{
				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
			}
			else
			{
				// top level
				if (current_test_result_ == nullptr)
				{
					return 1;
				}
			}

			std::size_t result = 0;
			for (const auto* p = current_test_result_; p != nullptr; p = p->parent)
			{
				result += 1;
			}

			return result + (Type == IdentType::ASSERTION);
		}

		template<IdentType Type>
		[[nodiscard]] auto ident_size_of_current_test() const noexcept -> std::size_t
		{
			return executor().config_get_ident_size(nested_level_of_current_test<Type>());
		}

		// [suite_name] test1.test2.test3
		[[nodiscard]] auto fullname_of_current_test() const noexcept -> std::string
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not suite().results.empty());

			auto result = std::format("[{}] ", suite().name);

			const auto* p = std::addressof(suite().results.back());
			while (p != nullptr)
			{
				result.append(p->name);
				result.push_back('.');

				p = p->children.empty() ? nullptr : std::addressof(p->children.back());
			}

			result.pop_back();
			return result;
		}

		// =========================================

		// run(const events::EventSuite&) => OK
		// check_total_failures => ERROR
		auto off_work() noexcept -> void
		{
			current_test_result_ = worker_off_work;
		}

		[[nodiscard]] auto on_working() const noexcept -> bool
		{
			return current_test_result_ != worker_off_work;
		}

		[[nodiscard]] auto off_working() const noexcept -> bool
		{
			return current_test_result_ == worker_off_work;
		}

		auto check_total_failures() noexcept(false) -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

			if (executor().report_failure())
			{
				current_test_result_->status = test_result_type::Status::TERMINATED;

				const auto n_failures_abort = executor().config_n_failures_abort();
				const auto prefix = executor().config_output_prefix();
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(suite().report_string),
					"{:<{}}{}The number of errors has reached the specified threshold {} "
					"(this test raises {} error(s)), "
					"terminate all suite/test!{}\n",
					prefix,
					ident_size_of_current_test<IdentType::ASSERTION>(),
					color.failure,
					n_failures_abort,
					current_test_result_->total_assertions_failed,
					color.none
				);

				throw EndThisSuite{}; // NOLINT(hicpp-exception-baseclass)
			}
		}

		// =========================================
		// SUITE
		// =========================================

		auto run(const events::EventSuite& suite, suite_result_type& suite_result) noexcept -> void
		{
			// ==================================
			// bind result
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ == nullptr);
			suite_ = std::addressof(suite_result);

			// ==================================
			// begin
			this->suite().name = suite.name;

			if (executor().config_check_report_level<config_type::ReportLevel::SUITE_NAME>())
			{
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(this->suite().report_string),
					"Executing suite {}{}{} vvv\n",
					color.suite,
					this->suite().name,
					color.none
				);
			}

			// ==================================
			// run
			try
			{
				std::invoke(suite);
			}
			catch (const EndThisSuite&) // NOLINT(bugprone-empty-catch)
			{
				// This exception is thrown by the function `check_total_failures()` and is intercepted here and no longer propagated
				// do nothing here :)
			}
			// The user's suite threw an exception, but it was not handled.
			// We capture the exception here to avoid early termination of the program.
			// suite<"throw"> _ = []
			// {
			//	throw std::runtime_error{"throw!"};
			//
			// "unreachable"_test = [] { };
			// };
			catch (const std::exception& exception)
			{
				on(events::EventUnexpected{.message = exception.what()});
			}
			catch (...)
			{
				on(events::EventUnexpected{.message = "unknown exception type, not derived from std::exception"});
			}

			// ==================================
			// end/error
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ == nullptr or off_working());

			if (executor().config_check_report_level<config_type::ReportLevel::SUITE_NAME>())
			{
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(this->suite().report_string),
					"^^^ End of suite {}{}{} execution\n",
					color.suite,
					this->suite().name,
					color.none
				);
			}

			off_work();
		}

		// =========================================
		// TEST
		// =========================================

		template<typename InvocableType, typename Arg>
		auto on(const events::EventTest<InvocableType, Arg>& test, internal_tag) noexcept(false) -> void
		{
			if (not on_working())
			{
				return;
			}

			// build chain
			test_node_type node{.parent = nullptr, .name = test.name, .categories = test.categories};
			{
				auto* parent = std::addressof(node.parent);
				std::ranges::for_each(
					test_data_stack_ | std::views::reverse,
					[&parent](const test_data_type& data) noexcept -> void
					{
						*parent = std::make_unique<test_node_type>(nullptr, data.name, data.categories);
						parent = std::addressof((*parent)->parent);
					}
				);
			}

			if (executor().config_check_test_execute(node))
			{
				test_data_stack_.emplace_back(test.name, test.categories);
				this->on(test.begin());

				try
				{
					std::invoke(test);
				}
				catch (const EndThisTest&) // NOLINT(bugprone-empty-catch)
				{
					// This exception is thrown by and only by the function `on(events::EventAssertionFatal)` and is intercepted here and no longer propagated
					// do nothing here :)
				}
				catch (const EndThisSuite&) // NOLINT(bugprone-empty-catch)
				{
					// end this test
					this->on(test.end());
					off_work();
					// propagate exception
					throw;
				}
				// The user's suite threw an exception, but it was not handled.
				// We capture the exception here to avoid early termination of the program.
				catch (const std::exception& exception)
				{
					on(events::EventUnexpected{.message = exception.what()});
				}
				catch (...)
				{
					on(events::EventUnexpected{.message = "unhandled exception, not derived from std::exception"});
				}

				this->on(test.end());
				test_data_stack_.pop_back();
			}
			else
			{
				this->on(test.skip());
			}
		}

		auto on(const events::EventTestBegin& test_begin) noexcept -> void
		{
			// we chose to construct a temporary object here to avoid possible errors, and trust that the optimizer will forgive us ;)
			auto t = test_result_type
			{
					.name = std::string{test_begin.name},
					.parent = current_test_result_,
					.children = {},
					.total_assertions_passed = 0,
					.total_assertions_failed = 0,
					.time = {},
					.status = test_result_type::Status::PENDING
			};

			if (current_test_result_)
			{
				// nested
				auto& this_test = current_test_result_->children.emplace_back(std::move(t));
				current_test_result_ = std::addressof(this_test);

				if (executor().config_check_report_level<config_type::ReportLevel::TEST_NAME>())
				{
					const auto prefix = executor().config_output_prefix();
					const auto& color = executor().config_output_color();

					std::format_to(
						std::back_inserter(suite().report_string),
						"{:<{}}Running nested test {}{}{}...\n",
						prefix,
						ident_size_of_current_test<IdentType::TEST>(),
						color.test,
						fullname_of_current_test(),
						color.none
					);
				}
			}
			else
			{
				// top level
				auto& this_test = suite().results.emplace_back(std::move(t));
				current_test_result_ = std::addressof(this_test);

				if (executor().config_check_report_level<config_type::ReportLevel::TEST_NAME>())
				{
					const auto prefix = executor().config_output_prefix();
					const auto& color = executor().config_output_color();

					std::format_to(
						std::back_inserter(suite().report_string),
						"{:<{}}Running test {}{}{}...\n",
						prefix,
						ident_size_of_current_test<IdentType::TEST>(),
						color.test,
						fullname_of_current_test(),
						color.none
					);
				}
			}
		}

		auto on(const events::EventTestSkip& test_skip) noexcept -> void
		{
			on(events::EventTestBegin{.name = test_skip.name});
			current_test_result_->status = test_result_type::Status::SKIPPED_FILTERED;
			on(events::EventTestEnd{.name = test_skip.name});
		}

		auto on(const events::EventTestEnd& test_end) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_->name == test_end.name);

			const auto time_count = current_test_result_->time.count();
			if (current_test_result_->status == test_result_type::Status::PENDING)
			[[likely]]
			{
				// the current test is considered SKIPPED only if it does not have any assertions and has no children.
				if (current_test_result_->total_assertions_failed == 0 and current_test_result_->total_assertions_passed == 0)
				{
					if (current_test_result_->children.empty())
					{
						current_test_result_->status = test_result_type::Status::SKIPPED_NO_ASSERTION;
					}
					else
					{
						current_test_result_->status =
								std::ranges::all_of(
									current_test_result_->children,
									[](const auto& child_test) noexcept
									{
										return child_test.total_assertions_failed == 0;
									}
								)
									? test_result_type::Status::PASSED
									: test_result_type::Status::FAILED;
					}
				}
				else
				{
					current_test_result_->status = current_test_result_->total_assertions_failed == 0
						                               ? test_result_type::Status::PASSED
						                               : test_result_type::Status::FAILED;
				}
			}

			if (executor().config_check_report_level<config_type::ReportLevel::TEST_NAME>())
			{
				const auto prefix = executor().config_output_prefix();
				const auto& color = executor().config_output_color();

				if (const auto status = current_test_result_->status;
					status == test_result_type::Status::PASSED or status == test_result_type::Status::FAILED)
				[[likely]]
				{
					std::format_to(
						std::back_inserter(suite().report_string),
						"{:<{}}{}{}{} after {} milliseconds.\n",
						prefix,
						ident_size_of_current_test<IdentType::TEST>(),
						status == test_result_type::Status::PASSED ? color.pass : color.failure,
						status == test_result_type::Status::PASSED ? "PASSED" : "FAILED",
						color.none,
						time_count
					);
				}
				else if (status == test_result_type::Status::SKIPPED_NO_ASSERTION or status == test_result_type::Status::SKIPPED_FILTERED)
				[[unlikely]]
				{
					std::format_to(
						std::back_inserter(suite().report_string),
						"{:<{}}{}SKIPPED{} --- [{}] \n",
						prefix,
						ident_size_of_current_test<IdentType::TEST>(),
						color.skip,
						color.none,
						status == test_result_type::Status::SKIPPED_NO_ASSERTION
							? "No Assertion(s) Found"
							: "FILTERED"
					);
				}
				else if (status == test_result_type::Status::INTERRUPTED or status == test_result_type::Status::TERMINATED)
				[[unlikely]]
				{
					std::format_to(
						std::back_inserter(suite().report_string),
						"{:<{}}{}{}{}\n",
						prefix,
						ident_size_of_current_test<IdentType::TEST>(),
						color.fatal,
						status == test_result_type::Status::INTERRUPTED
							? "INTERRUPTED"
							: "TERMINATED",
						color.none
					);
				}
				else { std::unreachable(); }
			}

			// reset to parent test
			current_test_result_ = current_test_result_->parent;
		}

		// =========================================
		// ASSERTION
		// =========================================

		template<expression_t Expression>
		auto on(const events::EventAssertion<Expression>& assertion, internal_tag) noexcept(false) -> bool
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

			if (static_cast<bool>(assertion.expression))
			[[likely]]
			{
				this->on(assertion.pass());
				return true;
			}

			this->on(assertion.fail());
			return false;
		}

		template<expression_t Expression>
		auto on(const events::EventAssertionPass<Expression>& assertion_pass) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

			if (executor().config_check_report_level<config_type::ReportLevel::ASSERTION_PASS>())
			{
				// @see: Operand::prefer_no_type_name
				constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

				const auto prefix = executor().config_output_prefix();
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(suite().report_string),
					"{:<{}}[{}:{}] {}[{}]{} - {}PASSED{} \n",
					prefix,
					ident_size_of_current_test<IdentType::ASSERTION>(),
					assertion_pass.location.file_name(),
					assertion_pass.location.line(),
					color.expression,
					meta::to_string<std::string, not prefer_no_type_name>(assertion_pass.expression),
					color.none,
					color.pass,
					color.none
				);
			}

			current_test_result_->total_assertions_passed += 1;
		}

		template<expression_t Expression>
		auto on(const events::EventAssertionFail<Expression>& assertion_fail) noexcept(false) -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

			GAL_PROMETHEUS_ERROR_BREAKPOINT_IF(executor().config_check_break_point<config_type::BreakPointLevel::FAILURE>(), "EventAssertionFail");

			if (executor().config_check_report_level<config_type::ReportLevel::ASSERTION_FAILURE>())
			{
				// @see: Operand::prefer_no_type_name
				constexpr auto prefer_no_type_name = requires { typename Expression::prefer_no_type_name; };

				const auto prefix = executor().config_output_prefix();
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(suite().report_string),
					"{:<{}}[{}:{}] {}[{}]{} - {}FAILED{} \n",
					prefix,
					ident_size_of_current_test<IdentType::ASSERTION>(),
					assertion_fail.location.file_name(),
					assertion_fail.location.line(),
					color.expression,
					meta::to_string<std::string, not prefer_no_type_name>(assertion_fail.expression),
					color.none,
					color.failure,
					color.none
				);
			}

			current_test_result_->total_assertions_failed += 1;

			check_total_failures();
		}

		auto on(const events::EventAssertionFatal& assertion_fatal, internal_tag) noexcept(false) -> void
		{
			GAL_PROMETHEUS_ERROR_BREAKPOINT_IF(executor().config_check_break_point<config_type::BreakPointLevel::FATAL>(), "EventAssertionFatal");

			if (executor().config_check_report_level<config_type::ReportLevel::ASSERTION_FATAL>())
			{
				const auto prefix = executor().config_output_prefix();
				const auto& color = executor().config_output_color();

				std::format_to(
					std::back_inserter(suite().report_string),
					"{:<{}}^^^ {}FATAL ERROR! END TEST!{}\n",
					prefix,
					ident_size_of_current_test<IdentType::ASSERTION>() +
					(
						// '['
						1 +
						// file_name
						std::string_view::traits_type::length(assertion_fatal.location.file_name())) +
					// ':'
					1 +
					// line
					[]<typename Line>(Line line)
					{
						Line result = 0;
						while (line)
						{
							result += 1;
							line /= 10;
						}
						return result;
					}(assertion_fatal.location.line()) +
					// "] ["
					3,
					color.fatal,
					color.none
				);
			}

			check_total_failures();

			throw EndThisTest{}; // NOLINT(hicpp-exception-baseclass)
		}

		// =========================================
		// UNEXPECTED
		// =========================================

		auto on(const events::EventUnexpected& unexpected) noexcept -> void
		{
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(current_test_result_ != nullptr);

			const auto& color = executor().config_output_color();

			std::format_to(
				std::back_inserter(suite().name),
				"Unhandled exception threw from {}: {}{}{}\n",
				fullname_of_current_test(),
				color.failure,
				unexpected.what(),
				color.none
			);
		}

		// =========================================
		// LOG
		// =========================================

		template<typename MessageType>
		auto on(const events::EventLog<MessageType>& log, internal_tag) noexcept -> void
		{
			if (log.message != "\n")
			[[likely]]
			{
				// pop '\n'
				suite().report_string.pop_back();
			}

			const auto& color = executor().config_output_color();

			suite().report_string.append(color.message);
			#if __cpp_lib_containers_ranges >= 202202L
			suite().report_string.append_range(log.message);
			#else
			suite().report_string.insert(suite().report_string.end(), std::ranges::begin(log.message), std::ranges::end(log.message));
			#endif
			suite().report_string.append(color.none);

			// push '\n'
			suite().report_string.push_back('\n');
		}

		Worker() noexcept
			:
			suite_{nullptr},
			current_test_result_{nullptr} {}

	public:
		[[nodiscard]] static auto instance() noexcept -> Worker&
		{
			// one worker per thread
			thread_local Worker worker{};
			return worker;
		}

		// =========================================
		// TEST
		// =========================================

		template<typename InvocableType, typename Arg>
		auto on(const events::EventTest<InvocableType, Arg>& test) noexcept(false) -> void
		{
			this->on(test, internal_tag{});
		}

		// =========================================
		// ASSERTION
		// =========================================

		template<expression_t Expression>
		auto on(const events::EventAssertion<Expression>& assertion) noexcept(false) -> bool
		{
			return this->on(assertion, internal_tag{});
		}

		auto on(const events::EventAssertionFatal& assertion_fatal) noexcept(false) -> void
		{
			on(assertion_fatal, internal_tag{});
		}

		// =========================================
		// LOG
		// =========================================

		template<typename MessageType>
		auto on(const events::EventLog<MessageType>& log) noexcept -> void
		{
			this->on(log, internal_tag{});
		}
	};

	inline auto Executor::worker_work(const events::EventSuite& suite, suite_result_type& suite_result) noexcept -> bool
	{
		const auto error = [this]() noexcept -> bool
		{
			std::scoped_lock lock{mutex_reporting_};
			return is_executor_fatal_error();
		}();
		if (not error)
		{
			auto& worker = Worker::instance();
			worker.run(suite, suite_result);
			return true;
		}

		return false;
	}

	inline auto Executor::worker_thread_func(const std::size_t thread_index, const std::size_t step) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(suites_.size() == suite_results_.size());

		for (const auto bucket_count = suites_.size() / step;
		     const auto index_in_bucket: std::views::iota(static_cast<std::size_t>(0), bucket_count))
		{
			const auto index = thread_index + index_in_bucket * step;
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < suites_.size());

			// for print
			worker_job_tracer_[thread_index] = index;

			const auto& suite = suites_[index];
			auto& suite_result = suite_results_[index];

			if (const auto result = worker_work(suite, suite_result);
				not result)
			{
				break;
			}
		}

		// for print
		worker_job_tracer_[thread_index] = worker_job_done;
	}
}
