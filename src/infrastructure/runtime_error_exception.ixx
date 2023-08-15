// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.infrastructure:runtime_error.exception;

import std;

export namespace gal::prometheus::infrastructure
{
	class RuntimeError : public std::runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	class LogicError : public std::logic_error
	{
	public:
		using logic_error::logic_error;
	};

	class BadCastError final : public LogicError
	{
	public:
		using LogicError::LogicError;
	};

	class InvalidArgumentError final : public LogicError
	{
	public:
		using LogicError::LogicError;
	};

	class OutOfRangeError final : public LogicError
	{
	public:
		using LogicError::LogicError;
	};

	/**
	 * @brief Exception thrown during an operating system call.
	 *
	 * @note This exception is often thrown due to an error with permission or incorrect given parameters.
	 */
	// ReSharper disable once CppInconsistentNaming
	class OSError final : public RuntimeError
	{
	public:
		using RuntimeError::RuntimeError;
	};

	/**
	 * @brief Exception thrown during string parsing on an error.
	 */
	class StringParseError final : public RuntimeError
	{
	public:
		using RuntimeError::RuntimeError;
	};
}// namespace gal::prometheus::infrastructure
