// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

// todo
#define GAL_PROMETHEUS_GFX_RENDER_DX11

#if defined(GAL_PROMETHEUS_GFX_RENDER_DX11)

#include <unordered_map>

#include <gfx/renderer.hpp>

#include <d3d11.h>
#include <wrl/client.h>

namespace gal::prometheus::gfx
{
	using Microsoft::WRL::ComPtr;

	class Dx11Renderer final : public Renderer
	{
	public:
		using textures_type = std::unordered_map<ID3D11ShaderResourceView*, ID3D11Texture2D*>;

	private:
		struct render_buffer_type
		{
			ComPtr<ID3D11Buffer> index;
			UINT index_count;
			ComPtr<ID3D11Buffer> vertex;
			UINT vertex_count;
		};

		ComPtr<ID3D11Device> device_;
		ComPtr<ID3D11DeviceContext> device_immediate_context_;

		ComPtr<ID3D11BlendState> blend_state_;
		ComPtr<ID3D11RasterizerState> rasterizer_state_;
		ComPtr<ID3D11DepthStencilState> depth_stencil_state_;

		ComPtr<ID3D11VertexShader> vertex_shader_;
		ComPtr<ID3D11InputLayout> vertex_input_layout_;
		ComPtr<ID3D11Buffer> vertex_projection_matrix_;

		ComPtr<ID3D11PixelShader> pixel_shader_;
		ComPtr<ID3D11SamplerState> pixel_font_sampler_;

		textures_type textures_;

		render_buffer_type render_buffer_;

		[[nodiscard]] auto create_blend_state() noexcept -> bool;
		[[nodiscard]] auto create_rasterizer_state() noexcept -> bool;
		[[nodiscard]] auto create_depth_stencil_state() noexcept -> bool;

		[[nodiscard]] auto create_vertex_shader() noexcept -> bool;
		[[nodiscard]] auto create_pixel_shader() noexcept -> bool;

		[[nodiscard]] auto upload_texture(
			Texture::data_view_type data,
			Texture::size_type size,
			D3D11_USAGE usage,
			std::uint32_t bind_flags,
			std::uint32_t cpu_access_flags,
			std::uint32_t misc_flags,
			bool record_resource = true
		) noexcept -> texture_id_type;

	public:
		Dx11Renderer() noexcept;
		Dx11Renderer(ID3D11Device* device, ID3D11DeviceContext* device_immediate_context) noexcept;
		Dx11Renderer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept;

		auto bind_device(ID3D11Device* device) noexcept -> void;
		auto bind_device(ComPtr<ID3D11Device> device) noexcept -> void;
		auto bind_device_context(ID3D11DeviceContext* device_immediate_context) noexcept -> void;
		auto bind_device_context(ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept -> void;

		auto create() noexcept -> bool override;
		auto destroy() noexcept -> void override;

		[[nodiscard]] auto ready() const noexcept -> bool override;

		auto present(const RenderContext& renderer_context, const rect_type& display_area) noexcept -> void override;

		auto create_texture(Texture::data_view_type data, Texture::size_type size) noexcept -> texture_id_type override;
		auto update_texture(const Texture& texture) noexcept -> void override;
		auto destroy_texture(texture_id_type texture_id) noexcept -> void override;
	};
}

#endif
