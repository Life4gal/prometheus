// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

// todo
#define GAL_PROMETHEUS_GFX_RENDER_DX12

#if defined(GAL_PROMETHEUS_GFX_RENDER_DX12)

#include <vector>

#include <gfx/renderer.hpp>

#include <d3d12.h>
#include <wrl/client.h>

namespace gal::prometheus::gfx
{
	using Microsoft::WRL::ComPtr;

	class Dx12Renderer final : public Renderer
	{
	public:
		struct texture_type
		{
			ComPtr<ID3D12Resource> resource;
			D3D12_CPU_DESCRIPTOR_HANDLE cpu;
			D3D12_GPU_DESCRIPTOR_HANDLE gpu;
		};

		using textures_type = std::vector<texture_type>;

	private:
		struct render_buffer_type
		{
			ComPtr<ID3D12Resource> index;
			UINT index_count;
			ComPtr<ID3D12Resource> vertex;
			UINT vertex_count;
		};

		ComPtr<ID3D12Device> device_;
		ComPtr<ID3D12GraphicsCommandList> command_list_;

		ComPtr<ID3D12RootSignature> root_signature_;
		ComPtr<ID3D12PipelineState> pipeline_state_;
		ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap_;
		// 8 descriptors are enough, and if not, then 1.5 times more
		UINT srv_max_size_;

		textures_type textures_;

		constexpr static UINT num_frames_in_flight = 3;
		// note: overflow(max + 1 => 0)
		UINT frame_resource_index_ = (std::numeric_limits<UINT>::max)();
		// render_buffer_type frame_resource_[num_frames_in_flight] = {};
		// num_frames_in_flight < 16
		render_buffer_type frame_resource_[16] = {};

		render_buffer_type render_buffer_;

		[[nodiscard]] auto create_root_signature() noexcept -> bool;
		[[nodiscard]] auto create_pipeline_state() noexcept -> bool;
		[[nodiscard]] auto create_srv_descriptor_heap(UINT num) noexcept -> bool;

		[[nodiscard]] auto upload_texture(
			std::size_t index,
			Texture::data_view_type data,
			Texture::size_type size,
			bool record_resource = true
		) noexcept -> texture_id_type;

	public:
		Dx12Renderer() noexcept;
		Dx12Renderer(ID3D12Device* device, ID3D12GraphicsCommandList* command_list) noexcept;
		Dx12Renderer(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> command_list) noexcept;

		auto bind_device(ID3D12Device* device) noexcept -> void;
		auto bind_device(ComPtr<ID3D12Device> device) noexcept -> void;
		auto bind_command_list(ID3D12GraphicsCommandList* command_list) noexcept -> void;
		auto bind_command_list(ComPtr<ID3D12GraphicsCommandList> command_list) noexcept -> void;

	private:
		auto do_create() noexcept -> bool override;
		auto do_destroy() noexcept -> void override;

		[[nodiscard]] auto do_ready() const noexcept -> bool override;

		auto do_create_texture(Texture::data_view_type data, Texture::size_type size) noexcept -> texture_id_type override;
		auto do_update_texture(const Texture& texture) noexcept -> void override;
		auto do_destroy_texture(texture_id_type texture_id) noexcept -> void override;

		auto do_present(const RenderContext& renderer_context, const rect_type& display_area) noexcept -> void override;
	};
}

#endif
