#include "def.hpp"
#include "dx_error_handler.hpp"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
	using Microsoft::WRL::ComPtr;

	using namespace gal::prometheus;
}

extern const std::size_t num_frames_in_flight;

extern ComPtr<ID3D12Device> g_device;
extern ComPtr<ID3D12GraphicsCommandList> g_command_list;

extern int g_window_width;
extern int g_window_height;

extern double g_last_time;
extern std::uint64_t g_frame_count;
extern float g_fps;

extern std::shared_ptr<draw::DrawListSharedData> g_draw_list_shared_data;
extern draw::DrawList g_draw_list;

namespace
{
	struct render_buffer_type
	{
		ComPtr<ID3D12Resource> index;
		UINT index_count;
		ComPtr<ID3D12Resource> vertex;
		UINT vertex_count;
	};

	// note: overflow(max + 1 => 0)
	UINT g_frame_resource_index = (std::numeric_limits<UINT>::max)();
	// render_buffer_type g_frame_resource[num_frames_in_flight] = {};
	// num_frames_in_flight < 16
	render_buffer_type g_frame_resource[16] = {};

	ComPtr<ID3D12RootSignature> g_root_signature = nullptr;
	ComPtr<ID3D12PipelineState> g_pipeline_state = nullptr;

	// (default) font + additional picture
	constexpr UINT num_shader_resource_view_descriptor_heap = 2;
	ComPtr<ID3D12DescriptorHeap> g_shader_resource_view_descriptor_heap = nullptr;

	ComPtr<ID3D12Resource> g_font_resource = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE g_font_handle = {.ptr = 0};

	ComPtr<ID3D12Resource> g_additional_picture_resource = nullptr;
	D3D12_GPU_DESCRIPTOR_HANDLE g_additional_picture_handle = {.ptr = 0};

	[[nodiscard]] auto load_texture(
		const std::uint8_t* texture_data,
		const std::uint32_t texture_width,
		const std::uint32_t texture_height,
		ComPtr<ID3D12DescriptorHeap>& in_descriptor_heap,
		const SIZE_T in_resource_index,
		D3D12_GPU_DESCRIPTOR_HANDLE& out_handle,
		ComPtr<ID3D12Resource>& out_resource
	) -> bool
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
				.Width = static_cast<UINT64>(texture_width),
				.Height = static_cast<UINT>(texture_height),
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
				.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		ComPtr<ID3D12Resource> texture;
		check_hr_error(
			g_device->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(texture.GetAddressOf())
			)
		);

		const auto upload_pitch = (static_cast<UINT>(texture_width * 4) + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		const auto upload_size = static_cast<UINT>(texture_height) * upload_pitch;

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
			g_device->CreateCommittedResource(
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
		check_hr_error(upload_buffer->Map(0, &range, &mapped_data));
		for (UINT i = 0; i < static_cast<UINT>(texture_height); ++i)
		{
			auto* dest = static_cast<std::uint8_t*>(mapped_data) + static_cast<std::ptrdiff_t>(upload_pitch * i);
			auto* source = reinterpret_cast<const std::uint8_t*>(texture_data) + static_cast<std::ptrdiff_t>(texture_width * i * 4);
			const auto size = texture_width * 4;
			std::memcpy(dest, source, size);
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
								.Width = static_cast<UINT>(texture_width),
								.Height = static_cast<UINT>(texture_height),
								.Depth = 1,
								.RowPitch = upload_pitch
						}
				}
		};

		const D3D12_TEXTURE_COPY_LOCATION dest_location{
				.pResource = texture.Get(),
				.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
				.SubresourceIndex = 0
		};

		const D3D12_RESOURCE_BARRIER barrier{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition =
				{
						.pResource = texture.Get(),
						.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
						.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
				}
		};

		ComPtr<ID3D12CommandAllocator> command_allocator;
		check_hr_error(
			g_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(command_allocator.GetAddressOf())
			)
		);

		ComPtr<ID3D12GraphicsCommandList> command_list;
		check_hr_error(
			g_device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocator.Get(),
				nullptr,
				IID_PPV_ARGS(command_list.GetAddressOf())
			)
		);

		command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, nullptr);
		command_list->ResourceBarrier(1, &barrier);
		check_hr_error(command_list->Close());

		constexpr D3D12_COMMAND_QUEUE_DESC command_queue_desc{
				.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
				.Priority = 0,
				.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
				.NodeMask = 1
		};

		ComPtr<ID3D12CommandQueue> command_queue;
		check_hr_error(
			g_device->CreateCommandQueue(
				&command_queue_desc,
				IID_PPV_ARGS(command_queue.GetAddressOf())
			)
		);

		ID3D12CommandList* command_lists[]{command_list.Get()};
		command_queue->ExecuteCommandLists(1, command_lists);

		ComPtr<ID3D12Fence> fence;
		check_hr_error(
			g_device->CreateFence(
				0,
				D3D12_FENCE_FLAG_NONE,
				IID_PPV_ARGS(fence.GetAddressOf())
			)
		);

		constexpr UINT64 fence_value = 1;
		check_hr_error(command_queue->Signal(fence.Get(), fence_value));
		if (fence->GetCompletedValue() < fence_value)
		{
			HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			check_hr_error(fence->SetEventOnCompletion(fence_value, event));
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

		// note:
		// We set `NumDescriptors` to 2 at `create_device => g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(g_shader_resource_view_descriptor_heap.GetAddressOf()))`.
		// where g_shader_resource_view_descriptor_heap[0] is used for our default font texture, and g_shader_resource_view_descriptor_heap[1] is used for the additional image
		const auto increment_size = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		auto picture_cpu_handle = in_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		auto picture_gpu_handle = in_descriptor_heap->GetGPUDescriptorHandleForHeapStart();

		picture_cpu_handle.ptr += in_resource_index * increment_size;
		picture_gpu_handle.ptr += in_resource_index * increment_size;

		g_device->CreateShaderResourceView(texture.Get(), &resource_view_desc, picture_cpu_handle);

		out_handle = picture_gpu_handle;
		out_resource = texture;

		return true;
	}
}

auto prometheus_init() -> void
{
	print_time();

	using functional::operators::operator|;
	g_draw_list.draw_list_flag(draw::DrawListFlag::ANTI_ALIASED_LINE | draw::DrawListFlag::ANTI_ALIASED_FILL);
	g_draw_list.shared_data(g_draw_list_shared_data);

	// Create the root signature
	{
		// [0] projection_matrix
		constexpr D3D12_ROOT_PARAMETER param_0
		{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
				.Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = sizeof(d3d_projection_matrix_type) / sizeof(float)},
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
		// @see `prometheus_draw` -> `g_command_list->SetGraphicsRootXxx`
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

		static auto d3d12_dll = GetModuleHandleW(L"d3d12.dll");
		if (d3d12_dll == nullptr)
		{
			d3d12_dll = LoadLibraryW(L"d3d12.dll");
			assert(d3d12_dll);
		}

		auto serialize_root_signature_function =
				reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature")); // NOLINT(clang-diagnostic-cast-function-type-strict)
		assert(serialize_root_signature_function);

		ComPtr<ID3DBlob> blob = nullptr;
		check_hr_error(serialize_root_signature_function(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), nullptr));
		check_hr_error(g_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(g_root_signature.GetAddressOf())));
	}

	// Create the pipeline state
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
			if (not check_hr_error<false>(D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, blob.GetAddressOf(), nullptr)))
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
			if (not check_hr_error<false>(D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, blob.GetAddressOf(), nullptr)))
			{
				return nullptr;
			}

			return blob;
		}();

		assert(vertex_shader_blob and pixel_shader_blob);

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
						.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, position)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "TEXCOORD",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R32G32_FLOAT,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, uv)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
				{
						.SemanticName = "COLOR",
						.SemanticIndex = 0,
						.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
						.InputSlot = 0,
						.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, color)),
						.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
						.InstanceDataStepRate = 0
				},
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc
		{
				.pRootSignature = g_root_signature.Get(),
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

		check_hr_error(g_device->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(g_pipeline_state.GetAddressOf())));
	}

	// Create the shader resource view descriptor heap
	{
		constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
				.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
				.NumDescriptors = num_shader_resource_view_descriptor_heap,
				.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
				.NodeMask = 0
		};
		check_hr_error(g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(g_shader_resource_view_descriptor_heap.GetAddressOf())));
	}

	// Load default font texture
	{
		const auto& default_font = g_draw_list_shared_data->get_default_font();

		const auto font_data = default_font.texture_data.get();
		assert(font_data);

		const auto font_width = default_font.texture_size.width;
		const auto font_height = default_font.texture_size.height;

		const auto load_font_texture_result = load_texture(
			reinterpret_cast<const std::uint8_t*>(font_data),
			font_width,
			font_height,
			g_shader_resource_view_descriptor_heap,
			0,
			g_font_handle,
			g_font_resource
		);
		assert(load_font_texture_result);

		g_draw_list_shared_data->get_default_font().texture_id = static_cast<draw::font_type::texture_id_type>(g_font_handle.ptr);
	}

	// Load additional picture texture
	{
		int image_width;
		int image_height;

		auto* data = stbi_load(ASSETS_PATH_PIC, &image_width, &image_height, nullptr, 4);
		assert(data);

		const auto load_additional_texture_result = load_texture(
			data,
			image_width,
			image_height,
			g_shader_resource_view_descriptor_heap,
			1,
			g_additional_picture_handle,
			g_additional_picture_resource
		);
		assert(load_additional_texture_result);

		stbi_image_free(data);
	}
}

auto prometheus_new_frame() -> void //
{
	g_draw_list.reset();
	g_draw_list.push_clip_rect({0, 0}, {static_cast<float>(g_window_width), static_cast<float>(g_window_height)}, false);
}

auto prometheus_render() -> void
{
	g_draw_list.text(24.f, {10, 10}, primitive::colors::blue, std::format("FPS: {:.3f}", g_fps));

	g_draw_list.text(24.f, {50, 50}, primitive::colors::red, "The quick brown fox jumps over the lazy dog.\nHello world!\n你好世界!\n");

	g_draw_list.line({200, 100}, {200, 300}, primitive::colors::red);
	g_draw_list.line({100, 200}, {300, 200}, primitive::colors::red);

	g_draw_list.rect({100, 100}, {300, 300}, primitive::colors::blue);
	g_draw_list.rect({150, 150}, {250, 250}, primitive::colors::blue, 30);

	g_draw_list.triangle({120, 120}, {120, 150}, {150, 120}, primitive::colors::green);
	g_draw_list.triangle_filled({130, 130}, {130, 150}, {150, 130}, primitive::colors::red);

	g_draw_list.rect_filled({300, 100}, {400, 200}, primitive::colors::pink);
	g_draw_list.rect_filled({300, 200}, {400, 300}, primitive::colors::pink, 20);
	g_draw_list.rect_filled({300, 300}, {400, 400}, primitive::colors::pink, primitive::colors::gold, primitive::colors::azure, primitive::colors::lavender);

	g_draw_list.quadrilateral({100, 500}, {200, 500}, {250, 550}, {50, 550}, primitive::colors::red);
	g_draw_list.quadrilateral_filled({100, 500}, {200, 500}, {250, 450}, {50, 450}, primitive::colors::red);

	g_draw_list.circle({100, 600}, 50, primitive::colors::green);
	g_draw_list.circle({200, 600}, 50, primitive::colors::red, 8);
	g_draw_list.circle_filled({100, 700}, 50, primitive::colors::green);
	g_draw_list.circle_filled({200, 700}, 50, primitive::colors::red, 8);

	g_draw_list.ellipse({500, 100}, {50, 70}, std::numbers::pi_v<float> * .35f, primitive::colors::red, 8);
	g_draw_list.ellipse_filled({500, 200}, {50, 70}, std::numbers::pi_v<float> * -.35f, primitive::colors::red, 8);
	g_draw_list.ellipse({600, 100}, {50, 70}, std::numbers::pi_v<float> * .35f, primitive::colors::red, 16);
	g_draw_list.ellipse_filled({600, 200}, {50, 70}, std::numbers::pi_v<float> * -.35f, primitive::colors::red, 16);
	g_draw_list.ellipse({700, 100}, {50, 70}, std::numbers::pi_v<float> * .35f, primitive::colors::red, 24);
	g_draw_list.ellipse_filled({700, 200}, {50, 70}, std::numbers::pi_v<float> * -.35f, primitive::colors::red, 24);
	g_draw_list.ellipse({800, 100}, {50, 70}, std::numbers::pi_v<float> * .35f, primitive::colors::red);
	g_draw_list.ellipse_filled({800, 200}, {50, 70}, std::numbers::pi_v<float> * -.35f, primitive::colors::red);

	g_draw_list.circle_filled({500, 300}, 5, primitive::colors::red);
	g_draw_list.circle_filled({600, 350}, 5, primitive::colors::red);
	g_draw_list.circle_filled({450, 500}, 5, primitive::colors::red);
	g_draw_list.circle_filled({550, 550}, 5, primitive::colors::red);
	g_draw_list.bezier_cubic({500, 300}, {600, 350}, {450, 500}, {550, 550}, primitive::colors::green);

	g_draw_list.circle_filled({600, 300}, 5, primitive::colors::red);
	g_draw_list.circle_filled({700, 350}, 5, primitive::colors::red);
	g_draw_list.circle_filled({550, 500}, 5, primitive::colors::red);
	g_draw_list.circle_filled({650, 550}, 5, primitive::colors::red);
	g_draw_list.bezier_cubic({600, 300}, {700, 350}, {550, 500}, {650, 550}, primitive::colors::green, 5);

	g_draw_list.circle_filled({500, 600}, 5, primitive::colors::red);
	g_draw_list.circle_filled({600, 650}, 5, primitive::colors::red);
	g_draw_list.circle_filled({450, 800}, 5, primitive::colors::red);
	g_draw_list.bezier_quadratic({500, 600}, {600, 650}, {450, 800}, primitive::colors::green);

	g_draw_list.circle_filled({600, 600}, 5, primitive::colors::red);
	g_draw_list.circle_filled({700, 650}, 5, primitive::colors::red);
	g_draw_list.circle_filled({550, 800}, 5, primitive::colors::red);
	g_draw_list.bezier_quadratic({600, 600}, {700, 650}, {550, 800}, primitive::colors::green, 5);

	// push bound
	// [800,350] => [1000, 550] (200 x 200)
	g_draw_list.push_clip_rect({800, 350}, {1000, 550}, true);
	g_draw_list.rect({800, 350}, {1000, 550}, primitive::colors::red);
	// out-of-bound
	g_draw_list.triangle_filled({700, 250}, {900, 400}, {850, 450}, primitive::colors::green);
	// in-bound
	g_draw_list.triangle_filled({900, 450}, {1000, 450}, {950, 550}, primitive::colors::blue);
	// pop bound
	g_draw_list.pop_clip_rect();

	g_draw_list.triangle_filled({800, 450}, {700, 750}, {850, 800}, primitive::colors::gold);

	// font texture
	g_draw_list.image(g_draw_list_shared_data->get_default_font().texture_id, {900, 20, 1200, 320});
	g_draw_list.image_rounded(static_cast<draw::DrawList::texture_id_type>(g_additional_picture_handle.ptr), {900, 350, 1200, 650}, 10);

	#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
	g_draw_list.bind_debug_info();
	#endif
}

auto prometheus_draw() -> void
{
	g_frame_resource_index += 1;
	const auto this_frame_index = g_frame_resource_index % num_frames_in_flight;
	auto& this_frame = g_frame_resource[this_frame_index];
	auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = this_frame;

	const auto command_list = g_draw_list.command_list();
	const auto vertex_list = g_draw_list.vertex_list();
	const auto index_list = g_draw_list.index_list();

	constexpr D3D12_HEAP_PROPERTIES heap_properties{
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 0,
			.VisibleNodeMask = 0
	};
	// Create and grow vertex/index buffers if needed
	if (this_frame_vertex_buffer == nullptr or this_frame_vertex_count < vertex_list.size())
	{
		// todo: grow factor
		this_frame_vertex_count = static_cast<UINT>(vertex_list.size()) + 5000;

		const D3D12_RESOURCE_DESC resource_desc{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = 0,
				.Width = this_frame_vertex_count * sizeof(d3d_vertex_type),
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_UNKNOWN,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_NONE
		};
		check_hr_error(
			g_device->CreateCommittedResource(
				&heap_properties,
				D3D12_HEAP_FLAG_NONE,
				&resource_desc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(this_frame_vertex_buffer.ReleaseAndGetAddressOf())
			)
		);
	}
	if (this_frame_index_buffer == nullptr or this_frame_index_count < index_list.size())
	{
		// todo: grow factor
		this_frame_index_count = static_cast<UINT>(index_list.size()) + 10000;

		const D3D12_RESOURCE_DESC resource_desc{
				.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
				.Alignment = 0,
				.Width = this_frame_index_count * sizeof(d3d_index_type),
				.Height = 1,
				.DepthOrArraySize = 1,
				.MipLevels = 1,
				.Format = DXGI_FORMAT_UNKNOWN,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
				.Flags = D3D12_RESOURCE_FLAG_NONE
		};
		check_hr_error(
			g_device->CreateCommittedResource(
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

		auto* mapped_vertex = static_cast<d3d_vertex_type*>(mapped_vertex_resource);
		auto* mapped_index = static_cast<d3d_index_type*>(mapped_index_resource);

		std::ranges::transform(
			vertex_list,
			mapped_vertex,
			[](const draw::DrawList::vertex_type& vertex) -> d3d_vertex_type
			{
				// return {
				// 		.position = {vertex.position.x, vertex.position.y},
				// 		.uv = {vertex.uv.x, vertex.uv.y},
				// 		.color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)
				// };
				return std::bit_cast<d3d_vertex_type>(vertex);
			}
		);
		std::ranges::copy(index_list, mapped_index);

		this_frame_vertex_buffer->Unmap(0, &vertex_range);
		this_frame_index_buffer->Unmap(0, &index_range);
	}

	// Setup orthographic projection matrix into our constant buffer
	d3d_projection_matrix_type projection_matrix;
	{
		constexpr auto left = .0f;
		const auto right = static_cast<float>(g_window_width);
		constexpr auto top = .0f;
		const auto bottom = static_cast<float>(g_window_height);

		const float mvp[4][4]{
				{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
				{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
				{0.0f, 0.0f, 0.5f, 0.0f},
				{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
		};
		static_assert(sizeof(mvp) == sizeof(d3d_projection_matrix_type));
		std::memcpy(projection_matrix, mvp, sizeof(d3d_projection_matrix_type));
	}

	// Setup viewport
	{
		const D3D12_VIEWPORT viewport{
				.TopLeftX = .0f,
				.TopLeftY = .0f,
				.Width = static_cast<FLOAT>(g_window_width),
				.Height = static_cast<FLOAT>(g_window_height),
				.MinDepth = .0f,
				.MaxDepth = 1.f
		};
		g_command_list->RSSetViewports(1, &viewport);
	}

	// Bind shader/vertex buffers, root signature and pipeline state
	{
		ID3D12DescriptorHeap* descriptor_heaps[]{g_shader_resource_view_descriptor_heap.Get()};
		g_command_list->SetDescriptorHeaps(1, descriptor_heaps);

		g_command_list->SetGraphicsRootSignature(g_root_signature.Get());
		g_command_list->SetGraphicsRoot32BitConstants(0, sizeof(d3d_projection_matrix_type) / sizeof(float), &projection_matrix, 0);

		g_command_list->SetPipelineState(g_pipeline_state.Get());

		const D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{
				.BufferLocation = this_frame_vertex_buffer->GetGPUVirtualAddress(),
				.SizeInBytes = this_frame_vertex_count * static_cast<UINT>(sizeof(d3d_vertex_type)),
				.StrideInBytes = sizeof(d3d_vertex_type)
		};
		g_command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);

		const D3D12_INDEX_BUFFER_VIEW index_buffer_view{
				.BufferLocation = this_frame_index_buffer->GetGPUVirtualAddress(),
				.SizeInBytes = this_frame_index_count * static_cast<UINT>(sizeof(d3d_index_type)),
				// ReSharper disable once CppUnreachableCode
				.Format = sizeof(d3d_index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
		};
		g_command_list->IASetIndexBuffer(&index_buffer_view);
		g_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	// Setup blend factor
	constexpr float blend_factor[4]{.0f, .0f, .0f, .0f};
	g_command_list->OMSetBlendFactor(blend_factor);

	for (const auto& [clip_rect, texture, index_offset, element_count]: command_list)
	{
		const auto [point, extent] = clip_rect;
		const D3D12_RECT rect{static_cast<LONG>(point.x), static_cast<LONG>(point.y), static_cast<LONG>(point.x + extent.width), static_cast<LONG>(point.y + extent.height)};
		g_command_list->RSSetScissorRects(1, &rect);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture != 0, "push_texture_id when create texture view");
		const D3D12_GPU_DESCRIPTOR_HANDLE texture_handle{.ptr = static_cast<UINT64>(texture)};
		g_command_list->SetGraphicsRootDescriptorTable(1, texture_handle);

		g_command_list->DrawIndexedInstanced(static_cast<UINT>(element_count), 1, static_cast<UINT>(index_offset), 0, 0);
	}
}

auto prometheus_shutdown() -> void
{
	print_time();
}
