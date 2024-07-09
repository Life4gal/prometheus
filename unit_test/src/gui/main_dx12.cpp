#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

// for print_hr_error
#include <comdef.h>
// for font
#include "font.hpp"

#include <algorithm>
#include <cassert>
#include <memory>
#include <source_location>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

namespace
{
	namespace p
	{
		using namespace gal::prometheus;

		struct d3d_vertex_constant_buffer
		{
			float mvp[4][4];
		};

		struct d3d_render_buffer
		{
			ID3D12Resource* index;
			UINT index_count;
			ID3D12Resource* vertex;
			UINT vertex_count;
		};

		struct d3d_data_type
		{
			ID3D12Device* device;
			ID3D12RootSignature* root_signature;
			ID3D12PipelineState* pipeline_state;
			DXGI_FORMAT rtv_format;
			ID3D12Resource* font_texture_resource;
			D3D12_CPU_DESCRIPTOR_HANDLE font_cpu_descriptor;
			D3D12_GPU_DESCRIPTOR_HANDLE font_gpu_descriptor;
			ID3D12DescriptorHeap* descriptor_heap;

			UINT frames_in_flight;
			std::unique_ptr<d3d_render_buffer[]> frame_resource;
			UINT frame_index;
		};

		using point_type = primitive::basic_point<float, 2>;
		using rect_type = primitive::basic_rect<float, 2>;
		using vertex_type = primitive::basic_vertex<point_type>;
		using vertex_index_type = std::uint16_t;

		struct d3d_vertex_type
		{
			float pos[2];
			float uv[2];
			std::uint32_t color;
		};

		template<typename T> using my_vector_type = std::vector<T>;

		using vertex_list_type = primitive::basic_vertex_list<vertex_type, my_vector_type>;
		// todo
		using vertex_index_list_type = my_vector_type<vertex_index_type>;

		struct draw_list_type
		{
			vertex_list_type vertex_list;
			vertex_index_list_type index_list;
		};

		struct draw_data_type
		{
			rect_type display_rect;

			my_vector_type<draw_list_type> draw_lists;

			[[nodiscard]] constexpr auto total_vertex_size() const noexcept -> std::size_t
			{
				return std::ranges::fold_left(
					draw_lists,
					static_cast<std::size_t>(0),
					[](const auto s, const auto& c) noexcept -> std::size_t { return s + c.vertex_list.size(); }
				);
			}

			[[nodiscard]] constexpr auto total_index_size() const noexcept -> std::size_t
			{
				return std::ranges::fold_left(
					draw_lists,
					static_cast<std::size_t>(0),
					[](const auto s, const auto& c) noexcept -> std::size_t { return s + c.index_list.size(); }
				);
			}
		};

		d3d_data_type g_d3d_data;
		draw_data_type g_draw_data;

		font_type g_font;

		namespace detail
		{
			[[nodiscard]] auto create_fonts_texture() -> bool
			{
				// todo: RGBA(8+8+8+8)
				if (g_font.data == nullptr)
				{
					g_font = load_font();
				}
				const auto& [pixels, width, height] = g_font;

				if (pixels == nullptr)
				{
					return false;
				}

				// Upload texture to graphics system
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
							.Width = static_cast<UINT64>(width),
							.Height = static_cast<UINT>(height),
							.DepthOrArraySize = 1,
							.MipLevels = 1,
							.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
							.SampleDesc = {.Count = 1, .Quality = 0},
							.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
							.Flags = D3D12_RESOURCE_FLAG_NONE
					};

					ID3D12Resource* texture;
					(void)g_d3d_data.device->CreateCommittedResource(
						&heap_properties,
						D3D12_HEAP_FLAG_NONE,
						&resource_desc,
						D3D12_RESOURCE_STATE_COPY_DEST,
						nullptr,
						IID_PPV_ARGS(&texture)
					);

					constexpr D3D12_HEAP_PROPERTIES upload_heap_properties{
							.Type = D3D12_HEAP_TYPE_UPLOAD,
							.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
							.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
							.CreationNodeMask = 0,
							.VisibleNodeMask = 0
					};

					const auto upload_pitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
					const auto upload_size = height * upload_pitch;
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

					ID3D12Resource* upload_buffer;
					if (const auto upload_result = g_d3d_data.device->CreateCommittedResource(
							&upload_heap_properties,
							D3D12_HEAP_FLAG_NONE,
							&upload_resource_desc,
							D3D12_RESOURCE_STATE_GENERIC_READ,
							nullptr,
							IID_PPV_ARGS(&upload_buffer)
						);
						FAILED(upload_result))
					{
						__debugbreak();
						return false;
					}

					void* mapped_data = nullptr;
					const D3D12_RANGE range{.Begin = 0, .End = upload_size};
					if (const auto map_result = upload_buffer->Map(0, &range, &mapped_data); FAILED(map_result))
					{
						__debugbreak();
						return false;
					}
					for (std::decay_t<decltype(height)> i = 0; i < height; ++i)
					{
						std::memcpy(
							reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(mapped_data) + i * upload_pitch),
							pixels + i * width * 4,
							width * 4
						);
					}
					upload_buffer->Unmap(0, &range);

					const D3D12_TEXTURE_COPY_LOCATION source_location{
							.pResource = upload_buffer,
							.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
							.PlacedFootprint =
							{.Offset = 0,
							 .Footprint =
							 {.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
							  .Width = static_cast<UINT>(width),
							  .Height = static_cast<UINT>(height),
							  .Depth = 1,
							  .RowPitch = upload_pitch}}
					};

					const D3D12_TEXTURE_COPY_LOCATION dest_location{
							.pResource = texture, .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, .SubresourceIndex = 0
					};

					const D3D12_RESOURCE_BARRIER barrier{
							.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
							.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
							.Transition =
							{.pResource = texture,
							 .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
							 .StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
							 .StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE}
					};

					ID3D12Fence* fence;
					if (const auto fence_result = g_d3d_data.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
						FAILED(fence_result))
					{
						__debugbreak();
						return false;
					}

					constexpr D3D12_COMMAND_QUEUE_DESC command_queue_desc{
							.Type = D3D12_COMMAND_LIST_TYPE_DIRECT, .Priority = 0, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE, .NodeMask = 1
					};

					ID3D12CommandQueue* command_queue;
					if (const auto command_queue_result = g_d3d_data.device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue));
						FAILED(command_queue_result))
					{
						__debugbreak();
						return false;
					}

					ID3D12CommandAllocator* command_allocator;
					if (const auto command_allocator_result =
								g_d3d_data.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator));
						FAILED(command_allocator_result))
					{
						__debugbreak();
						return false;
					}

					ID3D12GraphicsCommandList* command_list;
					if (const auto command_list_result = g_d3d_data.device->CreateCommandList(
							0,
							D3D12_COMMAND_LIST_TYPE_DIRECT,
							command_allocator,
							nullptr,
							IID_PPV_ARGS(&command_list)
						);
						FAILED(command_list_result))
					{
						__debugbreak();
						return false;
					}

					command_list->CopyTextureRegion(&dest_location, 0, 0, 0, &source_location, nullptr);
					command_list->ResourceBarrier(1, &barrier);

					if (FAILED(command_list->Close()))
					{
						__debugbreak();
						return false;
					}

					command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&command_list));
					if (FAILED(command_queue->Signal(fence, 1)))
					{
						__debugbreak();
						return false;
					}

					auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
					(void)fence->SetEventOnCompletion(1, event);
					WaitForSingleObject(event, INFINITE);

					command_list->Release();
					command_allocator->Release();
					command_queue->Release();

					CloseHandle(event);
					fence->Release();
					upload_buffer->Release();

					// Create texture view
					const D3D12_SHADER_RESOURCE_VIEW_DESC resource_view_desc{
							.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
							.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
							.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
							.Texture2D = {.MostDetailedMip = 0, .MipLevels = resource_desc.MipLevels, .PlaneSlice = 0, .ResourceMinLODClamp = .0f}
					};

					g_d3d_data.device->CreateShaderResourceView(texture, &resource_view_desc, g_d3d_data.font_cpu_descriptor);
					if (g_d3d_data.font_texture_resource)
					{
						g_d3d_data.font_texture_resource->Release();
					}
					g_d3d_data.font_texture_resource = texture;
				}

				return true;
			}

			auto setup_render_state(ID3D12GraphicsCommandList& context, const d3d_render_buffer& frame) -> void
			{
				// Setup orthographic projection matrix into our constant buffer
				d3d_vertex_constant_buffer vertex_constant_buffer;
				{
					const auto [left, top] = g_draw_data.display_rect.left_top();
					const auto [right, bottom] = g_draw_data.display_rect.right_bottom();

					const float mvp[4][4]{
							{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
							{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
							{0.0f, 0.0f, 0.5f, 0.0f},
							{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
					};
					std::memcpy(vertex_constant_buffer.mvp, mvp, sizeof(mvp));
				}

				// Setup viewport
				{
					const D3D12_VIEWPORT viewport{
							.TopLeftX = .0f,
							.TopLeftY = .0f,
							.Width = g_draw_data.display_rect.width(),
							.Height = g_draw_data.display_rect.height(),
							.MinDepth = .0f,
							.MaxDepth = 1.f
					};
					context.RSSetViewports(1, &viewport);
				}

				// Bind shader and vertex buffers
				{
					const D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{
							.BufferLocation = frame.vertex->GetGPUVirtualAddress(),
							.SizeInBytes = frame.vertex_count * static_cast<UINT>(sizeof(d3d_vertex_type)),
							.StrideInBytes = sizeof(d3d_vertex_type)
					};
					context.IASetVertexBuffers(0, 1, &vertex_buffer_view);

					const D3D12_INDEX_BUFFER_VIEW index_buffer_view{
							.BufferLocation = frame.index->GetGPUVirtualAddress(),
							.SizeInBytes = frame.index_count * static_cast<UINT>(sizeof(vertex_index_type)),
							.Format = sizeof(vertex_index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
					};
					context.IASetIndexBuffer(&index_buffer_view);
					context.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					context.SetPipelineState(g_d3d_data.pipeline_state);
					context.SetGraphicsRootSignature(g_d3d_data.root_signature);
					context.SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);
				}

				// Setup blend factor
				{
					constexpr float blend_factor[4]{.0f, .0f, .0f, .0f};
					context.OMSetBlendFactor(blend_factor);
				}
			}
		} // namespace detail

		auto d3d_destroy_device_objects() -> void
		{
			if (g_d3d_data.device == nullptr)
			{
				return;
			}

			if (g_d3d_data.root_signature)
			{
				g_d3d_data.root_signature->Release();
			}
			if (g_d3d_data.pipeline_state)
			{
				g_d3d_data.pipeline_state->Release();
			}
			if (g_d3d_data.font_texture_resource)
			{
				g_d3d_data.font_texture_resource->Release();
			}

			for (UINT i = 0; i < g_d3d_data.frames_in_flight; ++i)
			{
				const auto& frame = g_d3d_data.frame_resource[i];
				if (frame.vertex)
				{
					frame.vertex->Release();
				}
				if (frame.index)
				{
					frame.index->Release();
				}
			}
		}

		auto d3d_create_device_objects() -> bool
		{
			if (g_d3d_data.device == nullptr)
			{
				return false;
			}

			if (g_d3d_data.pipeline_state)
			{
				d3d_destroy_device_objects();
			}

			// Create the root signature
			{
				constexpr D3D12_DESCRIPTOR_RANGE range{
						.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
						.NumDescriptors = 1,
						.BaseShaderRegister = 0,
						.RegisterSpace = 0,
						.OffsetInDescriptorsFromTableStart = 0
				};

				constexpr D3D12_ROOT_PARAMETER param_0{
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
						.Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = 16},
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
				};
				const D3D12_ROOT_PARAMETER param_1{
						.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
						.DescriptorTable = {.NumDescriptorRanges = 1, .pDescriptorRanges = &range},
						.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
				};
				const D3D12_ROOT_PARAMETER params[]{param_0, param_1};

				// Bi-linear sampling is required by default
				constexpr D3D12_STATIC_SAMPLER_DESC static_sampler_desc{
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

				const D3D12_ROOT_SIGNATURE_DESC root_signature_desc{
						.NumParameters = static_cast<UINT>(std::ranges::size(params)),
						.pParameters = params,
						.NumStaticSamplers = 1,
						.pStaticSamplers = &static_sampler_desc,
						.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
						         D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
						         D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
				};

				static auto d3d12_dll = GetModuleHandleW(L"d3d12.dll");
				if (d3d12_dll == nullptr)
				{
					d3d12_dll = LoadLibraryW(L"d3d12.dll");

					if (d3d12_dll == nullptr)
					{
						return false;
					}
				}

				auto serialize_root_signature_function =
						reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature"));
				if (serialize_root_signature_function == nullptr)
				{
					return false;
				}

				ID3DBlob* blob = nullptr;
				if (serialize_root_signature_function(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr) != S_OK)
				{
					__debugbreak();
					return false;
				}

				(void)g_d3d_data.device->CreateRootSignature(0,
				                                             blob->GetBufferPointer(),
				                                             blob->GetBufferSize(),
				                                             IID_PPV_ARGS(&g_d3d_data.root_signature));
				blob->Release();
			}

			// Create the vertex shader
			auto* vertex_shader_blob = []() -> ID3DBlob* {
				constexpr static char shader[] =
						"cbuffer vertexBuffer : register(b0) \
			            {\
			              float4x4 ProjectionMatrix; \
			            };\
			            struct VS_INPUT\
			            {\
			              float2 pos : POSITION;\
			              float4 col : COLOR0;\
			              float2 uv  : TEXCOORD0;\
			            };\
			            \
			            struct PS_INPUT\
			            {\
			              float4 pos : SV_POSITION;\
			              float4 col : COLOR0;\
			              float2 uv  : TEXCOORD0;\
			            };\
			            \
			            PS_INPUT main(VS_INPUT input)\
			            {\
			              PS_INPUT output;\
			              output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			              output.col = input.col;\
			              output.uv  = input.uv;\
			              return output;\
			            }";

				ID3DBlob* blob;
				if (const auto result =
							D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &blob, nullptr);
					FAILED(result))
				{
					__debugbreak();
					return nullptr;
				}

				return blob;
			}();

			// Create the pixel shader
			auto* pixel_shader_blob = []() -> ID3DBlob* {
				constexpr static char shader[] =
						"struct PS_INPUT\
			            {\
			              float4 pos : SV_POSITION;\
			              float4 col : COLOR0;\
			              float2 uv  : TEXCOORD0;\
			            };\
			            SamplerState sampler0 : register(s0);\
			            Texture2D texture0 : register(t0);\
			            \
			            float4 main(PS_INPUT input) : SV_Target\
			            {\
			              float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
			              return out_col; \
			            }";

				ID3DBlob* blob;
				if (const auto result =
							D3DCompile(shader, std::ranges::size(shader), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &blob, nullptr);
					FAILED(result))
				{
					__debugbreak();
					return nullptr;
				}

				return blob;
			}();

			if (vertex_shader_blob == nullptr or pixel_shader_blob == nullptr)
			{
				return false;
			}

			// Create the blending setup
			constexpr D3D12_RENDER_TARGET_BLEND_DESC render_target_blend_desc{
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
			constexpr D3D12_BLEND_DESC blend_desc{
					.AlphaToCoverageEnable = FALSE, .IndependentBlendEnable = FALSE, .RenderTarget = {render_target_blend_desc}
			};

			// Create the rasterizer state
			constexpr D3D12_RASTERIZER_DESC rasterizer_desc{
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
			constexpr D3D12_DEPTH_STENCIL_DESC depth_stencil_desc{
					.DepthEnable = FALSE,
					.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
					.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS,
					.StencilEnable = FALSE,
					.StencilReadMask = 0,
					.StencilWriteMask = 0,
					.FrontFace =
					{.StencilFailOp = D3D12_STENCIL_OP_KEEP,
					 .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
					 .StencilPassOp = D3D12_STENCIL_OP_KEEP,
					 .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS},
					.BackFace =
					{.StencilFailOp = D3D12_STENCIL_OP_KEEP,
					 .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
					 .StencilPassOp = D3D12_STENCIL_OP_KEEP,
					 .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS}
			};

			// Create the input layout
			constexpr D3D12_INPUT_ELEMENT_DESC input_element_desc[]{
					{.SemanticName = "POSITION",
					 .SemanticIndex = 0,
					 .Format = DXGI_FORMAT_R32G32_FLOAT,
					 .InputSlot = 0,
					 .AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, position)),
					 .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					 .InstanceDataStepRate = 0},
					{.SemanticName = "TEXCOORD",
					 .SemanticIndex = 0,
					 .Format = DXGI_FORMAT_R32G32_FLOAT,
					 .InputSlot = 0,
					 .AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, uv)),
					 .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					 .InstanceDataStepRate = 0},
					{.SemanticName = "COLOR",
					 .SemanticIndex = 0,
					 .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
					 .InputSlot = 0,
					 .AlignedByteOffset = static_cast<UINT>(offsetof(vertex_type, color)),
					 .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
					 .InstanceDataStepRate = 0},
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{
					.pRootSignature = g_d3d_data.root_signature,
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
					.InputLayout =
					{.pInputElementDescs = input_element_desc, .NumElements = static_cast<UINT>(std::ranges::size(input_element_desc))},
					.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
					.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
					.NumRenderTargets = 1,
					.RTVFormats = {g_d3d_data.rtv_format},
					.DSVFormat = {},
					.SampleDesc = {.Count = 1, .Quality = 0},
					.NodeMask = 1,
					.CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
					.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
			};

			const auto result = g_d3d_data.device->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(&g_d3d_data.pipeline_state));
			vertex_shader_blob->Release();
			pixel_shader_blob->Release();

			if (result != S_OK)
			{
				return false;
			}

			return detail::create_fonts_texture();
		}

		auto d3d_init(
			ID3D12Device& device,
			const DXGI_FORMAT rtv_format,
			const D3D12_CPU_DESCRIPTOR_HANDLE font_cpu_descriptor,
			const D3D12_GPU_DESCRIPTOR_HANDLE font_gpu_descriptor,
			ID3D12DescriptorHeap& descriptor_heap,
			const UINT frames_in_flight
		) -> void
		{
			g_d3d_data.device = std::addressof(device);
			g_d3d_data.root_signature = nullptr;
			g_d3d_data.pipeline_state = nullptr;
			g_d3d_data.rtv_format = rtv_format;
			g_d3d_data.font_texture_resource = nullptr;
			g_d3d_data.font_cpu_descriptor = font_cpu_descriptor;
			g_d3d_data.font_gpu_descriptor = font_gpu_descriptor;
			g_d3d_data.descriptor_heap = std::addressof(descriptor_heap);

			g_d3d_data.frames_in_flight = frames_in_flight;
			g_d3d_data.frame_resource = std::make_unique<d3d_render_buffer[]>(frames_in_flight);
			// note: overflow(max + 1 => 0)
			g_d3d_data.frame_index = (std::numeric_limits<decltype(g_d3d_data.frame_index)>::max)();
			for (UINT i = 0; i < frames_in_flight; ++i)
			{
				auto& frame = g_d3d_data.frame_resource[i];
				// todo
				frame.vertex = nullptr;
				frame.index = nullptr;
			}
		}

		auto d3d_shutdown() -> void
		{
			d3d_destroy_device_objects();

			// delete g_d3d_data.frame_resource

			// fixme
			// if (g_font.data)
			// {
			// 	std::free(g_font.data);
			// }
		}

		auto d3d_new_frame() -> void //
		{
			if (g_d3d_data.pipeline_state == nullptr) //
			{
				d3d_create_device_objects();
			}
		}

		auto render_draw_data(ID3D12GraphicsCommandList& context) -> void
		{
			auto& draw_data = g_draw_data;
			auto& d3d_data = g_d3d_data;

			// Avoid rendering when minimized
			if (not draw_data.display_rect.valid() or draw_data.display_rect.empty())
			{
				return;
			}

			d3d_data.frame_index += 1;
			const auto this_frame_index = d3d_data.frame_index % d3d_data.frames_in_flight;
			auto& this_frame = d3d_data.frame_resource[this_frame_index];
			auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = this_frame;

			constexpr D3D12_HEAP_PROPERTIES heap_properties{
					.Type = D3D12_HEAP_TYPE_UPLOAD,
					.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
					.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
					.CreationNodeMask = 0,
					.VisibleNodeMask = 0
			};
			// Create and grow vertex/index buffers if needed
			if (this_frame_vertex_buffer == nullptr or this_frame_vertex_count < draw_data.total_vertex_size())
			{
				if (this_frame_vertex_buffer)
				{
					this_frame_vertex_buffer->Release();
				}

				// todo: grow factor
				this_frame_vertex_count = static_cast<UINT>(draw_data.total_vertex_size()) + 5000;

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
				if (const auto resource_result = d3d_data.device->CreateCommittedResource(
						&heap_properties,
						D3D12_HEAP_FLAG_NONE,
						&resource_desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&this_frame_vertex_buffer)
					);
					FAILED(resource_result))
				{
					return;
				}
			}
			if (this_frame_index_buffer == nullptr or this_frame_index_count < draw_data.total_index_size())
			{
				if (this_frame_index_buffer)
				{
					this_frame_index_buffer->Release();
				}

				// todo: grow factor
				this_frame_index_count = static_cast<UINT>(draw_data.total_index_size()) + 10000;

				const D3D12_RESOURCE_DESC resource_desc{
						.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
						.Alignment = 0,
						.Width = this_frame_index_count * sizeof(vertex_index_type),
						.Height = 1,
						.DepthOrArraySize = 1,
						.MipLevels = 1,
						.Format = DXGI_FORMAT_UNKNOWN,
						.SampleDesc = {.Count = 1, .Quality = 0},
						.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
						.Flags = D3D12_RESOURCE_FLAG_NONE
				};
				if (const auto resource_result = d3d_data.device->CreateCommittedResource(
						&heap_properties,
						D3D12_HEAP_FLAG_NONE,
						&resource_desc,
						D3D12_RESOURCE_STATE_GENERIC_READ,
						nullptr,
						IID_PPV_ARGS(&this_frame_index_buffer)
					);
					FAILED(resource_result))
				{
					return;
				}
			}

			// Upload vertex/index data into a single contiguous GPU buffer
			void* mapped_vertex;
			void* mapped_index;
			constexpr D3D12_RANGE range{.Begin = 0, .End = 0};
			if (this_frame_vertex_buffer->Map(0, &range, &mapped_vertex) != S_OK)
			{
				// todo
				return;
			}
			if (this_frame_index_buffer->Map(0, &range, &mapped_index) != S_OK)
			{
				// todo
				return;
			}

			std::ranges::for_each(
				draw_data.draw_lists,
				[
					// fixme: start_lifetime_as
					vertex_dest = static_cast<d3d_vertex_type*>(mapped_vertex),
					index_dest = static_cast<vertex_index_type*>(mapped_index)
				](const draw_list_type& draw_list) mutable
				{
					const auto& source_range = draw_list.vertex_list.vertices();
					std::ranges::transform(
						source_range,
						vertex_dest,
						[](const vertex_type& vertex) noexcept -> d3d_vertex_type
						{
							return
							{
									.pos = {vertex.position.x, vertex.position.y},
									.uv = {vertex.uv.x, vertex.uv.y},
									.color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)
							};
						}
					);
					std::ranges::copy(draw_list.index_list, index_dest);

					vertex_dest += source_range.size();
					index_dest += draw_list.index_list.size();
				}
			);
			this_frame_vertex_buffer->Unmap(0, &range);
			this_frame_index_buffer->Unmap(0, &range);

			detail::setup_render_state(context, this_frame);

			std::ranges::for_each(
				draw_data.draw_lists,
				[&context,
					left_top = draw_data.display_rect.left_top(),
					right_bottom = draw_data.display_rect.right_bottom(),
					offset_vertex = static_cast<INT>(0),
					offset_index = static_cast<UINT>(0)](const draw_list_type& draw_list) mutable
				{
					// todo: clip rect
					const D3D12_RECT rect{
							static_cast<LONG>(left_top.x),
							static_cast<LONG>(left_top.y),
							static_cast<LONG>(right_bottom.x),
							static_cast<LONG>(right_bottom.y)
					};

					context.RSSetScissorRects(1, &rect);
					context.DrawIndexedInstanced(static_cast<UINT>(draw_list.index_list.size()), 1, offset_index, offset_vertex, 0);
					// context.DrawInstanced(static_cast<UINT>(draw_list.vertex_list.size()), 1, offset_vertex, 0);

					offset_vertex += static_cast<UINT>(draw_list.vertex_list.size());
					offset_index += static_cast<INT>(draw_list.index_list.size());
				}
			);
		}
	} // namespace p

	struct d3d_frame_context
	{
		ID3D12CommandAllocator* command_allocator;
		UINT64 fence_value;
	};

	constexpr auto num_frames_in_flight = 3;
	constexpr auto num_back_buffers = 3;

	d3d_frame_context g_frame_context[num_frames_in_flight] = {};
	UINT g_frame_index = 0;

	ID3D12Device* g_d3d_device = nullptr;
	ID3D12DescriptorHeap* g_d3d_rtv_desc_heap = nullptr;
	ID3D12DescriptorHeap* g_d3d_srv_desc = nullptr;
	ID3D12CommandQueue* g_d3d_command_queue = nullptr;
	ID3D12GraphicsCommandList* g_d3d_command_list = nullptr;
	ID3D12Fence* g_fence = nullptr;
	HANDLE g_fence_event = nullptr;
	UINT64 g_fence_last_signaled_value = 0;
	IDXGISwapChain3* g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;
	HANDLE g_swap_chain_waitable_object = nullptr;
	ID3D12Resource* g_main_render_target_resource[num_back_buffers] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE g_main_render_target_descriptor[num_back_buffers] = {};

	void print_hr_error(const HRESULT hr, const std::source_location& location = std::source_location::current())
	{
		const _com_error err(hr);
		std::println(std::cerr, "Error: {} --- at {}:{}", err.ErrorMessage(), location.file_name(), location.line());
	}

	auto create_d3d_device(HWND window) -> bool;
	auto cleanup_d3d_device() -> void;
	auto create_render_target() -> void;
	auto cleanup_render_target() -> void;
	auto wait_for_last_submitted_frame() -> void;
	auto wait_for_next_frame_resources() -> d3d_frame_context*;

	auto WINAPI my_window_procedure(HWND window, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT;

	auto create_d3d_device(HWND window) -> bool
	{
		// Setup swap chain
		constexpr DXGI_SWAP_CHAIN_DESC1 swap_chain_desc{
				.Width = 0,
				.Height = 0,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.Stereo = FALSE,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
				.BufferCount = num_back_buffers,
				.Scaling = DXGI_SCALING_STRETCH,
				.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
				.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
				.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
		};

		#ifdef DX12_ENABLE_DEBUG_LAYER
		// [DEBUG] Enable debug interface
		ID3D12Debug* dx12_debug = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dx12_debug))))
		{
			dx12_debug->EnableDebugLayer();
		}
		#endif

		// Create device
		constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
		if (const auto result = D3D12CreateDevice(nullptr, feature_level, IID_PPV_ARGS(&g_d3d_device)); result != S_OK)
		{
			print_hr_error(result);
			return false;
		}

		#ifdef DX12_ENABLE_DEBUG_LAYER
		if (dx12_debug != nullptr)
		{
			// [DEBUG] Setup debug interface to break on any warnings/errors
			ID3D12InfoQueue* info_queue = nullptr;
			(void)g_d3d_device->QueryInterface(IID_PPV_ARGS(&info_queue));
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			info_queue->Release();

			// todo: cannot run program normally
			// Enable GPU-based validation (optional but useful)
			// ID3D12Debug1* dx12_debug1 = nullptr;
			// if (SUCCEEDED(dx12_debug->QueryInterface(IID_PPV_ARGS(&dx12_debug1))))
			// {
			// 	dx12_debug1->SetEnableGPUBasedValidation(true);
			// }
			// dx12_debug1->Release();

			dx12_debug->Release();
		}

		IDXGIInfoQueue* dxgi_info_queue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue))))
		{
			dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
			dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
		}
		dxgi_info_queue->Release();
		#endif

		{
			constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
					.NumDescriptors = num_back_buffers,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
					.NodeMask = 1
			};
			if (const auto result = g_d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_d3d_rtv_desc_heap)); result != S_OK)
			{
				print_hr_error(result);
				print_hr_error(g_d3d_device->GetDeviceRemovedReason());

				return false;
			}

			const auto rtv_descriptor_size = g_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_d3d_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
			for (auto& h: g_main_render_target_descriptor)
			{
				h = rtv_handle;
				rtv_handle.ptr += rtv_descriptor_size;
			}
		}

		{
			constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					.NumDescriptors = 1,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					.NodeMask = 0
			};
			if (const auto result = g_d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_d3d_srv_desc)); result != S_OK)
			{
				print_hr_error(result);
				return false;
			}
		}

		{
			constexpr D3D12_COMMAND_QUEUE_DESC desc{
					.Type = D3D12_COMMAND_LIST_TYPE_DIRECT, .Priority = 0, .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE, .NodeMask = 1
			};
			if (const auto result = g_d3d_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_d3d_command_queue)); result != S_OK)
			{
				print_hr_error(result);
				return false;
			}
		}

		for (auto& [command_allocator, _]: g_frame_context)
		{
			if (g_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)) != S_OK)
			{
				return false;
			}
		}

		if (g_d3d_device->CreateCommandList(
			    0,
			    D3D12_COMMAND_LIST_TYPE_DIRECT,
			    g_frame_context[0].command_allocator,
			    nullptr,
			    IID_PPV_ARGS(&g_d3d_command_list)
		    ) != S_OK ||
		    g_d3d_command_list->Close() != S_OK)
		{
			return false;
		}

		if (g_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
		{
			return false;
		}

		g_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (g_fence_event == nullptr)
		{
			return false;
		}

		{
			IDXGIFactory4* dxgi_factory = nullptr;
			IDXGISwapChain1* swap_chain1 = nullptr;
			if (const auto result = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)); result != S_OK)
			{
				return false;
			}
			if (const auto result =
						dxgi_factory->CreateSwapChainForHwnd(g_d3d_command_queue, window, &swap_chain_desc, nullptr, nullptr, &swap_chain1);
				result != S_OK)
			{
				print_hr_error(result);

				return false;
			}
			if (const auto result = swap_chain1->QueryInterface(IID_PPV_ARGS(&g_swap_chain)); result != S_OK)
			{
				print_hr_error(result);
				return false;
			}
			swap_chain1->Release();
			dxgi_factory->Release();
			(void)g_swap_chain->SetMaximumFrameLatency(num_back_buffers);
			g_swap_chain_waitable_object = g_swap_chain->GetFrameLatencyWaitableObject();
		}

		create_render_target();
		return true;
	}

	auto cleanup_d3d_device() -> void
	{
		cleanup_render_target();
		if (g_swap_chain)
		{
			(void)g_swap_chain->SetFullscreenState(false, nullptr);
			g_swap_chain->Release();
			g_swap_chain = nullptr;
		}
		if (g_swap_chain_waitable_object != nullptr)
		{
			CloseHandle(g_swap_chain_waitable_object);
		}
		for (auto& [command_allocator, _]: g_frame_context)
		{
			if (command_allocator)
			{
				command_allocator->Release();
				command_allocator = nullptr;
			}
		}
		if (g_d3d_command_queue)
		{
			g_d3d_command_queue->Release();
			g_d3d_command_queue = nullptr;
		}
		if (g_d3d_command_list)
		{
			g_d3d_command_list->Release();
			g_d3d_command_list = nullptr;
		}
		if (g_d3d_rtv_desc_heap)
		{
			g_d3d_rtv_desc_heap->Release();
			g_d3d_rtv_desc_heap = nullptr;
		}
		if (g_d3d_srv_desc)
		{
			g_d3d_srv_desc->Release();
			g_d3d_srv_desc = nullptr;
		}
		if (g_fence)
		{
			g_fence->Release();
			g_fence = nullptr;
		}
		if (g_fence_event)
		{
			CloseHandle(g_fence_event);
			g_fence_event = nullptr;
		}
		if (g_d3d_device)
		{
			g_d3d_device->Release();
			g_d3d_device = nullptr;
		}

		#ifdef DX12_ENABLE_DEBUG_LAYER
		IDXGIDebug1* dxgi_debug = nullptr;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug))))
		{
			(void)dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
			dxgi_debug->Release();
		}
		#endif
	}

	auto create_render_target() -> void
	{
		for (UINT i = 0; i < num_back_buffers; i++)
		{
			ID3D12Resource* back_buffer = nullptr;
			(void)g_swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
			g_d3d_device->CreateRenderTargetView(back_buffer, nullptr, g_main_render_target_descriptor[i]);
			g_main_render_target_resource[i] = back_buffer;
		}
	}

	auto cleanup_render_target() -> void
	{
		wait_for_last_submitted_frame();

		for (auto& resource: g_main_render_target_resource)
		{
			if (resource)
			{
				resource->Release();
				resource = nullptr;
			}
		}
	}

	auto wait_for_last_submitted_frame() -> void
	{
		d3d_frame_context* frame_context = &g_frame_context[g_frame_index % num_frames_in_flight];

		const UINT64 fence_value = frame_context->fence_value;
		if (fence_value == 0)
		{
			// No fence was signaled
			return;
		}

		frame_context->fence_value = 0;
		if (g_fence->GetCompletedValue() >= fence_value)
		{
			return;
		}

		(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
		WaitForSingleObject(g_fence_event, INFINITE);
	}

	auto wait_for_next_frame_resources() -> d3d_frame_context*
	{
		const UINT next_frame_index = g_frame_index + 1;
		g_frame_index = next_frame_index;

		HANDLE waitable_objects[] = {g_swap_chain_waitable_object, nullptr};
		DWORD num_waitable_objects = 1;

		d3d_frame_context* frame_context = &g_frame_context[next_frame_index % num_frames_in_flight];
		if (const UINT64 fence_value = frame_context->fence_value; fence_value != 0) // means no fence was signaled
		{
			frame_context->fence_value = 0;
			(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
			waitable_objects[1] = g_fence_event;
			num_waitable_objects = 2;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects, TRUE, INFINITE);

		return frame_context;
	}

	auto WINAPI my_window_procedure(HWND window, const UINT msg, const WPARAM w_param, const LPARAM l_param) -> LRESULT
	{
		switch (msg)
		{
			case WM_SIZE:
			{
				if (g_d3d_device != nullptr && w_param != SIZE_MINIMIZED)
				{
					wait_for_last_submitted_frame();
					cleanup_render_target();
					const HRESULT result = g_swap_chain->ResizeBuffers(
						0,
						(UINT)LOWORD(l_param),
						(UINT)HIWORD(l_param),
						DXGI_FORMAT_UNKNOWN,
						DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
					);
					assert(SUCCEEDED(result) && "Failed to resize swapchain.");
					create_render_target();
				}
				return 0;
			}
			case WM_DESTROY:
			{
				::PostQuitMessage(0);
				return 0;
			}
			default:
			{
				return ::DefWindowProcW(window, msg, w_param, l_param);
			}
		}
	}
} // namespace

int main(int, char**)
{
	const WNDCLASSEXW wc{
			.cbSize = sizeof(WNDCLASSEXW),
			.style = CS_CLASSDC,
			.lpfnWndProc = my_window_procedure,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = GetModuleHandle(nullptr),
			.hIcon = nullptr,
			.hCursor = nullptr,
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = L"GUI Playground",
			.hIconSm = nullptr
	};
	::RegisterClassExW(&wc);
	HWND window = ::CreateWindowW(
		wc.lpszClassName,
		L"GUI Playground Example",
		WS_OVERLAPPEDWINDOW,
		100,
		100,
		1280,
		800,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	// Initialize Direct3D
	if (not create_d3d_device(window))
	{
		cleanup_d3d_device();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	p::d3d_init(
		*g_d3d_device,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		g_d3d_srv_desc->GetCPUDescriptorHandleForHeapStart(),
		g_d3d_srv_desc->GetGPUDescriptorHandleForHeapStart(),
		*g_d3d_srv_desc,
		num_frames_in_flight
	);

	// Show the window
	::ShowWindow(window, SW_SHOWDEFAULT);
	::UpdateWindow(window);

	// test
	{
		{
			RECT rect;
			GetClientRect(window, &rect);
			p::g_draw_data.display_rect = rect;
		}

		{
			using namespace p;
			auto& draw_list = g_draw_data.draw_lists.emplace_back();
			draw_list.vertex_list.triangle({100, 100}, {150, 150}, {200, 100}, primitive::colors::blue);
			draw_list.vertex_list.rect_filled({150, 150}, {200, 200}, primitive::colors::gold);
			draw_list.vertex_list.rect_filled({200, 200}, {300, 300}, primitive::colors::red);

			const vertex_list_type::rect_type rect{vertex_list_type::point_type{300, 300}, vertex_list_type::extent_type{200, 200}};
			draw_list.vertex_list.rect(rect, primitive::colors::light_pink);
			draw_list.vertex_list.circle(primitive::inscribed_circle(rect), primitive::colors::orange);
			draw_list.vertex_list.circle(primitive::circumscribed_circle(rect), primitive::colors::orange);

			draw_list.vertex_list.circle_filled({100, 400}, 100, primitive::colors::red);

			draw_list.vertex_list.arc<primitive::ArcQuadrant::Q1>({400, 150}, 80, primitive::colors::red);
			draw_list.vertex_list.arc_filled<primitive::ArcQuadrant::Q2>({400, 150}, 60, primitive::colors::green);
			draw_list.vertex_list.arc<primitive::ArcQuadrant::Q3>({400, 150}, 40, primitive::colors::blue);
			draw_list.vertex_list.arc_filled<primitive::ArcQuadrant::Q4>({400, 150}, 20, primitive::colors::yellow);
			draw_list.vertex_list.circle_filled({400, 150}, 10, primitive::colors::gold);

			draw_list.vertex_list.triangle({100, 100}, {150, 150}, {200, 100}, primitive::colors::gold);
			draw_list.index_list.push_back(0);
			draw_list.index_list.push_back(1);
			draw_list.index_list.push_back(2);
		}
	}

	// Main loop
	bool done = false;
	while (not done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the my_window_procedure() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
		}
		if (done)
		{
			break;
		}

		// Handle window screen locked
		if (g_swap_chain_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_swap_chain_occluded = false;

		p::d3d_new_frame();

		// Update draw data

		// ...

		// Rendering

		d3d_frame_context* frame_context = wait_for_next_frame_resources();
		const UINT back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();
		(void)frame_context->command_allocator->Reset();

		D3D12_RESOURCE_BARRIER barrier{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition =
				{.pResource = g_main_render_target_resource[back_buffer_index],
				 .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				 .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
				 .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET}
		};
		(void)g_d3d_command_list->Reset(frame_context->command_allocator, nullptr);
		g_d3d_command_list->ResourceBarrier(1, &barrier);

		constexpr float clear_color_with_alpha[4]{.45f, .55f, .6f, 1.f};
		g_d3d_command_list->ClearRenderTargetView(g_main_render_target_descriptor[back_buffer_index], clear_color_with_alpha, 0, nullptr);
		g_d3d_command_list->OMSetRenderTargets(1, &g_main_render_target_descriptor[back_buffer_index], FALSE, nullptr);
		g_d3d_command_list->SetDescriptorHeaps(1, &g_d3d_srv_desc);
		p::render_draw_data(*g_d3d_command_list);
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_d3d_command_list->ResourceBarrier(1, &barrier);
		(void)g_d3d_command_list->Close();

		g_d3d_command_queue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&g_d3d_command_list));

		// Present
		const auto hr = g_swap_chain->Present(1, 0); // Present with vsync
		// const auto hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);

		const UINT64 fence_value = g_fence_last_signaled_value + 1;
		(void)g_d3d_command_queue->Signal(g_fence, fence_value);
		g_fence_last_signaled_value = fence_value;
		frame_context->fence_value = fence_value;
	}

	wait_for_last_submitted_frame();

	// Cleanup
	p::d3d_shutdown();
	cleanup_d3d_device();
	::DestroyWindow(window);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
