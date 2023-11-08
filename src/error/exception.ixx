// This file is part of prometheus
// Copyright (C) 2022-2023 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

export module gal.prometheus.error:exception;

import std;

export namespace gal::prometheus::error
{
	class Exception : public std::exception
	{
	public:
		using exception::exception;

		explicit Exception(const std::string& message)
			: Exception{message.c_str()} {}
	};

	// =========================
	// RuntimeError
	// =========================

	class RuntimeError : public Exception
	{
	public:
		using Exception::Exception;
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
	class StringParseError : public RuntimeError
	{
	public:
		using RuntimeError::RuntimeError;
	};

	// =========================
	// LogicError
	// =========================

	class LogicError : public Exception
	{
	public:
		using Exception::Exception;
	};

	class InvalidArgumentError : public LogicError
	{
	public:
		using LogicError::LogicError;
	};

	class OutOfRangeError : public LogicError
	{
	public:
		using LogicError::LogicError;
	};

	// =========================
	// BadCastError
	// =========================

	class BadCastError final : public Exception
	{
	public:
		using Exception::Exception;
	};
}// namespace gal::prometheus::error
