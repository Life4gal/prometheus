// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/context.hpp>

#include <gfx/internal/render_list.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx
{
	// =========================================================
	// TEXTURE
	// =========================================================

	auto TextureContext::root_id() const noexcept -> texture_atlas_id_type
	{
		std::ignore = this;
		return 0;
	}

	TextureContext::TextureContext() noexcept
	{
		// root atlas
		constexpr Texture::size_type root_texture_atlas_size{2048, 2048};
		texture_atlas_list_.emplace_back(root_texture_atlas_size);
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

			const auto texture_atlas_id = root_id();
			auto& texture = this->select(texture_atlas_id);

			// baked line rect area:
			// white pixel
			// ◿
			const auto borrowed_texture = texture.select(aa_size);
			borrowed_texture.fill(0);

			const auto aa_point = borrowed_texture.position();
			const auto aa_uv_scale = texture.uv();

			// white pixel
			{
				// LINE 0, 2 pixels
				borrowed_texture.fill(0, 2, white_color);
				// LINE 1, 2 pixels
				borrowed_texture.fill(1, 2, white_color);

				const auto uv_x = static_cast<point_type::value_type>(static_cast<float>(aa_point.x) + 1.0f) * aa_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(static_cast<float>(aa_point.y) + 1.0f) * aa_uv_scale.height;

				shared_data.white_pixel_uv = {uv_x, uv_y};
			}

			// ◿
			for (Texture::size_type::value_type y = 1; y < aa_height; ++y)
			{
				const auto line_width = y;
				const auto offset = aa_width - line_width;

				borrowed_texture.fill(y, offset, line_width, white_color);

				const auto p_x = aa_point.x + offset;
				const auto p_y = aa_point.y + y;
				const auto width = line_width;
				constexpr auto height = .5f;

				const auto uv_x = static_cast<point_type::value_type>(p_x) * aa_uv_scale.width;
				const auto uv_y = static_cast<point_type::value_type>(p_y) * aa_uv_scale.height;
				const auto uv_width = static_cast<point_type::value_type>(width) * aa_uv_scale.width;
				const auto uv_height = static_cast<point_type::value_type>(height) * aa_uv_scale.height;

				shared_data.baked_line_uvs[y] = {uv_x, uv_y, uv_width, uv_height};
			}
		}
	}

	auto TextureContext::root() noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlas_list_.empty());

		return texture_atlas_list_[root_id()];
	}

	auto TextureContext::root() const noexcept -> const Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture_atlas_list_.empty());

		return texture_atlas_list_[root_id()];
	}

	auto TextureContext::select(const texture_atlas_id_type texture_atlas_id) noexcept -> Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < texture_atlas_list_.size());

		return texture_atlas_list_[texture_atlas_id];
	}

	auto TextureContext::select(const texture_atlas_id_type texture_atlas_id) const noexcept -> const Texture&
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_atlas_id < texture_atlas_list_.size());

		return texture_atlas_list_[texture_atlas_id];
	}

	auto TextureContext::write(
		const texture_atlas_id_type texture_atlas_id,
		const Texture::data_view_type data,
		const Texture::size_type size
	) noexcept -> primitive::basic_rect_2d<Texture::uv_type::value_type>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(data.size() >= static_cast<std::size_t>(size.width) * size.height);

		auto& texture = this->select(texture_atlas_id);
		const auto texture_uv = texture.uv();

		const auto borrowed_texture = texture.select(size);
		borrowed_texture.fill(data);

		const auto position = borrowed_texture.position();
		return {position.to<point_type>() * texture_uv, size.to<extent_type>() * texture_uv};
	}

	auto TextureContext::write(
		const Texture::data_view_type data,
		const Texture::size_type size
	) noexcept -> random_write_result_type
	{
		// todo
		const auto id = root_id();
		const auto uv = this->write(id, data, size);

		return {.texture_atlas_id = id, .uv = uv};
	}

	auto TextureContext::upload(const Context& context) noexcept -> void
	{
		auto renderer_accessor = context.renderer_accessor();

		std::ranges::for_each(
			texture_atlas_list_,
			[&renderer_accessor](auto& texture) mutable noexcept -> void
			{
				if (not texture.uploaded())
				{
					renderer_accessor.upload(texture);
				}
				else
				{
					renderer_accessor.update_if_dirty(texture);
				}
			}
		);
	}

	// =========================================================
	// FONT
	// =========================================================

	auto FontContext::set_glyph_parser(GlyphParser& glyph_parser) noexcept -> void
	{
		glyph_parser_ = std::addressof(glyph_parser);
	}

	auto FontContext::set_fallback_glyph() noexcept -> void
	{
		if (fallback_glyph_ == nullptr)
		{
			{
				constexpr GlyphKey key{u'\xFFFD', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}

			if (fallback_glyph_ == nullptr)
			{
				constexpr GlyphKey key{u'?', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}

			if (fallback_glyph_ == nullptr)
			{
				constexpr GlyphKey key{u' ', 16u, GlyphFlag::NONE};
				fallback_glyph_ = glyph_of(key);
			}
		}
	}

	auto FontContext::add_font(const std::filesystem::path& path) noexcept -> void
	{
		font_load_queue_.push(path);
	}

	auto FontContext::load_all_font() noexcept -> void
	{
		const auto loader = [this](Font&& font) noexcept -> void
		{
			font_list_.emplace_back(std::move(font));
		};

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser_ != nullptr, "Use Renderer::set_glyph_parser to set a valid glyph parser first!");
		font_load_queue_.upload(*glyph_parser_, loader);
	}

	auto FontContext::glyph_of(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return info;
			}
		}

		return nullptr;
	}

	auto FontContext::glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*
	{
		return glyph_of({codepoint, size, flag});
	}

	auto FontContext::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::glyph_of_or_fallback(const GlyphKey& key) const noexcept -> const GlyphInfo*
	{
		for (const auto& font: font_list_)
		{
			if (const auto* info = font.get_glyph(key); info != nullptr)
			{
				return info;
			}
		}

		return fallback_glyph_;
	}

	auto FontContext::glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) const noexcept -> const GlyphInfo*
	{
		return this->glyph_of_or_fallback({codepoint, size, flag});
	}

	auto FontContext::glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) const noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of_or_fallback(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::glyph_of(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = std::as_const(*this).glyph_of(key); info != nullptr)
		{
			return info;
		}

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser_ != nullptr);
		for (auto& font: font_list_)
		{
			if (auto result = glyph_parser_->parse(font.descriptor.id, key); result.valid())
			{
				auto& queue = glyph_upload_queue_list_[std::addressof(font)];
				auto& inserted_info = font.set_glyph(key, result);

				queue.push(inserted_info, std::move(result));
				return std::addressof(inserted_info);
			}
		}

		return nullptr;
	}

	auto FontContext::glyph_of(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return this->glyph_of({codepoint, size, flag});
	}

	auto FontContext::glyph_of(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::glyph_of_or_fallback(const GlyphKey& key) noexcept -> const GlyphInfo*
	{
		if (const auto* info = glyph_of(key); info != nullptr)
		{
			return info;
		}

		return fallback_glyph_;
	}

	auto FontContext::glyph_of_or_fallback(std::uint32_t codepoint, std::uint32_t size, GlyphFlag flag) noexcept -> const GlyphInfo*
	{
		return this->glyph_of_or_fallback({codepoint, size, flag});
	}

	auto FontContext::glyph_of_or_fallback(std::u32string_view text, std::uint32_t size, GlyphFlag flag) noexcept -> std::vector<const GlyphInfo*>
	{
		std::vector<const GlyphInfo*> infos;
		infos.reserve(text.size());

		std::ranges::for_each(
			text,
			[&infos, this, size, flag](const auto codepoint) noexcept -> void
			{
				const auto* info = this->glyph_of_or_fallback(codepoint, size, flag);
				infos.emplace_back(info);
			}
		);

		return infos;
	}

	auto FontContext::upload_all_glyph(TextureContext& context) noexcept -> void
	{
		std::ranges::for_each(
			glyph_upload_queue_list_ | std::views::values,
			[&context](auto& queue) noexcept -> void
			{
				queue.upload(context);
			}
		);
		glyph_upload_queue_list_.clear();
	}

	// =========================================================
	// CONTEXT
	// =========================================================

	Context::RendererAccessor::RendererAccessor(Renderer& renderer) noexcept
		: renderer_{renderer} {}

	auto Context::RendererAccessor::upload(Texture& texture) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(not texture.uploaded());

		auto& renderer = renderer_.get();

		texture.texture_.id = renderer.do_texture_create(texture.data(), texture.size());
		texture.texture_.dirty = false;
	}

	auto Context::RendererAccessor::update(Texture& texture) noexcept -> void
	{
		auto& renderer = renderer_.get();

		renderer.do_texture_update(texture.texture_);
		texture.texture_.dirty = false;
	}

	auto Context::RendererAccessor::update_if_dirty(Texture& texture) noexcept -> void
	{
		if (texture.dirty())
		{
			update(texture);
		}
	}

	Context::TextureAccessor::TextureAccessor(TextureContext& texture_context) noexcept
		: texture_context_{texture_context} {}

	auto Context::TextureAccessor::texture_context() const noexcept -> const TextureContext&
	{
		static_assert(std::is_const_v<memory::RefWrapper<const TextureContext>::type>);

		return texture_context_;
	}

	Context::FontAccessor::FontAccessor(FontContext& font_context) noexcept
		: font_context_{font_context} {}

	auto Context::FontAccessor::font_context() noexcept -> FontContext&
	{
		return font_context_;
	}

	auto Context::FontAccessor::font_context() const noexcept -> const FontContext&
	{
		return font_context_;
	}

	Context::RenderListAccessor::RenderListAccessor(RenderListSharedData& render_list_shared_data) noexcept
		: render_list_shared_data_{render_list_shared_data} {}

	auto Context::RenderListAccessor::shared_data() const noexcept -> const RenderListSharedData&
	{
		return render_list_shared_data_;
	}

	Context::Context() noexcept
		: glyph_parser_{nullptr},
		  renderer_{nullptr},
		  texture_context_{},
		  font_context_{}
	{
		texture_context_.initialize(render_list_shared_data_);
	}

	auto Context::set_glyph_parser(std::shared_ptr<GlyphParser> glyph_parser) noexcept -> std::shared_ptr<GlyphParser>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr, "GlyphParser must not be null!");

		auto old = std::exchange(glyph_parser_, glyph_parser);
		font_context_.set_glyph_parser(*glyph_parser_);

		return old;
	}

	auto Context::set_renderer(std::shared_ptr<Renderer> renderer) noexcept -> std::shared_ptr<Renderer>
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer != nullptr, "Renderer must not be null!");

		return std::exchange(renderer_, renderer);
	}

	auto Context::renderer_accessor() const noexcept -> RendererAccessor
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer_ != nullptr);

		return RendererAccessor{*renderer_};
	}

	auto Context::texture_accessor() noexcept -> TextureAccessor
	{
		return TextureAccessor{texture_context_};
	}

	auto Context::font_accessor() noexcept -> FontAccessor
	{
		return FontAccessor{font_context_};
	}

	auto Context::render_list_accessor() noexcept -> RenderListAccessor
	{
		return RenderListAccessor{render_list_shared_data_};
	}

	auto Context::new_render_list(const RenderListFlag flag) noexcept -> RenderList&
	{
		RenderList render_list{*this, flag};
		return render_lists_.emplace_back(std::move(render_list));
	}

	auto Context::render_data() const noexcept -> render_data_list_type
	{
		render_data_list_type all_render_data{};
		all_render_data.reserve(render_lists_.size());

		std::ranges::transform(
			render_lists_,
			std::back_inserter(all_render_data),
			[](const RenderList& render_list) noexcept -> RenderData
			{
				auto& context = *render_list.context_;

				return {.vertex_list = context.vertex_list, .index_list = context.index_list, .command_list = context.command_list};
			}
		);

		return all_render_data;
	}

	[[nodiscard]] auto create_context(std::shared_ptr<GlyphParser> glyph_parser, std::shared_ptr<Renderer> renderer) noexcept -> Context*
	{
		auto* context = new Context{};
		context->set_glyph_parser(std::move(glyph_parser));
		context->set_renderer(std::move(renderer));

		return context;
	}

	auto destroy_context(Context& context) noexcept -> void
	{
		delete std::addressof(context);
	}

	auto destroy_context(Context* context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(context != nullptr);
		destroy_context(*context);
	}

	auto set_glyph_parser(Context& context, std::shared_ptr<GlyphParser> glyph_parser) noexcept -> std::shared_ptr<GlyphParser>
	{
		return context.set_glyph_parser(std::move(glyph_parser));
	}

	auto set_renderer(Context& context, std::shared_ptr<Renderer> renderer) noexcept -> std::shared_ptr<Renderer>
	{
		return context.set_renderer(std::move(renderer));
	}

	auto add_font(Context& context, const std::filesystem::path& path) noexcept -> void
	{
		auto font_accessor = context.font_accessor();

		font_accessor.font_context().add_font(path);
	}

	auto new_render_list(Context& context, const RenderListFlag flag) noexcept -> RenderList&
	{
		return context.new_render_list(flag);
	}

	GlyphParser::~GlyphParser() noexcept = default;

	Renderer::~Renderer() noexcept = default;

	Renderer::Renderer() noexcept = default;

	auto Renderer::construct() noexcept -> bool
	{
		return do_construct();
	}

	auto Renderer::destruct() noexcept -> void
	{
		return do_destruct();
	}

	auto Renderer::ready() const noexcept -> bool
	{
		return do_ready();
	}

	auto Renderer::new_frame(Context& context) noexcept -> void
	{
		// font
		{
			context.font_context_.load_all_font();
			context.font_context_.set_fallback_glyph();
		}
		// texture
		{
			context.texture_context_.upload(context);
		}
	}

	auto Renderer::present(Context& context, const extent_type& display_size) noexcept -> void
	{
		// glyphs
		{
			// note: newly added glyph information is not available until the next frame
			context.font_context_.upload_all_glyph(context.texture_context_);
		}

		do_present(context.render_data(), display_size);
	}

	auto Renderer::end_frame(Context& context) noexcept -> void
	{
		context.render_lists_.clear();
	}
}
