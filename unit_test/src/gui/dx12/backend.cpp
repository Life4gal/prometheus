#include "../win/def.hpp"
#include "../common/print_time.hpp"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <i18n/i18n.hpp>

using Microsoft::WRL::ComPtr;
using namespace gal::prometheus;

extern const std::size_t num_frames_in_flight;

extern ComPtr<ID3D12Device> g_device;
extern ComPtr<ID3D12GraphicsCommandList> g_command_list;

extern int g_window_width;
extern int g_window_height;

extern double g_last_time;
extern std::uint64_t g_frame_count;
extern float g_fps;

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

	gui::create_current_context();

	set_default_theme(gui::test_theme());
	set_default_draw_list_flag(gui::DrawListFlag::ANTI_ALIASED_LINE | gui::DrawListFlag::ANTI_ALIASED_LINE_USE_TEXTURE | gui::DrawListFlag::ANTI_ALIASED_FILL);

	gui::FontOption font_option{};
	font_option.font_path = R"(C:\Windows\Fonts\msyh.ttc)";
	font_option.pixel_height = 18;
	font_option.glyph_ranges = i18n::RangeBuilder{}.simplified_chinese_common().range();
	auto font_texture = set_default_font(font_option);
	GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(font_texture.valid());

	// Load default font texture
	{
		[[maybe_unused]] const auto load_font_texture_result = load_texture(
			reinterpret_cast<const std::uint8_t*>(font_texture.data.get()),
			font_texture.width,
			font_texture.height,
			g_shader_resource_view_descriptor_heap,
			0,
			g_font_handle,
			g_font_resource
		);
		assert(load_font_texture_result);

		font_texture.bind(static_cast<gui::texture_id_type>(g_font_handle.ptr));
	}

	// Load additional picture texture
	{
		int image_width;
		int image_height;

		auto* data = stbi_load(ASSETS_PATH_PIC, &image_width, &image_height, nullptr, 4);
		assert(data);

		[[maybe_unused]] const auto load_additional_texture_result = load_texture(
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
	auto& io = gui::get_io();

	io.display_size = {static_cast<float>(g_window_width), static_cast<float>(g_window_height)};
	io.delta_time = 1.f / g_fps;

	gui::new_frame();
}

auto prometheus_render() -> void
{
	if (static bool window_closed = false;
		not window_closed)
	{
		window_closed = gui::begin_window("Window 1", {640, 480});

		static bool theme_window_closed = true;
		theme_window_closed ^= gui::draw_button("OpenThemeEditor");

		if (not theme_window_closed)
		{
			theme_window_closed = gui::show_theme_editor();
		}

		gui::draw_text_colored("Text:", primitive::colors::red);
		{
			gui::draw_text("Hello");
			gui::draw_text("World");

			gui::draw_text("你好,");
			gui::layout_same_line();
			gui::draw_text("世界");

			std::string string{};
			string.resize(200);
			for (int i = 0; i < 200; ++i)
			{
				if (i != 0 and i % 25 == 0)
				{
					string[i] = '\n';
				}
				else
				{
					const auto c = 'a' + i % ('z' - 'a');
					string[i] = static_cast<char>(c);
				}
			}

			gui::push_text_wrap_width(150);
			gui::draw_text(string);
			gui::pop_text_wrap_width();

			gui::push_text_wrap_width(300);
			gui::draw_text(string);
			gui::pop_text_wrap_width();
		}

		gui::draw_text_colored("Button:", primitive::colors::red);
		{
			if (gui::draw_button("Button"))
			{
				std::println(stdout, "Press Button!");
			}
			if (gui::draw_small_button("SmallButton"))
			{
				std::println(stdout, "Press SmallButton!");
			}
		}

		gui::draw_text_colored("RadioButton:", primitive::colors::red);
		{
			struct radio_button
			{
				int value;

				[[nodiscard]] constexpr auto operator==(const radio_button& other) const noexcept -> bool
				{
					return value == other.value;
				}
			};
			static radio_button value{.value = 0};

			gui::draw_radio_button("RadioButton1", value, radio_button{.value = 0});
			gui::layout_same_line();
			gui::draw_radio_button("RadioButton2", value, radio_button{.value = 1});
			gui::layout_same_line();
			gui::draw_radio_button("RadioButton3", value, radio_button{.value = 2});
			gui::layout_same_line();
			gui::draw_text(std::format("> Select: {}", value.value));
		}

		gui::draw_text_colored("Checkbox:", primitive::colors::red);
		{
			struct checkbox
			{
				int value;

				[[nodiscard]] constexpr auto operator==(const checkbox& other) const noexcept -> bool
				{
					return value == other.value;
				}
			};
			static checkbox value{.value = 0};

			gui::draw_checkbox("Checkbox", value, checkbox{.value = 1}, checkbox{.value = 0});
			gui::layout_same_line();
			gui::draw_text(std::format("> Select: {}", value.value));
		}

		gui::draw_text_colored("Slider", primitive::colors::red);
		{
			static float v = 0;
			gui::draw_slider("Slider", v, -1, 1);
		}

		gui::end_window();
	}

	gui::render();
}

auto prometheus_draw() -> void
{
	g_frame_resource_index += 1;
	const auto this_frame_index = g_frame_resource_index % num_frames_in_flight;
	auto& this_frame = g_frame_resource[this_frame_index];
	auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = this_frame;

	const auto& draw_datas = gui::get_draw_data();

	const auto [total_vertex_count, total_index_count] = [&]() noexcept
	{
		struct sum
		{
			UINT vertex;
			UINT index;
		};

		return std::ranges::fold_left(
			draw_datas,
			sum{.vertex = 0, .index = 0},
			[](const sum s, const gui::DrawData& draw_data) noexcept -> sum
			{
				const auto vertex_list = draw_data.vertex_list.get();
				const auto index_list = draw_data.index_list.get();

				return
				{
						.vertex = s.vertex + static_cast<UINT>(vertex_list.size()),
						.index = s.index + static_cast<UINT>(index_list.size())
				};
			}
		);
	}();

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
	if (this_frame_index_buffer == nullptr or this_frame_index_count < total_index_count)
	{
		// todo: grow factor
		this_frame_index_count = total_index_count + 10000;

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

		UINT vertex_offset = 0;
		UINT index_offset = 0;

		std::ranges::for_each(
			draw_datas,
			[&](const gui::DrawData& draw_data) noexcept -> void
			{
				const auto vertex_list = draw_data.vertex_list.get();
				const auto index_list = draw_data.index_list.get();

				std::ranges::transform(
					vertex_list,
					mapped_vertex + vertex_offset,
					[](const gui::vertex_type& vertex) -> d3d_vertex_type
					{
						// return {
						// 		.position = {vertex.position.x, vertex.position.y},
						// 		.uv = {vertex.uv.x, vertex.uv.y},
						// 		.color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)
						// };
						return std::bit_cast<d3d_vertex_type>(vertex);
					}
				);
				std::ranges::transform(
					index_list,
					mapped_index + index_offset,
					[vertex_offset](const gui::index_type index) noexcept -> d3d_index_type
					{
						return static_cast<d3d_index_type>(index + vertex_offset);
					}
				);

				vertex_offset += static_cast<UINT>(vertex_list.size());
				index_offset += static_cast<UINT>(index_list.size());
			}
		);

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

	UINT total_index_offset = 0;
	std::ranges::for_each(
		draw_datas,
		[&total_index_offset](const gui::DrawData& draw_data) noexcept -> void
		{
			const auto vertex_list = draw_data.vertex_list.get();
			const auto index_list = draw_data.index_list.get();

			for (const auto& command_list = draw_data.command_list.get();
			     const auto& [clip_rect, texture, index_offset, element_count]: command_list)
			{
				const auto [point, extent] = clip_rect;
				const D3D12_RECT rect
				{
						static_cast<LONG>(point.x),
						static_cast<LONG>(point.y),
						static_cast<LONG>(point.x + extent.width),
						static_cast<LONG>(point.y + extent.height)
				};
				g_command_list->RSSetScissorRects(1, &rect);

				GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture != 0, "push_texture_id when create texture view");
				const D3D12_GPU_DESCRIPTOR_HANDLE texture_handle{.ptr = static_cast<UINT64>(texture)};
				g_command_list->SetGraphicsRootDescriptorTable(1, texture_handle);

				const auto this_index_offset = static_cast<UINT>(total_index_offset + index_offset);
				g_command_list->DrawIndexedInstanced(static_cast<UINT>(element_count), 1, this_index_offset, 0, 0);
			}

			total_index_offset += static_cast<UINT>(index_list.size());
		}
	);
}

auto prometheus_shutdown() -> void
{
	print_time();

	gui::destroy_current_context();
}
