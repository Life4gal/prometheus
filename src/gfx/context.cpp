// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/context.hpp>

#include <fstream>

#include <gfx/render_list.hpp>

#include <chars/chars.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	auto TextureContext::root_atlas() noexcept -> Texture&
	{
		return texture_atlases_.front();
	}

	auto TextureContext::root_atlas() const noexcept -> const Texture&
	{
		return texture_atlases_.front();
	}

	auto TextureContext::select_atlas(const texture_atlas_id_type id) noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(id < texture_atlases_.size());

		return texture_atlases_[id];
	}

	auto TextureContext::select_atlas(const Texture::size_type size) const noexcept -> texture_atlas_id_type
	{
		// width/height == 0 ==> whitespace
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width >= 0 and size.height >= 0);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlases_.empty());

		std::ignore = size;

		// todo: root only?
		return 0;
	}

	auto TextureContext::make_territory(const Texture::size_type size) noexcept -> BorrowTexture
	{
		// width/height == 0 ==> whitespace
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(size.width >= 0 and size.height >= 0);

		const auto atlas_id = select_atlas(size);
		auto& atlas = select_atlas(atlas_id);

		const auto texture = atlas.select(size);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.valid());

		Territory territory{.id = atlas_id, .point = texture.point(), .size = size};
		territories_.emplace_back(territory);

		return texture;
	}

	TextureContext::TextureContext() noexcept
		: parser_{nullptr}
	{
		// root atlas
		constexpr Texture::size_type root_texture_atlas_size{2048, 2048};
		texture_atlases_.emplace_back(root_texture_atlas_size);
	}

	auto TextureContext::initialize(RenderListSharedData& shared_data) noexcept -> void
	{
		// ========================================
		// BAKE LINES (AA)
		// ========================================
		{
			constexpr std::uint32_t white_color = 0xff'ff'ff'ff;
			constexpr auto aa_width = static_cast<Texture::size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_height = static_cast<Texture::size_type::value_type>(RenderListSharedData::baked_line_uv_count);
			constexpr auto aa_size = Texture::size_type{aa_width, aa_height};

			const auto atlas_id = select_atlas(aa_size);
			auto& atlas = select_atlas(atlas_id);

			const auto atlas_uv_scale = atlas.uv();

			const auto& aa_texture = make_territory(aa_size);
			const auto aa_point = aa_texture.point();

			// baked line rect area:
			// white pixel
			// ◿
			aa_texture.fill(0);

			// white pixel
			{
				aa_texture[0, 0] = white_color;
				aa_texture[1, 0] = white_color;
				aa_texture[0, 1] = white_color;
				aa_texture[1, 1] = white_color;

				const auto uv_x = static_cast<point_type::value_type>(static_cast<float>(aa_point.x) + .5f) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(static_cast<float>(aa_point.y) + .5f) * atlas_uv_scale.height;

				shared_data.white_pixel_uv = {uv_x, uv_y};
			}

			// ◿
			for (Texture::size_type::value_type y = 1; y < aa_height; ++y)
			{
				const auto line_width = y;
				const auto offset = aa_width - line_width;

				aa_texture.fill(y, offset, line_width, white_color);

				const auto p_x = aa_point.x + offset;
				const auto p_y = aa_point.y + y;
				const auto width = line_width;
				constexpr auto height = .5f;

				const auto uv_x = static_cast<point_type::value_type>(p_x) * atlas_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(p_y) * atlas_uv_scale.height;
				const auto uv_width = static_cast<point_type::value_type>(width) * atlas_uv_scale.width;
				const auto uv_height = static_cast<point_type::value_type>(height) * atlas_uv_scale.height;

				shared_data.baked_line_uvs[y] = {uv_x, uv_y, uv_width, uv_height};
			}
		}

		// ========================================
		// FontFace (load fallback glyph)
		// ========================================
		std::ranges::for_each(
			font_faces_,
			[](auto& font_face) noexcept -> void
			{
				font_face.initialize();
			}
		);
	}

	auto TextureContext::bind_parser(GlyphParser& parser) noexcept -> void
	{
		parser_ = std::addressof(parser);
	}

	auto TextureContext::parser() const noexcept -> GlyphParser&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);
		return *parser_;
	}

	auto TextureContext::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		std::ifstream file{path, std::ios::binary};
		if (not file.is_open())
		{
			// todo: error handling
			return false;
		}

		file.seekg(0, std::ios::end);
		const auto size = file.tellg();

		auto* data = new FontData::element_type[size];
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(data), size);
		file.close();

		font_data_list_.emplace_back(std::unique_ptr<FontData::element_type>{data}, static_cast<FontData::size_type>(size));
		return true;
	}

	auto TextureContext::root_texture() const noexcept -> texture_id_type
	{
		return root_atlas().id();
	}

	auto TextureContext::atlas_of(const GlyphInfo& info) noexcept -> const Texture&
	{
		return select_atlas(info.texture_atlas_id);
	}

	auto TextureContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		for (auto& face: font_faces_)
		{
			if (const auto* info = face.find_glyph_no_fallback({.codepoint = codepoint, .size = size, .flag = flag}); info != nullptr)
			{
				return info;
			}
		}

		return nullptr;
	}

	auto TextureContext::glyph_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		const auto utf32_text = chars::convert<chars::CharsType::UTF8_CHAR, chars::CharsType::UTF32>(text);

		std::vector<const GlyphInfo*> infos;
		infos.reserve(utf32_text.size());

		std::ranges::for_each(
			utf32_text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto TextureContext::size_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		const auto* info = glyph_of(codepoint, size, flag);
		if (info == nullptr)
		{
			return {0, 0};
		}

		return {info->advance_x, info->rect.height()};
	}

	auto TextureContext::size_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		const auto infos = glyph_of(text, size, flag);

		extent_type total_size{0, 0};
		std::ranges::for_each(
			infos,
			[&total_size](const auto* info) noexcept -> void
			{
				if (info == nullptr)
				{
					return;
				}

				total_size.width += info->advance_x;
				total_size.height = std::max(total_size.height, info->rect.height());
			}
		);

		return total_size;
	}

	auto TextureContext::load_all_font() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(parser_ != nullptr);

		std::ranges::for_each(
			font_data_list_,
			[this](FontData& data) noexcept -> void
			{
				// note: Transferring ownership of font file data
				if (auto result = parser_->load(std::move(data.data), data.size); result.valid())
				{
					font_faces_.emplace_back(*this, std::move(result.name), result.id);
				}
				else
				{
					// todo: error handling
					GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
				}
			}
		);
		font_data_list_.clear();
	}

	auto TextureContext::upload_all_font_face() noexcept -> void
	{
		std::ranges::for_each(font_faces_, &FontFace::upload);
	}

	auto TextureContext::upload_glyph_to_texture(GlyphInfo& info, const GlyphParsedInfo::data_type& data) noexcept -> void
	{
		const auto width = static_cast<Texture::size_type::value_type>(info.rect.width());
		const auto height = static_cast<Texture::size_type::value_type>(info.rect.height());
		const auto size = Texture::size_type{width, height};

		const auto atlas_id = select_atlas(size);
		const auto& atlas = select_atlas(atlas_id);

		const auto atlas_uv_scale = atlas.uv();

		const auto& texture = make_territory(size);
		texture.fill({data.get(), static_cast<std::size_t>(width) * height});

		const auto texture_point = texture.point();
		info.texture_atlas_id = atlas_id;
		info.uv.point = texture_point.to<point_type>() * atlas_uv_scale;
		info.uv.extent = size.to<extent_type>() * atlas_uv_scale;
	}

	auto TextureContext::upload_all_texture(Renderer& renderer) noexcept -> void
	{
		std::ranges::for_each(
			texture_atlases_,
			[&renderer](auto& texture) noexcept -> void
			{
				if (not texture.uploaded())
				{
					texture.create(renderer);
				}
				else
				{
					texture.upload_if_required(renderer);
				}
			}
		);
	}

	RenderContext::~RenderContext() noexcept = default;

	RenderContext::RenderContext() noexcept = default;

	auto RenderContext::initialize() noexcept -> void
	{
		texture_context_.initialize(render_list_shared_data_);
	}

	auto RenderContext::begin_frame(Renderer& renderer) noexcept -> void
	{
		texture_context_.load_all_font();
		texture_context_.upload_all_texture(renderer);
	}

	auto RenderContext::end_frame(Renderer& renderer) noexcept -> void
	{
		std::ignore = renderer;
		texture_context_.upload_all_font_face();
	}

	auto RenderContext::bind_parser(GlyphParser& parser) noexcept -> void
	{
		texture_context_.bind_parser(parser);
	}

	auto RenderContext::add_font(const std::filesystem::path& path) noexcept -> bool
	{
		return texture_context_.add_font(path);
	}

	auto RenderContext::root_texture() const noexcept -> texture_id_type
	{
		return texture_context_.root_texture();
	}

	auto RenderContext::atlas_of(const GlyphInfo& info) noexcept -> const Texture&
	{
		return texture_context_.atlas_of(info);
	}

	auto RenderContext::glyph_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return texture_context_.glyph_of(codepoint, size, flag);
	}

	auto RenderContext::glyph_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		return texture_context_.glyph_of(text, size, flag);
	}

	auto RenderContext::size_of(const std::uint32_t codepoint, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		return texture_context_.size_of(codepoint, size, flag);
	}

	auto RenderContext::size_of(const std::string_view text, const std::uint32_t size, const GlyphFlag flag) noexcept -> extent_type
	{
		return texture_context_.size_of(text, size, flag);
	}

	auto RenderContext::render_list_shared_data() const noexcept -> const RenderListSharedData&
	{
		return render_list_shared_data_;
	}

	auto RenderContext::render_data() const noexcept -> std::vector<RenderData>
	{
		std::vector<RenderData> all_render_data{};
		all_render_data.reserve(render_lists_.size());

		std::ranges::for_each(
			render_lists_,
			[&all_render_data](const auto& render_list) noexcept -> void
			{
				all_render_data.emplace_back(render_list.render_data());
			}
		);

		return all_render_data;
	}

	auto RenderContext::test_render_list() noexcept -> RenderList&
	{
		if (render_lists_.empty())
		{
			render_lists_.emplace_back(RenderListFlag::ANTI_ALIASED_LINE | RenderListFlag::ANTI_ALIASED_LINE_USE_TEXTURE | RenderListFlag::ANTI_ALIASED_FILL, *this);
		}

		return render_lists_.front();
	}
} // namespace gal::prometheus::gfx
