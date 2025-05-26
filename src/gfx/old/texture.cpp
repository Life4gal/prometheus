// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/texture.hpp>

#include <gfx/renderer.hpp>

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace
{
	// #define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
} // namespace

namespace gal::prometheus::gfx
{
	struct Texture::pack_context_type
	{
		stbrp_context context{};
		std::vector<stbrp_node> nodes{};
	};

	Texture::Texture(Texture&&) noexcept = default;
	auto Texture::operator=(Texture&&) noexcept -> Texture& = default;

	Texture::~Texture() noexcept = default;

	Texture::Texture(const size_type size) noexcept
		: pack_context_{memory::make_unique<pack_context_type>()},
		  data_{std::make_unique_for_overwrite<std::uint32_t[]>(static_cast<std::size_t>(size.width) * size.height)},
		  size_{size},
		  dirty_{false},
		  id_{invalid_texture_id}
	{
		pack_context_->nodes.resize(size.width);

		stbrp_init_target(
			&pack_context_->context,
			static_cast<stbrp_coord>(size.width),
			static_cast<stbrp_coord>(size.height),
			pack_context_->nodes.data(),
			static_cast<int>(pack_context_->nodes.size())
		);
	}

	auto Texture::create(Renderer& renderer) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not uploaded());

		const auto id = renderer.create_texture(data(), size_);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id != invalid_texture_id);

		dirty_ = false;
		id_ = id;
	}

	auto Texture::upload(Renderer& renderer) noexcept -> void
	{
		renderer.update_texture(*this);
		dirty_ = false;
	}

	auto Texture::upload_if_required(Renderer& renderer) noexcept -> void
	{
		if (dirty_)
		{
			renderer.update_texture(*this);
			dirty_ = false;
		}
	}

	auto Texture::data() const noexcept -> data_view_type
	{
		return {data_.get(), area_size()};
	}

	auto Texture::area_size() const noexcept -> std::size_t
	{
		return static_cast<std::size_t>(size_.width) * size_.height;
	}

	auto Texture::size() const noexcept -> size_type
	{
		return size_;
	}

	auto Texture::uv() const noexcept -> uv_type
	{
		const auto s = size();

		return {1.f / static_cast<uv_type::value_type>(s.width), 1.f / static_cast<uv_type::value_type>(s.height)};
	}

	auto Texture::dirty() const noexcept -> bool
	{
		return dirty_;
	}

	auto Texture::uploaded() const noexcept -> bool
	{
		return id_ != invalid_texture_id;
	}

	auto Texture::id() const noexcept -> texture_id_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(uploaded());

		return id_;
	}

	auto Texture::select(const size_type size) noexcept -> BorrowTexture
	{
		stbrp_rect rect{.id = -1, .w = static_cast<stbrp_coord>(size.width), .h = static_cast<stbrp_coord>(size.height), .x = 0, .y = 0, .was_packed = 0};

		if (stbrp_pack_rects(&pack_context_->context, &rect, 1))
		{
			const point_type point{static_cast<point_type::value_type>(rect.x), static_cast<point_type::value_type>(rect.y)};

			auto* address = data_.get() + (point.y * size.width + point.x);
			const auto mapping = BorrowTexture::data_type::mapping_type{
					std::dextents<size_type::value_type, 2>{size.height, size.width},
					std::array<size_type::value_type, 2>{size_.width, 1},
			};
			const auto data = BorrowTexture::data_type{address, mapping};

			dirty_ = true;
			return {point, data};
		}

		return {BorrowTexture::invalid_point, {}};
	}

	BorrowTexture::BorrowTexture(const point_type point, const data_type data) noexcept
		: point_{point},
		  data_{data} {}

	auto BorrowTexture::valid() const noexcept -> bool
	{
		return point_ != invalid_point;
	}

	auto BorrowTexture::point() const noexcept -> point_type
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		return point_;
	}

	auto BorrowTexture::fill(const size_type::value_type y, const size_type::value_type offset, const size_type::value_type n, const element_type element) const noexcept -> void
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

	auto BorrowTexture::fill(const size_type::value_type y, const size_type::value_type offset, const data_view_type data) const noexcept -> void
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

	auto BorrowTexture::fill(const size_type::value_type y, const size_type::value_type n, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(n <= width);

		fill(y, 0, n, element);
	}

	auto BorrowTexture::fill(const size_type::value_type y, const data_view_type data) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() <= width);

		fill(y, 0, data);
	}

	auto BorrowTexture::fill(const size_type::value_type y, const element_type element) const noexcept -> void
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);

		fill(y, width, element);
	}

	auto BorrowTexture::fill(const element_type element) const noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());

		const auto height = data_.extent(0);
		for (size_type::value_type y = 0; y < height; ++y)
		{
			fill(y, element);
		}
	}

	auto BorrowTexture::fill(const data_view_type data) const noexcept -> void
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

	auto BorrowTexture::operator[](const size_type::value_type x, const size_type::value_type y) const noexcept -> data_type::reference
	{
		const auto width = data_.extent(1);
		const auto height = data_.extent(0);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(valid());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(y < height);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(x < width);

		return data_[y, x];
	}
} // namespace gal::prometheus::gfx
