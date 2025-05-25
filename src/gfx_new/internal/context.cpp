// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx_new/internal/context.hpp>

#include "render_list.hpp"

#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

namespace gal::prometheus::gfx_new
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
		auto renderer = context.get_renderer();

		std::ranges::for_each(
			texture_atlas_list_,
			[&renderer](auto& texture) mutable noexcept -> void
			{
				if (not texture.uploaded())
				{
					renderer.upload(texture);
				}
				else
				{
					renderer.update_if_dirty(texture);
				}
			}
		);
	}

	// =========================================================
	// FONT
	// =========================================================

	auto FontContext::set_glyph_parser(std::shared_ptr<GlyphParser> glyph_parser) noexcept -> void
	{
		glyph_parser_ = std::move(glyph_parser);
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

	Context::Context(std::shared_ptr<GlyphParser> glyph_parser, std::shared_ptr<Renderer> renderer) noexcept
		: glyph_parser_{nullptr},
		  renderer_{nullptr},
		  texture_context_{},
		  font_context_{}
	{
		set_glyph_parser(std::move(glyph_parser));
		set_renderer(std::move(renderer));
	}

	auto Context::get_glyph_parser() const noexcept -> std::shared_ptr<GlyphParser>
	{
		return glyph_parser_;
	}

	auto Context::set_glyph_parser(std::shared_ptr<GlyphParser> glyph_parser) noexcept -> std::shared_ptr<GlyphParser>
	{
		auto old = std::exchange(glyph_parser_, glyph_parser);
		font_context_.set_glyph_parser(glyph_parser_);

		return old;
	}

	// auto Context::get_renderer() noexcept -> std::shared_ptr<Renderer>
	// {
	// 	return renderer_;
	// }

	auto Context::get_renderer() const noexcept -> RendererAccessor
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer_ != nullptr);

		return RendererAccessor{*renderer_};
	}

	auto Context::set_renderer(std::shared_ptr<Renderer> renderer) noexcept -> std::shared_ptr<Renderer>
	{
		return std::exchange(renderer_, renderer);
	}

	auto Context::get_texture_context() const noexcept -> const TextureContext&
	{
		return texture_context_;
	}

	auto Context::get_font_context() noexcept -> FontContext&
	{
		return font_context_;
	}

	auto Context::get_render_list_shared_data() const noexcept -> const RenderListSharedData&
	{
		return render_list_shared_data_;
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
		auto* context = new Context{std::move(glyph_parser), std::move(renderer)};
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
		context.get_font_context().add_font(path);
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

	auto Renderer::present(Context& context) noexcept -> void
	{
		// glyphs
		{
			// note: newly added glyph information is not available until the next frame
			context.font_context_.upload_all_glyph(context.texture_context_);
		}

		do_present(context.render_data());
	}

	auto Renderer::end_frame(Context& context) noexcept -> void
	{
		context.render_lists_.clear();
	}
}
