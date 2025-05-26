// This file is part of prometheus
// Copyright (C) 2022-2025 Life4gal <life4gal@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <gfx/renderer_dx12.hpp>

#if defined(GAL_PROMETHEUS_GFX_RENDER_DX12)

#include <prometheus/macro.hpp>

#if GAL_PROMETHEUS_COMPILER_DEBUG
#define GAL_PROMETHEUS_GFX_DEBUG
#include <source_location>
#endif

#include <print>

#include <platform/os.hpp>
#include <gfx/font.hpp>
#include <gfx/context.hpp>
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

	constexpr std::size_t gpu_handle_to_id_offset = 0x1000'0000;

	[[nodiscard]] auto id_to_gpu_handle(const texture_id_type id) noexcept -> std::size_t
	{
		return id - gpu_handle_to_id_offset;
	}

	[[nodiscard]] auto gpu_handle_to_id(const std::size_t index) noexcept -> texture_id_type
	{
		return index + gpu_handle_to_id_offset;
	}
}

namespace gal::prometheus::gfx
{
	auto Dx12Renderer::create_root_signature() noexcept -> bool
	{
		// [0] projection_matrix
		constexpr D3D12_ROOT_PARAMETER param_0
		{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = sizeof(projection_matrix_type) / sizeof(float)},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
		};
		// [1] texture
		constexpr D3D12_DESCRIPTOR_RANGE range
		{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart = 0
		};
		const D3D12_ROOT_PARAMETER param_1
		{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = {.NumDescriptorRanges = 1, .pDescriptorRanges = &range},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};
		// @see Dx12Renderer::do_present -> `command_list_->SetGraphicsRootXxx`
		const D3D12_ROOT_PARAMETER params[]{param_0, param_1};

		// Bi-linear sampling is required by default
		constexpr D3D12_STATIC_SAMPLER_DESC static_sampler_desc
		{
				.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
				.MipLODBias = .0f,
				.MaxAnisotropy = 0,
				.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS,
				.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
				.MinLOD = .0f,
				.MaxLOD = .0f,
				.ShaderRegister = 0,
				.RegisterSpace = 0,
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};

		const D3D12_ROOT_SIGNATURE_DESC root_signature_desc
		{
				.NumParameters = static_cast<UINT>(std::ranges::size(params)),
				.pParameters = params,
				.NumStaticSamplers = 1,
				.pStaticSamplers = &static_sampler_desc,
				.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				         D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
		};

		auto d3d12_dll = GetModuleHandleW(L"d3d12.dll");
		if (d3d12_dll == nullptr)
		{
			d3d12_dll = LoadLibraryW(L"d3d12.dll");
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(d3d12_dll != nullptr);
		}

		const auto serialize_root_signature_function =
				reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature")); // NOLINT(clang-diagnostic-cast-function-type-strict)
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(serialize_root_signature_function != nullptr);

		ComPtr<ID3DBlob> blob = nullptr;
		if (not check_hr_error(serialize_root_signature_function(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), nullptr)))
		{
			return false;
		}
		if (not check_hr_error(device_->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(root_signature_.GetAddressOf()))))
		{
			return false;
		}

		return true;
	}

	auto Dx12Renderer::create_pipeline_state() noexcept -> bool
	{
		// Create the vertex shader
		auto vertex_shader_blob = []() -> ComPtr<ID3DBlob>
		{
			constexpr static char shader[]{
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

			ComPtr<ID3DBlob> blob;
			if (not check_hr_error(D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, blob.GetAddressOf(), nullptr)))
			{
				return nullptr;
			}

			return blob;
		}();

		// Create the pixel shader
		auto pixel_shader_blob = []() -> ComPtr<ID3DBlob>
		{
			constexpr static char shader[]{
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

			ComPtr<ID3DBlob> blob;
			if (not check_hr_error(D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, blob.GetAddressOf(), nullptr)))
			{
				return nullptr;
			}

			return blob;
		}();

		if (vertex_shader_blob == nullptr or pixel_shader_blob == nullptr)
		{
			return false;
		}

		// Create the blending setup
		constexpr D3D12_RENDER_TARGET_BLEND_DESC render_target_blend_desc
		{
				.BlendEnable = true,
				.LogicOpEnable = false,
				.SrcBlend = D3D12_BLEND_SRC_ALPHA,
				.DestBlend = D3D12_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_ONE,
				.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA,
				.BlendOpAlpha = D3D12_BLEND_OP_ADD,
				.LogicOp = D3D12_LOGIC_OP_CLEAR,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
		};
		constexpr D3D12_BLEND_DESC blend_desc
		{
				.AlphaToCoverageEnable = FALSE,
				.IndependentBlendEnable = FALSE,
				.RenderTarget = {render_target_blend_desc}
		};

		// Create the rasterizer state
		constexpr D3D12_RASTERIZER_DESC rasterizer_desc
		{
				.FillMode = D3D12_FILL_MODE_SOLID,
				.CullMode = D3D12_CULL_MODE_NONE,
				.FrontCounterClockwise = FALSE,
				.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
				.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
				.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
				.DepthClipEnable = TRUE,
				.MultisampleEnable = FALSE,
				.AntialiasedLineEnable = FALSE,
				.ForcedSampleCount = 0,
				.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		// Create depth-stencil State
		constexpr D3D12_DEPTH_STENCIL_DESC depth_stencil_desc
		{
				.DepthEnable = FALSE,
				.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
				.StencilEnable = FALSE,
				.StencilReadMask = 0,
				.StencilWriteMask = 0,
				.FrontFace =
				{
						.StencilFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilPassOp = D3D12_STENCIL_OP_KEEP,
						.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
				},
				.BackFace =
				{
						.StencilFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
						.StencilPassOp = D3D12_STENCIL_OP_KEEP,
						.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
				}
		};

		// Create the input layout
		constexpr D3D12_INPUT_ELEMENT_DESC input_element_desc[]
		{
				{
						.SemanticName = "POSITION",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R32G32_FLOAT,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, position)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "TEXCOORD",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R32G32_FLOAT,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, uv)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "COLOR",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, color)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc
		{
				.pRootSignature = root_signature_.Get(),
				.VS = {.pShaderBytecode = vertex_shader_blob->GetBufferPointer(), .BytecodeLength = vertex_shader_blob->GetBufferSize()},
				.PS = {.pShaderBytecode = pixel_shader_blob->GetBufferPointer(), .BytecodeLength = pixel_shader_blob->GetBufferSize()},
				.DS = {.pShaderBytecode = nullptr, .BytecodeLength = 0},
				.HS = {.pShaderBytecode = nullptr, .BytecodeLength = 0},
				.GS = {.pShaderBytecode = nullptr, .BytecodeLength = 0},
				.StreamOutput = {.pSODeclaration = nullptr, .NumEntries = 0, .pBufferStrides = nullptr, .NumStrides = 0, .RasterizedStream = 0},
				.BlendState = blend_desc,
				.SampleMask = UINT_MAX,
				.RasterizerState = rasterizer_desc,
				.DepthStencilState = depth_stencil_desc,
				.InputLayout = {.pInputElementDescs = input_element_desc, .NumElements = static_cast<UINT>(std::ranges::size(input_element_desc))},
				.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
				.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
				.NumRenderTargets = 1,
				.RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM},
				.DSVFormat = {},
				.SampleDesc = {.Count = 1, .Quality = 0},
				.NodeMask = 1,
				.CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
				.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		return check_hr_error(device_->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(pipeline_state_.GetAddressOf())));
	}

	auto Dx12Renderer::create_srv_descriptor_heap(const UINT num) noexcept -> bool
	{
		const D3D12_DESCRIPTOR_HEAP_DESC desc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = num,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				.NodeMask = 0
		};
		return check_hr_error(device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(srv_descriptor_heap_.ReleaseAndGetAddressOf())));
	}

	auto Dx12Renderer::upload_texture(
		const std::size_t index,
		const Texture::data_view_type data,
		const Texture::size_type size,
		const bool record_resource
	) noexcept -> texture_id_type
	{
		constexpr D3D12_HEAP_PROPERTIES heap_properties{
				.Type = D3D12_HEAP_TYPE_DEFAULT,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
		};

		const D3D12_RESOURCE_DESC resource_desc{
				.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
				.Alignment = 0,
				.Width = static_cast<UINT64>(size.width),
				.Height = static_cast<UINT>(size.height),
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		ComPtr<ID3D12Resource> texture_2d;
		if (not check_hr_error(
			device_->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(texture_2d.GetAddressOf())
			)
		))
		{
			return invalid_texture_id;
		}

		const auto upload_pitch = (static_cast<UINT>(size.width * 4) + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		const auto upload_size = static_cast<UINT>(size.height) * upload_pitch;

		constexpr D3D12_HEAP_PROPERTIES upload_heap_properties{
				.Type = D3D12_HEAP_TYPE_UPLOAD,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
		};

		const D3D12_RESOURCE_DESC upload_resource_desc{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = 0,
				.Width = static_cast<UINT64>(upload_size),
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_UNKNOWN,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		ComPtr<ID3D12Resource> upload_buffer;
		check_hr_error(
			device_->CreateCommittedResource(
				&upload_heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&upload_resource_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(upload_buffer.GetAddressOf())
			)
		);

		void* mapped_data = nullptr;
		const D3D12_RANGE range{.Begin = 0, .End = upload_size};
		if (not check_hr_error(upload_buffer->Map(0, &range, &mapped_data)))
		{
			return invalid_texture_id;
		}
		for (UINT i = 0; i < static_cast<UINT>(size.height); ++i)
		{
			auto* dest = static_cast<std::uint8_t*>(mapped_data) + static_cast<std::ptrdiff_t>(upload_pitch * i);
			auto* source = reinterpret_cast<const std::uint8_t*>(data.data()) + static_cast<std::ptrdiff_t>(size.width * i * 4);
			const auto length = size.width * 4;
			std::memcpy(dest, source, length);
		}
		upload_buffer->Unmap(0, &range);

		const D3D12_TEXTURE_COPY_LOCATION source_location{
				.pResource = upload_buffer.Get(),
				.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
				.PlacedFootprint =
				{
						.Offset = 0,
						.Footprint =
						{
								.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
								.Width = static_cast<UINT>(size.width),
								.Height = static_cast<UINT>(size.height),
								.Depth = 1,
								.RowPitch = upload_pitch
						}
				}
		};

		const D3D12_TEXTURE_COPY_LOCATION dest_location{
				.pResource = texture_2d.Get(),
				.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				.SubresourceIndex = 0
		};

		const D3D12_RESOURCE_BARRIER barrier{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition =
				{
						.pResource = texture_2d.Get(),
						.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
						.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				}
		};

		ComPtr<ID3D12CommandAllocator> command_allocator;
		if (not check_hr_error(
			device_->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(command_allocator.GetAddressOf())
			)
		))
		{
			return invalid_texture_id;
		}

		ComPtr<ID3D12GraphicsCommandList> command_list;
		if (not check_hr_error(
			device_->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocator.Get(),
				nullptr,
				IID_PPV_ARGS(command_list.GetAddressOf())
			)
		))
		{
			return invalid_texture_id;
		}

		command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, nullptr);
		command_list->ResourceBarrier(1, &barrier);
		if (not check_hr_error(command_list->Close()))
		{
			return invalid_texture_id;
		}

		constexpr D3D12_COMMAND_QUEUE_DESC command_queue_desc{
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = 0,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 1
		};

		ComPtr<ID3D12CommandQueue> command_queue;
		if (not check_hr_error(
			device_->CreateCommandQueue(
				&command_queue_desc,
				IID_PPV_ARGS(command_queue.GetAddressOf())
			)
		))
		{
			return invalid_texture_id;
		}

		ID3D12CommandList* command_lists[]{command_list.Get()};
		command_queue->ExecuteCommandLists(1, command_lists);

		ComPtr<ID3D12Fence> fence;
		if (not check_hr_error(
			device_->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(fence.GetAddressOf())
			)
		))
		{
			return invalid_texture_id;
		}

		constexpr UINT64 fence_value = 1;
		if (not check_hr_error(command_queue->Signal(fence.Get(), fence_value)))
		{
			return invalid_texture_id;
		}
		if (fence->GetCompletedValue() < fence_value)
		{
			HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (not check_hr_error(fence->SetEventOnCompletion(fence_value, event)))
			{
				return invalid_texture_id;
			}
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}

		// Create texture view
		const D3D12_SHADER_RESOURCE_VIEW_DESC resource_view_desc{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
				.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Texture2D = {.MostDetailedMip = 0, .MipLevels = resource_desc.MipLevels, .PlaneSlice = 0, .ResourceMinLODClamp = .0f}
		};

		const auto increment_size = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		auto cpu_handle = srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();
		auto gpu_handle = srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart();

		cpu_handle.ptr += index * increment_size;
		gpu_handle.ptr += index * increment_size;

		device_->CreateShaderResourceView(texture_2d.Get(), &resource_view_desc, cpu_handle);

		if (record_resource)
		{
			textures_[index] = {.resource = texture_2d, .cpu = cpu_handle, .gpu = gpu_handle};
		}
		else
		{
			// ComPtr
			texture_2d = nullptr;
		}

		return gpu_handle_to_id(index);
	}

	Dx12Renderer::Dx12Renderer() noexcept
		: device_{nullptr},
		  command_list_{nullptr},
		  root_signature_{nullptr},
		  pipeline_state_{nullptr},
		  srv_descriptor_heap_{nullptr},
		  srv_max_size_{8},
		  render_buffer_{} {}

	Dx12Renderer::Dx12Renderer(ID3D12Device* device, ID3D12GraphicsCommandList* command_list) noexcept
	{
		bind_device(device);
		bind_command_list(command_list);
	}

	Dx12Renderer::Dx12Renderer(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> command_list) noexcept
	{
		bind_device(std::move(device));
		bind_command_list(std::move(command_list));
	}

	auto Dx12Renderer::bind_device(ID3D12Device* device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = device;
	}

	auto Dx12Renderer::bind_device(ComPtr<ID3D12Device> device) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(device != nullptr);
		device_ = std::move(device);
	}

	auto Dx12Renderer::bind_command_list(ID3D12GraphicsCommandList* command_list) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list != nullptr);
		command_list_ = command_list;
	}

	auto Dx12Renderer::bind_command_list(ComPtr<ID3D12GraphicsCommandList> command_list) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(command_list != nullptr);
		command_list_ = std::move(command_list);
	}

	auto Dx12Renderer::do_create() noexcept -> bool
	{
		if (not create_root_signature())
		{
			return false;
		}
		if (not create_pipeline_state())
		{
			return false;
		}
		if (not create_srv_descriptor_heap(srv_max_size_))
		{
			return false;
		}
		textures_.resize(srv_max_size_);

		return true;
	}

	auto Dx12Renderer::do_destroy() noexcept -> void
	{
		// ComPtr
		textures_.clear();

		render_buffer_.vertex = nullptr;
		render_buffer_.index = nullptr;

		// command_list_->ClearState(pipeline_state_.Get());
		// std::ignore = command_list_->Close();

		srv_descriptor_heap_ = nullptr;
		pipeline_state_ = nullptr;
		root_signature_ = nullptr;

		command_list_ = nullptr;
		device_ = nullptr;
	}

	auto Dx12Renderer::do_ready() const noexcept -> bool
	{
		if (device_ == nullptr or command_list_ == nullptr)
		{
			return false;
		}

		if (root_signature_ == nullptr or pipeline_state_ == nullptr or srv_descriptor_heap_ == nullptr)
		{
			return false;
		}

		return true;
	}

	auto Dx12Renderer::do_create_texture(const Texture::data_view_type data, const Texture::size_type size) noexcept -> texture_id_type
	{
		auto it = std::ranges::find(textures_, nullptr, &texture_type::resource);
		if (it == textures_.end())
		{
			// full, expand heap
			const auto old_size = srv_max_size_;
			const auto new_size = static_cast<UINT>(static_cast<float>(srv_max_size_) * 1.5f);

			srv_max_size_ = new_size;
			textures_.resize(srv_max_size_);
			it = textures_.begin() + old_size;

			const auto old_heap = srv_descriptor_heap_;

			const auto create_srv_descriptor_heap_result = create_srv_descriptor_heap(srv_max_size_);
			GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(create_srv_descriptor_heap_result);

			const auto source = old_heap->GetCPUDescriptorHandleForHeapStart();
			const auto dest = srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart();

			device_->CopyDescriptorsSimple(
				old_size,
				dest,
				source,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);

			const auto increment_size = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			const auto new_cpu_begin = srv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart().ptr;
			const auto new_gpu_begin = srv_descriptor_heap_->GetGPUDescriptorHandleForHeapStart().ptr;

			for (std::size_t i = 0; i < old_size; ++i)
			{
				if (auto& [resource, cpu, gpu] = textures_[i]; resource != nullptr)
				{
					cpu.ptr = new_cpu_begin + i * increment_size;
					gpu.ptr = new_gpu_begin + i * increment_size;
				}
			}
		}

		const auto index = std::ranges::distance(textures_.begin(), it);

		return upload_texture(index, data, size);
	}

	auto Dx12Renderer::do_update_texture(const Texture& texture) noexcept -> void
	{
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.uploaded(), "Create texture first!");
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.dirty(), "No need to update texture!");

		const auto index = id_to_gpu_handle(texture.id());
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < srv_max_size_);
		auto& texture_gpu = textures_[index];
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_gpu.resource != nullptr);

		// todo
		do_destroy_texture(texture.id());
		std::ignore = upload_texture(index, texture.data(), texture.size());
	}

	auto Dx12Renderer::do_destroy_texture(const texture_id_type texture_id) noexcept -> void
	{
		const auto index = id_to_gpu_handle(texture_id);
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(index < srv_max_size_);

		auto& texture = textures_[index];
		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture.resource != nullptr);
		// ComPtr
		texture.resource = nullptr;
	}

	auto Dx12Renderer::do_present(const RenderContext& renderer_context, const rect_type& display_area) noexcept -> void
	{
		const auto all_render_data = renderer_context.render_data();
		// const auto [display_x, display_y] = display_area.point;
		const auto [display_width, display_height] = display_area.extent;

		const auto [total_vertex_count, total_index_count] = [&]() noexcept
		{
			struct sum
			{
				UINT vertex;
				UINT index;
			};

			return std::ranges::fold_left(
				all_render_data,
				sum{.vertex = 0, .index = 0},
				[](const sum s, const RenderData& render_data) noexcept -> sum
				{
					const auto vertex_list = render_data.vertex_list.get();
					const auto index_list = render_data.index_list.get();

					return {.vertex = s.vertex + static_cast<UINT>(vertex_list.size()), .index = s.index + static_cast<UINT>(index_list.size())};
				}
			);
		}();

		frame_resource_index_ += 1;
		const auto this_frame_index = frame_resource_index_ % num_frames_in_flight;
		auto& this_frame = frame_resource_[this_frame_index];
		auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = this_frame;

		constexpr D3D12_HEAP_PROPERTIES heap_properties{
				.Type = D3D12_HEAP_TYPE_UPLOAD,
				.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
				.CreationNodeMask = 0,
				.VisibleNodeMask = 0
		};
		// Create and grow vertex/index buffers if needed
		if (this_frame_vertex_buffer == nullptr or this_frame_vertex_count < total_vertex_count)
		{
			// todo: grow factor
			this_frame_vertex_count = total_vertex_count + 5000;

			const D3D12_RESOURCE_DESC resource_desc{
					.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
					.Alignment = 0,
					.Width = this_frame_vertex_count * sizeof(vertex_type),
					.Height = 1,
					.DepthOrArraySize = 1,
					.MipLevels = 1,
					.Format = DXGI_FORMAT_UNKNOWN,
					.SampleDesc = {.Count = 1, .Quality = 0},
					.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
					.Flags = D3D12_RESOURCE_FLAG_NONE
			};
			check_hr_error(
				device_->CreateCommittedResource(
					&heap_properties,
					D3D12_HEAP_FLAG_NONE,
					&resource_desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(this_frame_vertex_buffer.ReleaseAndGetAddressOf())
				)
			);
		}
		if (this_frame_index_buffer == nullptr or this_frame_index_count < total_index_count)
		{
			// todo: grow factor
			this_frame_index_count = total_index_count + 10000;

			const D3D12_RESOURCE_DESC resource_desc{
					.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
					.Alignment = 0,
					.Width = this_frame_index_count * sizeof(index_type),
					.Height = 1,
					.DepthOrArraySize = 1,
					.MipLevels = 1,
					.Format = DXGI_FORMAT_UNKNOWN,
					.SampleDesc = {.Count = 1, .Quality = 0},
					.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
					.Flags = D3D12_RESOURCE_FLAG_NONE
			};
			check_hr_error(
				device_->CreateCommittedResource(
					&heap_properties,
					D3D12_HEAP_FLAG_NONE,
					&resource_desc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(this_frame_index_buffer.ReleaseAndGetAddressOf())
				)
			);
		}

		// Upload vertex/index data into a single contiguous GPU buffer
		{
			void* mapped_vertex_resource;
			void* mapped_index_resource;
			constexpr D3D12_RANGE vertex_range{.Begin = 0, .End = 0};
			constexpr D3D12_RANGE index_range{.Begin = 0, .End = 0};
			check_hr_error(this_frame_vertex_buffer->Map(0, &vertex_range, &mapped_vertex_resource));
			check_hr_error(this_frame_index_buffer->Map(0, &index_range, &mapped_index_resource));

			auto* mapped_vertex = static_cast<vertex_type*>(mapped_vertex_resource);
			auto* mapped_index = static_cast<index_type*>(mapped_index_resource);

			UINT vertex_offset = 0;
			UINT index_offset = 0;

			std::ranges::for_each(
				all_render_data,
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

			this_frame_vertex_buffer->Unmap(0, &vertex_range);
			this_frame_index_buffer->Unmap(0, &index_range);
		}

		// Setup orthographic projection matrix into our constant buffer
		projection_matrix_type projection_matrix;
		{
			constexpr auto left = 0.f;
			const auto right = display_width;
			constexpr auto top = 0.f;
			const auto bottom = display_height;

			const float mvp[4][4]{
					{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
					{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
					{0.0f, 0.0f, 0.5f, 0.0f},
					{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
			};
			static_assert(sizeof(mvp) == sizeof(projection_matrix_type));
			std::memcpy(projection_matrix, mvp, sizeof(projection_matrix_type));
		}

		// Setup viewport
		{
			const D3D12_VIEWPORT viewport{
					.TopLeftX = .0f,
					.TopLeftY = .0f,
					.Width = static_cast<FLOAT>(display_width),
					.Height = static_cast<FLOAT>(display_height),
					.MinDepth = .0f,
					.MaxDepth = 1.f
			};
			command_list_->RSSetViewports(1, &viewport);
		}

		// Bind shader/vertex buffers, root signature and pipeline state
		{
			ID3D12DescriptorHeap* descriptor_heaps[]{srv_descriptor_heap_.Get()};
			command_list_->SetDescriptorHeaps(1, descriptor_heaps);

			command_list_->SetGraphicsRootSignature(root_signature_.Get());
			command_list_->SetGraphicsRoot32BitConstants(0, sizeof(projection_matrix_type) / sizeof(float), &projection_matrix, 0);

			command_list_->SetPipelineState(pipeline_state_.Get());

			const D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{
					.BufferLocation = this_frame_vertex_buffer->GetGPUVirtualAddress(),
					.SizeInBytes = this_frame_vertex_count * static_cast<UINT>(sizeof(vertex_type)),
					.StrideInBytes = sizeof(vertex_type)
			};
			command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view);

			const D3D12_INDEX_BUFFER_VIEW index_buffer_view{
					.BufferLocation = this_frame_index_buffer->GetGPUVirtualAddress(),
					.SizeInBytes = this_frame_index_count * static_cast<UINT>(sizeof(index_type)),
					// ReSharper disable once CppUnreachableCode
					.Format = sizeof(index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
			};
			command_list_->IASetIndexBuffer(&index_buffer_view);
			command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		// Setup blend factor
		constexpr float blend_factor[4]{.0f, .0f, .0f, .0f};
		command_list_->OMSetBlendFactor(blend_factor);

		UINT total_index_offset = 0;
		std::ranges::for_each(
			all_render_data,
			[this, &total_index_offset](const RenderData& render_data) noexcept -> void
			{
				const auto vertex_list = render_data.vertex_list.get();
				const auto index_list = render_data.index_list.get();

				for (const auto& command_list = render_data.command_list.get();
				     const auto& [clip_rect, texture_id, index_offset, element_count]: command_list)
				{
					const auto [point, extent] = clip_rect;
					const D3D12_RECT rect
					{
							static_cast<LONG>(point.x),
							static_cast<LONG>(point.y),
							static_cast<LONG>(point.x + extent.width),
							static_cast<LONG>(point.y + extent.height)
					};
					command_list_->RSSetScissorRects(1, &rect);

					GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture_id != invalid_texture_id);
					const auto index = id_to_gpu_handle(texture_id);
					const auto& texture = textures_[index];
					command_list_->SetGraphicsRootDescriptorTable(1, texture.gpu);

					const auto this_index_offset = static_cast<UINT>(total_index_offset + index_offset);
					command_list_->DrawIndexedInstanced(static_cast<UINT>(element_count), 1, this_index_offset, 0, 0);
				}

				total_index_offset += static_cast<UINT>(index_list.size());
			}
		);
	}
}

#endif
