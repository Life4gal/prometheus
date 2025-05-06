// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/renderer_dx11.hpp>

#if defined(GAL_PROMETHEUS_GFX_RENDER_DX11)

#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_COMPILER_DEBUG
#define GAL_PROMETHEUS_GFX_DEBUG
#include <source_location>
#endif

#include <print>

#include <platform/os.hpp>
#include <gfx/font.hpp>
#include <utility>

#include <comdef.h>
#include <d3dcompiler.h>

namespace
{
	[[nodiscard]] auto check_hr_error(
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
}

namespace gal::prometheus::gfx
{
	auto Dx11Renderer::create_blend_state() noexcept -> bool
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

	auto Dx11Renderer::create_rasterizer_state() noexcept -> bool
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

	auto Dx11Renderer::create_depth_stencil_state() noexcept -> bool
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

	auto Dx11Renderer::create_vertex_shader() noexcept -> bool
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

	auto Dx11Renderer::create_pixel_shader() noexcept -> bool
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

		return false;
	}

	auto Dx11Renderer::create_texture(
		const TextureAtlas& texture,
		const D3D11_USAGE usage,
		const std::uint32_t bind_flags,
		const std::uint32_t cpu_access_flags,
		const std::uint32_t misc_flags,
		const bool record_resource
	) noexcept -> texture_id_type
	{
		const auto texture_size = texture.size();
		const auto texture_data = texture.data();

		const D3D11_TEXTURE2D_DESC texture_2d_desc
		{
				.Width = static_cast<UINT>(texture_size.width),
				.Height = static_cast<UINT>(texture_size.height),
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
				.pSysMem = texture_data.data(),
				.SysMemPitch = static_cast<UINT>(texture_size.width * 4),
				.SysMemSlicePitch = 0
		};

		ID3D11Texture2D* texture_2d = nullptr;
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

		return reinterpret_cast<texture_id_type>(srv);
	}

	Dx11Renderer::Dx11Renderer() noexcept
		: device_{nullptr},
		  device_immediate_context_{nullptr},
		  blend_state_{nullptr},
		  rasterizer_state_{nullptr},
		  depth_stencil_state_{nullptr},
		  vertex_shader_{nullptr},
		  vertex_input_layout_{nullptr},
		  vertex_projection_matrix_{nullptr},
		  pixel_shader_{nullptr},
		  pixel_font_sampler_{nullptr} {}

	Dx11Renderer::Dx11Renderer(ID3D11Device* device, ID3D11DeviceContext* device_immediate_context) noexcept
		: Dx11Renderer{}
	{
		bind_device(device);
		bind_device_context(device_immediate_context);
	}

	Dx11Renderer::Dx11Renderer(ComPtr<ID3D11Device> device, ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept
		: Dx11Renderer{}
	{
		bind_device(std::move(device));
		bind_device_context(std::move(device_immediate_context));
	}

	auto Dx11Renderer::bind_device(ID3D11Device* device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = device;
	}

	auto Dx11Renderer::bind_device(ComPtr<ID3D11Device> device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = std::move(device);
	}

	auto Dx11Renderer::bind_device_context(ID3D11DeviceContext* device_immediate_context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device_immediate_context != nullptr);
		device_immediate_context_ = device_immediate_context;
	}

	auto Dx11Renderer::bind_device_context(ComPtr<ID3D11DeviceContext> device_immediate_context) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device_immediate_context != nullptr);
		device_immediate_context_ = std::move(device_immediate_context);
	}

	auto Dx11Renderer::create() noexcept -> bool
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

	auto Dx11Renderer::destroy() noexcept -> void
	{
		// ComPtr

		std::ranges::for_each(
			textures_ | std::views::values,
			[](auto* texture_2d) noexcept -> void
			{
				texture_2d->Release();
			}
		);
	}

	auto Dx11Renderer::ready() const noexcept -> bool
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

	auto Dx11Renderer::begin_frame(const RendererContext& context) noexcept -> void
	{
		// todo: create texture 
	}

	auto Dx11Renderer::end_frame(const RendererContext& context) noexcept -> void
	{
		//
	}
}

#endif
