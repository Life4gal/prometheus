// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/texture.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	TextureWriter::TextureWriter(const point_type point, const borrow_data_type& data) noexcept
		: point_{point},
		  data_{data} {}

	auto TextureWriter::valid() const noexcept -> bool
	{
		return point_ != invalid_point;
	}

	auto TextureWriter::position() const noexcept -> point_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		return point_;
	}

	auto TextureWriter::fill(const size_type::value_type y, const size_type::value_type offset, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(offset + n <= width);

		for (size_type::value_type x = offset; x < offset + n; ++x)
		{
			data_[y, x] = element;
		}
	}

	auto TextureWriter::fill(const size_type::value_type y, const size_type::value_type offset, const data_view_type data) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(offset + data.size() <= width);

		for (size_type::value_type x = 0; x < data.size(); ++x)
		{
			data_[y, x + offset] = data[x];
		}
	}

	auto TextureWriter::fill(const size_type::value_type y, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(n <= width);

		fill(y, 0, n, element);
	}

	auto TextureWriter::fill(const size_type::value_type y, const data_view_type data) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() <= width);

		fill(y, 0, data);
	}

	auto TextureWriter::fill(const size_type::value_type y, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);

		fill(y, width, element);
	}

	auto TextureWriter::fill(const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		const auto height = data_.extent(0);
		for (size_type::value_type y = 0; y < height; ++y)
		{
			fill(y, element);
		}
	}

	auto TextureWriter::fill(const data_view_type data) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		for (size_type::value_type y = 0; y < height; ++y)
		{
			const data_view_type sub{data.begin() + static_cast<std::ptrdiff_t>(y) * width, width};

			fill(y, sub);
		}
	}

	auto TextureWriter::operator[](const size_type::value_type x, const size_type::value_type y) const noexcept -> borrow_data_type::reference
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(x < width);

		return data_[y, x];
	}

	TextureViewer::TextureViewer(const point_type point, const borrow_data_type& data) noexcept
		: point_{point},
		  data_{data} {}

	auto TextureViewer::position() const noexcept -> point_type
	{
		return point_;
	}

	auto TextureViewer::size() const noexcept -> size_type
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		return {width, height};
	}

	auto TextureViewer::line(const size_type::value_type y, const size_type::value_type offset, const size_type::value_type n) const noexcept -> data_view_type
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(offset + n <= width);

		// TODO: submdspan(C++ 26)
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data_.stride(1) == 1);
		const auto& this_line = data_[y, 0];

		return {&this_line, n};
	}

	auto TextureViewer::line(const size_type::value_type y, const size_type::value_type n) const noexcept -> data_view_type
	{
		return line(y, 0, n);
	}

	auto TextureViewer::line(const size_type::value_type y) const noexcept -> data_view_type
	{
		const auto width = data_.extent(1);
		// const auto height = data_.extent(0);

		return line(y, width);
	}

	auto TextureViewer::operator[](const size_type::value_type x, const size_type::value_type y) const noexcept -> borrow_data_type::reference
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(x < width);

		return data_[y, x];
	}
}
