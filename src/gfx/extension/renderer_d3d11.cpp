// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/extension/renderer_d3d11.hpp>

#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_COMPILER_DEBUG
#define GAL_PROMETHEUS_GFX_DEBUG
#include <source_location>
#endif

#include <print>

#include <platform/os.hpp>
#include GAL_PROMETHEUS_ERROR_DEBUG_MODULE

#include <comdef.h>
#include <d3dcompiler.h>

namespace
{
	using namespace gal::prometheus;
	using namespace gfx;

	auto check_hr_error(
		const HRESULT result
#if defined(GAL_PROMETHEUS_GFX_DEBUG)
		,
		const std::source_location& location = std::source_location::current()
#endif
	) noexcept -> bool
	{
		if (SUCCEEDED(result))
		{
			return true;
		}

#if defined(GAL_PROMETHEUS_GFX_DEBUG)

		const _com_error err{result};
		std::println(stderr, "Error: {} --- at {}:{}", err.ErrorMessage(), location.file_name(), location.line());

		GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();

#else

		GAL_PROMETHEUS_COMPILER_UNREACHABLE();

#endif

		return false;
	}

	using projection_matrix_type = float[4][4];

	[[nodiscard]] auto id_to_gpu_handle(const texture_id_type id) noexcept -> ID3D11ShaderResourceView*
	{
		return reinterpret_cast<ID3D11ShaderResourceView*>(id); // NOLINT(performance-no-int-to-ptr)
	}

	[[nodiscard]] auto gpu_handle_to_id(const ID3D11ShaderResourceView* srv) noexcept -> texture_id_type
	{
		return reinterpret_cast<texture_id_type>(srv);
	}
}

namespace gal::prometheus::gfx
{
	auto RendererD3D11::create_blend_state() noexcept -> bool
	{
		constexpr D3D11_RENDER_TARGET_BLEND_DESC render_target
		{
				.BlendEnable = TRUE,
				.SrcBlend = D3D11_BLEND_SRC_ALPHA,
				.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
		};
		constexpr D3D11_BLEND_DESC blend_desc
		{
				.AlphaToCoverageEnable = FALSE,
				.IndependentBlendEnable = FALSE,
				.RenderTarget =
				{
						render_target,
				}
		};

		return check_hr_error(device_->CreateBlendState(&blend_desc, blend_state_.ReleaseAndGetAddressOf()));
	}

	auto RendererD3D11::create_rasterizer_state() noexcept -> bool
	{
		constexpr D3D11_RASTERIZER_DESC rasterizer_desc
		{
				.FillMode = D3D11_FILL_SOLID,
				.CullMode = D3D11_CULL_NONE,
				.FrontCounterClockwise = FALSE,
				.DepthBias = 0,
				.DepthBiasClamp = 0,
				.SlopeScaledDepthBias = 0,
				.DepthClipEnable = TRUE,
				.ScissorEnable = TRUE,
				.MultisampleEnable = TRUE,
				.AntialiasedLineEnable = TRUE
		};

		return check_hr_error(device_->CreateRasterizerState(&rasterizer_desc, rasterizer_state_.ReleaseAndGetAddressOf()));
	}

	auto RendererD3D11::create_depth_stencil_state() noexcept -> bool
	{
		constexpr D3D11_DEPTH_STENCIL_DESC depth_stencil_desc
		{
				.DepthEnable = FALSE,
				.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D11_COMPARISON_ALWAYS,
				.StencilEnable = FALSE,
				.StencilReadMask = 0,
				.StencilWriteMask = 0,
				.FrontFace = {.StencilFailOp = D3D11_STENCIL_OP_KEEP, .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP, .StencilPassOp = D3D11_STENCIL_OP_KEEP, .StencilFunc = D3D11_COMPARISON_ALWAYS},
				.BackFace = {.StencilFailOp = D3D11_STENCIL_OP_KEEP, .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP, .StencilPassOp = D3D11_STENCIL_OP_KEEP, .StencilFunc = D3D11_COMPARISON_ALWAYS}
		};

		return check_hr_error(device_->CreateDepthStencilState(&depth_stencil_desc, depth_stencil_state_.ReleaseAndGetAddressOf()));
	}

	auto RendererD3D11::create_vertex_shader() noexcept -> bool
	{
		constexpr char shader_code[]
		{
				"cbuffer vertexBuffer : register(b0)"
				"{"
				"	float4x4 ProjectionMatrix;"
				"};"
				"struct VS_INPUT"
				"{"
				"	float2 pos : POSITION;"
				"	float4 col : COLOR0;"
				"	float2 uv  : TEXCOORD0;"
				"};"
				"struct PS_INPUT"
				"{"
				"	float4 pos : SV_POSITION;"
				"	float4 col : COLOR0;"
				"	float2 uv  : TEXCOORD0;"
				"};"
				"PS_INPUT main(VS_INPUT input)"
				"{"
				"	PS_INPUT output;"
				"	output.pos = mul(ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));"
				"	output.col = input.col;"
				"	output.uv  = input.uv;"
				"	return output;"
				"}"
		};

		ComPtr<ID3DBlob> shader_blob;
		ComPtr<ID3DBlob> error_message;

		if (const auto result = D3DCompile(
				shader_code,
				sizeof(shader_code),
				nullptr,
				nullptr,
				nullptr,
				"main",
				"vs_5_0",
				0,
				0,
				shader_blob.GetAddressOf(),
				error_message.GetAddressOf()
			);
			FAILED(result))
		{
			std::println(
				stderr,
				"D3DCompile failed: {} -- at {}:{}",
				static_cast<const char*>(error_message->GetBufferPointer()),
				std::source_location::current().file_name(),
				std::source_location::current().line()
			);

			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
			return false;
		}

		if (not check_hr_error(
			device_->CreateVertexShader(
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				nullptr,
				vertex_shader_.ReleaseAndGetAddressOf()
			)
		))
		{
			return false;
		}

		// vertex input layout
		constexpr D3D11_INPUT_ELEMENT_DESC input_element_desc[]{
				{
						.SemanticName = "POSITION",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R32G32_FLOAT,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, position)),
						.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "COLOR",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, color)),
						.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "TEXCOORD",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R32G32_FLOAT,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, uv)),
						.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
		};
		if (not check_hr_error(
			device_->CreateInputLayout(
				input_element_desc,
				static_cast<UINT>(std::ranges::size(input_element_desc)),
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				vertex_input_layout_.ReleaseAndGetAddressOf()
			)
		))
		{
			return false;
		}

		// constant buffer
		constexpr D3D11_BUFFER_DESC constant_buffer_desc{
				.ByteWidth = sizeof(projection_matrix_type),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
		};
		if (not check_hr_error(
			device_->CreateBuffer(
				&constant_buffer_desc,
				nullptr,
				vertex_projection_matrix_.ReleaseAndGetAddressOf()
			)
		))
		{
			return false;
		}

		return true;
	}

	auto RendererD3D11::create_pixel_shader() noexcept -> bool
	{
		constexpr char shader_code[]
		{
				"struct PS_INPUT"
				"{"
				"	float4 pos : SV_POSITION;"
				"	float4 col : COLOR0;"
				"	float2 uv  : TEXCOORD0;"
				"};"
				"sampler sampler0;"
				"Texture2D texture0;"
				"float4 main(PS_INPUT input) : SV_Target"
				"{"
				"	float4 out_col = texture0.Sample(sampler0, input.uv);"
				"	return input.col * out_col;"
				"}"
		};

		ComPtr<ID3DBlob> shader_blob;
		ComPtr<ID3DBlob> error_message;

		if (const auto result = D3DCompile(
				shader_code,
				sizeof(shader_code),
				nullptr,
				nullptr,
				nullptr,
				"main",
				"ps_5_0",
				0,
				0,
				shader_blob.GetAddressOf(),
				error_message.GetAddressOf()
			);
			FAILED(result))
		{
			std::println(
				stderr,
				"D3DCompile failed: {} -- at {}:{}",
				static_cast<const char*>(error_message->GetBufferPointer()),
				std::source_location::current().file_name(),
				std::source_location::current().line()
			);

			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
			return false;
		}

		if (not check_hr_error(
			device_->CreatePixelShader(
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				nullptr,
				pixel_shader_.ReleaseAndGetAddressOf()
			)
		))
		{
			return false;
		}

		// Create pixel shader texture sampler
		constexpr D3D11_SAMPLER_DESC sampler_desc
		{
				.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
				.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
				.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
				.MipLODBias = 0,
				.MaxAnisotropy = 0,
				.ComparisonFunc = D3D11_COMPARISON_ALWAYS,
				.BorderColor = {0, 0, 0, 0},
				.MinLOD = 0,
				.MaxLOD = 0
		};
		if (not check_hr_error(
			device_->CreateSamplerState(
				&sampler_desc,
				pixel_font_sampler_.ReleaseAndGetAddressOf()
			)
		))
		{
			return false;
		}

		return true;
	}

	auto RendererD3D11::upload_texture(
		const TextureDescriptor::data_view_type data,
		const TextureDescriptor::size_type size,
		const D3D11_USAGE usage,
		const std::uint32_t bind_flags,
		const std::uint32_t cpu_access_flags,
		const std::uint32_t misc_flags,
		const bool record_resource
	) noexcept -> texture_id_type
	{
		const D3D11_TEXTURE2D_DESC texture_2d_desc
		{
				.Width = static_cast<UINT>(size.width),
				.Height = static_cast<UINT>(size.height),
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Usage = usage,
				.BindFlags = bind_flags,
				.CPUAccessFlags = cpu_access_flags,
				.MiscFlags = misc_flags
		};

		const D3D11_SUBRESOURCE_DATA subresource_data
		{
				.pSysMem = data.data(),
				.SysMemPitch = static_cast<UINT>(size.width * 4),
				.SysMemSlicePitch = 0
		};

		ID3D11Texture2D* texture_2d;
		if (not check_hr_error(
			device_->CreateTexture2D(
				&texture_2d_desc,
				&subresource_data,
				&texture_2d
			)
		))
		{
			return invalid_texture_id;
		}

		const D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc
		{
				.Format = texture_2d_desc.Format,
				.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
				.Texture2D =
				{
						.MostDetailedMip = 0,
						.MipLevels = texture_2d_desc.MipLevels
				}
		};

		ID3D11ShaderResourceView* srv = nullptr;
		if (not check_hr_error(
			device_->CreateShaderResourceView(
				texture_2d,
				&shader_resource_view_desc,
				&srv
			)
		))
		{
			return false;
		}

		if (record_resource)
		{
			textures_.insert_or_assign(srv, texture_2d);
		}
		else
		{
			texture_2d->Release();
		}

		return gpu_handle_to_id(srv);
	}

	RendererD3D11::RendererD3D11() noexcept
		: device_{nullptr},
		  device_immediate_context_{nullptr},
		  blend_state_{nullptr},
		  rasterizer_state_{nullptr},
		  depth_stencil_state_{nullptr},
		  vertex_shader_{nullptr},
		  vertex_input_layout_{nullptr},
		  vertex_projection_matrix_{nullptr},
		  pixel_shader_{nullptr},
		  pixel_font_sampler_{nullptr},
		  render_buffer_{} {}

	RendererD3D11::RendererD3D11(ID3D11Device* device, ID3D11DeviceContext* device_immediate_context) noexcept
		: RendererD3D11{}
	{
		bind_device(device);
		bind_device_context(device_immediate_context);
	}

	RendererD3D11::RendererD3D11(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept
		: RendererD3D11{}
	{
		bind_device(std::move(device));
		bind_device_context(std::move(device_immediate_context));
	}

	auto RendererD3D11::bind_device(ID3D11Device* device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = device;
	}

	auto RendererD3D11::bind_device(ComPtr<ID3D11Device> device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = std::move(device);
	}

	auto RendererD3D11::bind_device_context(ID3D11DeviceContext* device_immediate_context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device_immediate_context != nullptr);
		device_immediate_context_ = device_immediate_context;
	}

	auto RendererD3D11::bind_device_context(ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device_immediate_context != nullptr);
		device_immediate_context_ = std::move(device_immediate_context);
	}

	auto RendererD3D11::do_construct() noexcept -> bool
	{
		if (not create_blend_state())
		{
			return false;
		}
		if (not create_rasterizer_state())
		{
			return false;
		}
		if (not create_depth_stencil_state())
		{
			return false;
		}
		if (not create_vertex_shader())
		{
			return false;
		}
		if (not create_pixel_shader())
		{
			return false;
		}

		return true;
	}

	auto RendererD3D11::do_destruct() noexcept -> void
	{
		// ComPtr
		blend_state_ = nullptr;
		rasterizer_state_ = nullptr;
		depth_stencil_state_ = nullptr;
		vertex_shader_ = nullptr;
		vertex_input_layout_ = nullptr;
		vertex_projection_matrix_ = nullptr;
		pixel_shader_ = nullptr;
		pixel_font_sampler_ = nullptr;
		render_buffer_.index = nullptr;
		render_buffer_.index_count = 0;
		render_buffer_.vertex = nullptr;
		render_buffer_.vertex_count = 0;

		// RAW
		std::ranges::for_each(
			textures_,
			[](auto& kv) noexcept -> void
			{
				kv.first->Release();
				kv.second->Release();
			}
		);
		textures_.clear();

		// ComPtr
		// device_immediate_context_->ClearState();
		// device_immediate_context_->Flush();
		device_immediate_context_ = nullptr;
		device_ = nullptr;
	}

	auto RendererD3D11::do_ready() const noexcept -> bool
	{
		if (device_ == nullptr or device_immediate_context_ == nullptr)
		{
			return false;
		}

		if (blend_state_ == nullptr or rasterizer_state_ == nullptr or depth_stencil_state_ == nullptr)
		{
			return false;
		}

		if (vertex_shader_ == nullptr or vertex_input_layout_ == nullptr or vertex_projection_matrix_ == nullptr)
		{
			return false;
		}

		if (pixel_shader_ == nullptr or pixel_font_sampler_ == nullptr)
		{
			return false;
		}

		return true;
	}

	auto RendererD3D11::do_texture_create(const TextureDescriptor::data_view_type data, const TextureDescriptor::size_type size) noexcept -> texture_id_type
	{
		return upload_texture(data, size, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE, 0);
	}

	auto RendererD3D11::do_texture_update(const TextureDescriptor& texture) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.id != invalid_texture_id, "Create texture first!");
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.dirty != 0, "No need to update texture!");

		auto* srv = id_to_gpu_handle(texture.id);
		const auto it = textures_.find(srv);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(it != textures_.end(), "Invalid texture id");

		auto* texture_2d = it->second;
		D3D11_MAPPED_SUBRESOURCE mapped_resource{};
		if (const auto result = device_immediate_context_->Map(
			texture_2d,
			0,
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mapped_resource
		); result != S_OK)
		{
			// todo: error handling
			GAL_PROMETHEUS_COMPILER_DEBUG_TRAP();
			return;
		}

		const auto* source = texture.data.get();
		const auto source_length = static_cast<std::size_t>(texture.size.width) * texture.size.height;
		std::ranges::copy(source, source + source_length, static_cast<TextureDescriptor::element_type*>(mapped_resource.pData));

		device_immediate_context_->Unmap(texture_2d, 0);
	}

	auto RendererD3D11::do_texture_destroy(const texture_id_type texture_id) noexcept -> void
	{
		auto* srv = id_to_gpu_handle(texture_id);

		if (const auto it = textures_.find(srv); it != textures_.end())
		{
			srv->Release();
			it->first->Release();
			it->second->Release();

			textures_.erase(it);
		}
	}

	auto RendererD3D11::do_present(const render_data_list_type& render_data_list, const extent_type& display_size) noexcept -> void
	{
		auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = render_buffer_;

		const auto [total_vertex_count, total_index_count] = [&]() noexcept
		{
			struct sum
			{
				UINT vertex;
				UINT index;
			};

			return std::ranges::fold_left(
				render_data_list,
				sum{.vertex = 0, .index = 0},
				[](const sum s, const RenderData& render_data) noexcept -> sum
				{
					const auto vertex_list = render_data.vertex_list.get();
					const auto index_list = render_data.index_list.get();

					return {.vertex = s.vertex + static_cast<UINT>(vertex_list.size()), .index = s.index + static_cast<UINT>(index_list.size())};
				}
			);
		}();

		if (not this_frame_vertex_buffer or total_vertex_count > this_frame_vertex_count)
		{
			// todo: grow factor
			this_frame_vertex_count = total_vertex_count + 5000;

			const D3D11_BUFFER_DESC buffer_desc{
					.ByteWidth = static_cast<UINT>(this_frame_vertex_count * sizeof(vertex_type)),
					.Usage = D3D11_USAGE_DYNAMIC,
					.BindFlags = D3D11_BIND_VERTEX_BUFFER,
					.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
					.MiscFlags = 0,
					.StructureByteStride = 0
			};
			check_hr_error(device_->CreateBuffer(&buffer_desc, nullptr, this_frame_vertex_buffer.ReleaseAndGetAddressOf()));
		}
		if (not this_frame_index_buffer or total_index_count > this_frame_index_count)
		{
			// todo: grow factor
			this_frame_index_count = total_index_count + 10000;

			const D3D11_BUFFER_DESC buffer_desc{
					.ByteWidth = static_cast<UINT>(this_frame_index_count * sizeof(index_type)),
					.Usage = D3D11_USAGE_DYNAMIC,
					.BindFlags = D3D11_BIND_INDEX_BUFFER,
					.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
					.MiscFlags = 0,
					.StructureByteStride = 0
			};
			check_hr_error(device_->CreateBuffer(&buffer_desc, nullptr, this_frame_index_buffer.ReleaseAndGetAddressOf()));
		}

		// Upload vertex/index data into a single contiguous GPU buffer
		{
			D3D11_MAPPED_SUBRESOURCE mapped_vertex_resource;
			D3D11_MAPPED_SUBRESOURCE mapped_index_resource;
			check_hr_error(device_immediate_context_->Map(this_frame_vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_resource));
			check_hr_error(device_immediate_context_->Map(this_frame_index_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_index_resource));

			auto* mapped_vertex = static_cast<vertex_type*>(mapped_vertex_resource.pData);
			auto* mapped_index = static_cast<index_type*>(mapped_index_resource.pData);

			UINT vertex_offset = 0;
			UINT index_offset = 0;

			std::ranges::for_each(
				render_data_list,
				[&](const RenderData& render_data) noexcept -> void
				{
					const auto vertex_list = render_data.vertex_list.get();
					const auto index_list = render_data.index_list.get();

					// std::ranges::transform(
					// 		vertex_list,
					// 		mapped_vertex + vertex_offset,
					// 		[](const vertex_type& vertex) noexcept -> vertex_type
					// 		{
					// 			// return {
					// 			// 		.position = {vertex.position.x, vertex.position.y},
					// 			// 		.uv = {vertex.uv.x, vertex.uv.y},
					// 			// 		.color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)
					// 			// };
					// 			return std::bit_cast<vertex_type>(vertex);
					// 		}
					// );
					std::ranges::copy(vertex_list, mapped_vertex + vertex_offset);
					// std::ranges::transform(
					// 		index_list,
					// 		mapped_index + index_offset,
					// 		[vertex_offset](const index_type index) noexcept -> index_type
					// 		{
					// 			return static_cast<index_type>(index + vertex_offset);
					// 		}
					// );
					std::ranges::copy(index_list, mapped_index + index_offset);

					vertex_offset += static_cast<UINT>(vertex_list.size());
					index_offset += static_cast<UINT>(index_list.size());
				}
			);

			device_immediate_context_->Unmap(this_frame_vertex_buffer.Get(), 0);
			device_immediate_context_->Unmap(this_frame_index_buffer.Get(), 0);
		}

		// Setup orthographic projection matrix into our constant buffer
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource;
			check_hr_error(device_immediate_context_->Map(vertex_projection_matrix_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));

			auto* mapped_projection_matrix = static_cast<projection_matrix_type*>(mapped_resource.pData);

			constexpr auto left = 0.f;
			const auto right = display_size.width;
			constexpr auto top = 0.f;
			const auto bottom = display_size.height;

			const projection_matrix_type mvp{
					{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
					{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
					{0.0f, 0.0f, 0.5f, 0.0f},
					{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
			};
			std::memcpy(mapped_projection_matrix, &mvp, sizeof(projection_matrix_type));

			device_immediate_context_->Unmap(vertex_projection_matrix_.Get(), 0);
		}

		// Setup viewport
		{
			const D3D11_VIEWPORT viewport{
					.TopLeftX = .0f,
					.TopLeftY = .0f,
					.Width = display_size.width,
					.Height = display_size.height,
					.MinDepth = 0,
					.MaxDepth = 1
			};
			device_immediate_context_->RSSetViewports(1, &viewport);
		}

		// Bind shader and vertex buffers
		constexpr UINT stride = sizeof(vertex_type);
		constexpr UINT offset = 0;
		device_immediate_context_->IASetInputLayout(vertex_input_layout_.Get());
		device_immediate_context_->IASetVertexBuffers(0, 1, this_frame_vertex_buffer.GetAddressOf(), &stride, &offset);
		device_immediate_context_->IASetIndexBuffer(
			this_frame_index_buffer.Get(),
			// ReSharper disable once CppUnreachableCode
			sizeof(index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
			0
		);
		device_immediate_context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		device_immediate_context_->VSSetShader(vertex_shader_.Get(), nullptr, 0);
		device_immediate_context_->VSSetConstantBuffers(0, 1, vertex_projection_matrix_.GetAddressOf());
		device_immediate_context_->PSSetShader(pixel_shader_.Get(), nullptr, 0);
		device_immediate_context_->PSSetSamplers(0, 1, pixel_font_sampler_.GetAddressOf());
		device_immediate_context_->DSSetShader(nullptr, nullptr, 0);
		device_immediate_context_->HSSetShader(nullptr, nullptr, 0);
		device_immediate_context_->GSSetShader(nullptr, nullptr, 0);
		device_immediate_context_->CSSetShader(nullptr, nullptr, 0);

		// Setup render state
		constexpr float blend_factor[]{0, 0, 0, 0};
		device_immediate_context_->OMSetBlendState(blend_state_.Get(), blend_factor, (std::numeric_limits<UINT>::max)());
		device_immediate_context_->OMSetDepthStencilState(depth_stencil_state_.Get(), 0);
		device_immediate_context_->RSSetState(rasterizer_state_.Get());

		UINT total_index_offset = 0;
		std::ranges::for_each(
			render_data_list,
			[this, &total_index_offset](const RenderData& render_data) noexcept -> void
			{
				const auto vertex_list = render_data.vertex_list.get();
				const auto index_list = render_data.index_list.get();

				for (const auto& command_list = render_data.command_list.get();
				     const auto& [scissor, texture, index_offset, element_count]: command_list)
				{
					const auto [point, extent] = scissor;
					const D3D11_RECT rect
					{
							static_cast<LONG>(point.x),
							static_cast<LONG>(point.y),
							static_cast<LONG>(point.x + extent.width),
							static_cast<LONG>(point.y + extent.height)
					};
					device_immediate_context_->RSSetScissorRects(1, &rect);

					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture != invalid_texture_id);
					auto* srv = id_to_gpu_handle(texture);
					device_immediate_context_->PSSetShaderResources(0, 1, &srv);

					const auto this_index_offset = static_cast<UINT>(total_index_offset + index_offset);
					// device_immediate_context_->DrawIndexed(static_cast<UINT>(element_count), this_index_offset, 0);
					device_immediate_context_->DrawIndexedInstanced(static_cast<UINT>(element_count), 1, this_index_offset, 0, 0);
				}

				total_index_offset += static_cast<UINT>(index_list.size());
			}
		);
	}
}
