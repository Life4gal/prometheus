// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/internal/context.hpp>

#include <gfx/renderer.hpp>

namespace gal::prometheus::gfx
{
	Context::Context(Context&&) noexcept = default;

	auto Context::operator=(Context&&) noexcept -> Context& = default;

	Context::~Context() noexcept = default;

	Context::Context() noexcept
		: glyph_parser{nullptr},
		  renderer{nullptr},
		  private_{memory::make_unique<ContextPrivate>()}
	{
		// ReSharper disable once CppUseStructuredBinding
		auto& context_private = *private_;
		auto& glyph_context = context_private.glyph_context;

		glyph_context.glyph_parser = &glyph_parser;
	}

	auto Context::initialize() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer != nullptr);

		// ReSharper disable once CppUseStructuredBinding
		auto& context_private = *private_;
		auto& texture_context = context_private.texture_context;

		texture_context.initialize(render_list_shared_data);
	}

	auto Context::new_render_list() noexcept -> RenderList&
	{
		RenderList render_list{*this};

		return render_lists.emplace_back(std::move(render_list));
	}

	auto Context::new_frame() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer != nullptr);

		// ReSharper disable once CppUseStructuredBinding
		auto& context_private = *private_;
		auto& texture_context = context_private.texture_context;

		texture_context.update_all_atlas(*renderer);
	}

	auto Context::present(const extent_type& display_size) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer != nullptr);

		// ReSharper disable once CppUseStructuredBinding
		auto& context_private = *private_;
		auto& texture_context = context_private.texture_context;
		auto& glyph_context = context_private.glyph_context;

		glyph_context.upload_all_glyph(texture_context);

		render_data_list_type all_render_data{};
		all_render_data.reserve(render_lists.size());

		std::ranges::transform(
			render_lists,
			std::back_inserter(all_render_data),
			&RenderList::data
		);

		renderer->present(all_render_data, display_size);
	}

	auto Context::end_frame() noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(glyph_parser != nullptr);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(renderer != nullptr);

		render_lists.clear();
	}
}
