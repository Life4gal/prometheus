// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/texture.hpp>

// #define STB_RECT_PACK_IMPLEMENTATION
// #include <stb_rect_pack.h>

namespace gal::prometheus::gfx_new
{
	Texture::Texture(const size_type size) noexcept
		: rp_context_{},
		  texture_{
				  .data = std::make_unique_for_overwrite<element_type[]>(static_cast<std::size_t>(size.width) * size.height),
				  .size = size,
				  .dirty = false,
				  .id = invalid_texture_id,
		  },
		  uv_{1.f / static_cast<uv_type::value_type>(size.width), 1.f / static_cast<uv_type::value_type>(size.height)}
	{
		rp_nodes_.resize(size.width);

		stbrp_init_target(
			&rp_context_,
			static_cast<stbrp_coord>(size.width),
			static_cast<stbrp_coord>(size.height),
			rp_nodes_.data(),
			static_cast<int>(rp_nodes_.size())
		);
	}

	auto Texture::data() const noexcept -> data_view_type
	{
		return {texture_.data.get(), area_size()};
	}

	auto Texture::area_size() const noexcept -> std::size_t
	{
		return static_cast<std::size_t>(texture_.size.width) * texture_.size.height;
	}

	auto Texture::size() const noexcept -> size_type
	{
		return texture_.size;
	}

	auto Texture::uv() const noexcept -> uv_type
	{
		return uv_;
	}

	auto Texture::dirty() const noexcept -> bool
	{
		return texture_.dirty;
	}

	auto Texture::uploaded() const noexcept -> bool
	{
		return texture_.id != invalid_texture_id;
	}

	auto Texture::id() const noexcept -> texture_id_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(uploaded());

		return texture_.id;
	}

	auto Texture::select(const size_type size) noexcept -> BorrowedTexture
	{
		stbrp_rect rect{.id = -1, .w = static_cast<stbrp_coord>(size.width), .h = static_cast<stbrp_coord>(size.height), .x = 0, .y = 0, .was_packed = 0};

		if (stbrp_pack_rects(&rp_context_, &rect, 1))
		{
			const point_type point{static_cast<point_type::value_type>(rect.x), static_cast<point_type::value_type>(rect.y)};

			auto* address = texture_.data.get() + (point.y * size.width + point.x);
			const auto mapping = BorrowedTexture::borrow_data_type::mapping_type{
					std::dextents<size_type::value_type, 2>{size.height, size.width},
					std::array<size_type::value_type, 2>{texture_.size.width, 1},
			};
			const auto data = BorrowedTexture::borrow_data_type{address, mapping};

			texture_.dirty = true;
			return {point, data};
		}

		return {BorrowedTexture::invalid_point, {}};
	}

	BorrowedTexture::BorrowedTexture(const point_type point, const borrow_data_type& data) noexcept
		: point_{point},
		  data_{data} {}

	auto BorrowedTexture::valid() const noexcept -> bool
	{
		return point_ != invalid_point;
	}

	auto BorrowedTexture::position() const noexcept -> point_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		return point_;
	}

	auto BorrowedTexture::fill(const size_type::value_type y, const size_type::value_type offset, const size_type::value_type n, const element_type element) const noexcept -> void
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

	auto BorrowedTexture::fill(const size_type::value_type y, const size_type::value_type offset, const data_view_type data) const noexcept -> void
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

	auto BorrowedTexture::fill(const size_type::value_type y, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(n <= width);

		fill(y, 0, n, element);
	}

	auto BorrowedTexture::fill(const size_type::value_type y, const data_view_type data) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() <= width);

		fill(y, 0, data);
	}

	auto BorrowedTexture::fill(const size_type::value_type y, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);

		fill(y, width, element);
	}

	auto BorrowedTexture::fill(const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		const auto height = data_.extent(0);
		for (size_type::value_type y = 0; y < height; ++y)
		{
			fill(y, element);
		}
	}

	auto BorrowedTexture::fill(const data_view_type data) const noexcept -> void
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

	auto BorrowedTexture::operator[](const size_type::value_type x, const size_type::value_type y) const noexcept -> borrow_data_type::reference
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(x < width);

		return data_[y, x];
	}
}
