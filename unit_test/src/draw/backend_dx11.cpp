#include "def.hpp"
#include "dx_error_handler.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <comdef.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace
{
	using Microsoft::WRL::ComPtr;

	using namespace gal::prometheus;
} // namespace

extern ComPtr<ID3D11Device> g_device;
extern ComPtr<ID3D11DeviceContext> g_device_immediate_context;

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
		ComPtr<ID3D11Buffer> index;
		UINT index_count;
		ComPtr<ID3D11Buffer> vertex;
		UINT vertex_count;
	};

	render_buffer_type render_buffer;

	ComPtr<ID3D11BlendState> g_blend_state = nullptr;
	ComPtr<ID3D11RasterizerState> g_rasterizer_state = nullptr;
	ComPtr<ID3D11DepthStencilState> g_depth_stencil_state = nullptr;

	ComPtr<ID3D11VertexShader> g_vertex_shader = nullptr;
	ComPtr<ID3D11InputLayout> g_vertex_input_layout = nullptr;
	ComPtr<ID3D11Buffer> g_vertex_projection_matrix = nullptr;

	ComPtr<ID3D11PixelShader> g_pixel_shader = nullptr;

	ComPtr<ID3D11ShaderResourceView> g_font_texture = nullptr;
	ComPtr<ID3D11SamplerState> g_font_sampler = nullptr;

	ComPtr<ID3D11ShaderResourceView> g_additional_picture_texture = nullptr;

	[[nodiscard]] auto load_texture(const std::uint8_t* texture_data, const std::uint32_t texture_width, const std::uint32_t texture_height, ComPtr<ID3D11ShaderResourceView>& out_srv) -> bool
	{
		assert(texture_data);
		assert(texture_width != 0 and texture_height != 0);

		const D3D11_TEXTURE2D_DESC texture_2d_desc{
				.Width = static_cast<UINT>(texture_width),
				.Height = static_cast<UINT>(texture_height),
				.MipLevels = 1,
				.ArraySize = 1,
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.SampleDesc = {.Count = 1, .Quality = 0},
				.Usage = D3D11_USAGE_DEFAULT,
				.BindFlags = D3D11_BIND_SHADER_RESOURCE,
				.CPUAccessFlags = 0,
				.MiscFlags = 0
		};

		const D3D11_SUBRESOURCE_DATA subresource_data
		{
				.pSysMem = texture_data,
				.SysMemPitch = static_cast<UINT>(texture_width * 4),
				.SysMemSlicePitch = 0
		};

		ComPtr<ID3D11Texture2D> texture;
		if (not check_hr_error<false>(g_device->CreateTexture2D(&texture_2d_desc, &subresource_data, texture.GetAddressOf())))
		{
			return false;
		}

		const D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{
				.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
				.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
				.Texture2D = {.MostDetailedMip = 0, .MipLevels = texture_2d_desc.MipLevels}
		};
		if (not check_hr_error<false>(g_device->CreateShaderResourceView(texture.Get(), &shader_resource_view_desc, out_srv.ReleaseAndGetAddressOf())))
		{
			return false;
		}

		return true;
	}
}

auto prometheus_init() -> void //
{
	print_time();

	using functional::operators::operator|;
	g_draw_list.draw_list_flag(draw::DrawListFlag::ANTI_ALIASED_LINE | draw::DrawListFlag::ANTI_ALIASED_FILL);
	g_draw_list.shared_data(g_draw_list_shared_data);

	// Create the blending setup
	{
		constexpr D3D11_RENDER_TARGET_BLEND_DESC render_target{
				.BlendEnable = TRUE,
				.SrcBlend = D3D11_BLEND_SRC_ALPHA,
				.DestBlend = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOp = D3D11_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D11_BLEND_ONE,
				.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
				.BlendOpAlpha = D3D11_BLEND_OP_ADD,
				.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL
		};
		constexpr D3D11_BLEND_DESC blend_desc{
				.AlphaToCoverageEnable = FALSE,
				.IndependentBlendEnable = FALSE,
				.RenderTarget =
				{
						render_target,
				}
		};

		check_hr_error(g_device->CreateBlendState(&blend_desc, g_blend_state.ReleaseAndGetAddressOf()));
	}

	// Create the rasterizer state
	{
		constexpr D3D11_RASTERIZER_DESC rasterizer_desc{
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

		check_hr_error(g_device->CreateRasterizerState(&rasterizer_desc, g_rasterizer_state.ReleaseAndGetAddressOf()));
	}

	// Create depth-stencil State
	{
		constexpr D3D11_DEPTH_STENCIL_DESC depth_stencil_desc{
				.DepthEnable = FALSE,
				.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
				.DepthFunc = D3D11_COMPARISON_ALWAYS,
				.StencilEnable = FALSE,
				.StencilReadMask = 0,
				.StencilWriteMask = 0,
				.FrontFace =
				{
						.StencilFailOp = D3D11_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
						.StencilPassOp = D3D11_STENCIL_OP_KEEP,
						.StencilFunc = D3D11_COMPARISON_ALWAYS
				},
				.BackFace =
				{
						.StencilFailOp = D3D11_STENCIL_OP_KEEP,
						.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
						.StencilPassOp = D3D11_STENCIL_OP_KEEP,
						.StencilFunc = D3D11_COMPARISON_ALWAYS
				}
		};

		check_hr_error(g_device->CreateDepthStencilState(&depth_stencil_desc, g_depth_stencil_state.ReleaseAndGetAddressOf()));
	}

	// Create shader
	{
		// vertex
		{
			constexpr char shader_code[]{
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
			if (
				const auto result = D3DCompile(
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
					std::cerr,
					"D3DCompile failed: {} -- at {}:{}",
					static_cast<const char*>(error_message->GetBufferPointer()),
					std::source_location::current().file_name(),
					std::source_location::current().line()
				);
				check_hr_error(result);
			}
			check_hr_error(g_device->CreateVertexShader(
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				nullptr,
				g_vertex_shader.ReleaseAndGetAddressOf()
			));

			// vertex input layout
			constexpr D3D11_INPUT_ELEMENT_DESC input_element_desc[]{
					{
							.SemanticName = "POSITION",
							.SemanticIndex = 0,
							.Format = DXGI_FORMAT_R32G32_FLOAT,
							.InputSlot = 0,
							.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, position)),
							.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
							.InstanceDataStepRate = 0
					},
					{
							.SemanticName = "COLOR",
							.SemanticIndex = 0,
							.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
							.InputSlot = 0,
							.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, color)),
							.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
							.InstanceDataStepRate = 0
					},
					{
							.SemanticName = "TEXCOORD",
							.SemanticIndex = 0,
							.Format = DXGI_FORMAT_R32G32_FLOAT,
							.InputSlot = 0,
							.AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, uv)),
							.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
							.InstanceDataStepRate = 0
					},
			};
			check_hr_error(g_device->CreateInputLayout(
				input_element_desc,
				static_cast<UINT>(std::ranges::size(input_element_desc)),
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				g_vertex_input_layout.ReleaseAndGetAddressOf()
			));

			// constant buffer
			constexpr D3D11_BUFFER_DESC constant_buffer_desc{
					.ByteWidth = sizeof(d3d_projection_matrix_type),
					.Usage = D3D11_USAGE_DYNAMIC,
					.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
					.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
					.MiscFlags = 0,
					.StructureByteStride = 0
			};
			check_hr_error(g_device->CreateBuffer(&constant_buffer_desc, nullptr, g_vertex_projection_matrix.ReleaseAndGetAddressOf()));
		}

		// pixel
		{
			constexpr char shader_code[]{
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
			if (
				const auto result = D3DCompile(
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
					std::cerr,
					"D3DCompile failed: {} -- at {}:{}",
					static_cast<const char*>(error_message->GetBufferPointer()),
					std::source_location::current().file_name(),
					std::source_location::current().line()
				);
				check_hr_error(result);
			}
			check_hr_error(g_device->CreatePixelShader(
				shader_blob->GetBufferPointer(),
				shader_blob->GetBufferSize(),
				nullptr,
				g_pixel_shader.ReleaseAndGetAddressOf()
			));
		}
	}

	// Create pixel shader texture sampler
	{
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
		check_hr_error(g_device->CreateSamplerState(&sampler_desc, g_font_sampler.ReleaseAndGetAddressOf()));
	}

	// Load default font texture
	{
		const auto& default_font = g_draw_list_shared_data->get_default_font();
		const auto font_data = default_font.texture_data.get();
		const auto font_width = default_font.texture_size.width;
		const auto font_height = default_font.texture_size.height;
		const auto load_font_texture_result = load_texture(reinterpret_cast<const std::uint8_t*>(font_data), font_width, font_height, g_font_texture);
		assert(load_font_texture_result);

		g_draw_list_shared_data->get_default_font().texture_id = reinterpret_cast<draw::font_type::texture_id_type>(g_font_texture.Get());
	}

	// Load additional picture texture
	{
		int image_width;
		int image_height;

		auto* data = stbi_load(ASSETS_PATH_PIC, &image_width, &image_height, nullptr, 4);
		assert(data);

		const auto load_additional_texture_result = load_texture(data, image_width, image_height, g_additional_picture_texture);
		assert(load_additional_texture_result);

		stbi_image_free(data);
	}
}

auto prometheus_new_frame() -> void //
{
	g_draw_list.reset();
	g_draw_list.push_clip_rect({0, 0},{static_cast<float>(g_window_width), static_cast<float>(g_window_height)},false);
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
	g_draw_list.image_rounded(reinterpret_cast<draw::DrawList::texture_id_type>(g_additional_picture_texture.Get()), {900, 350, 1200, 650}, 10);

	#if GAL_PROMETHEUS_DRAW_LIST_DEBUG
	g_draw_list.bind_debug_info();
	#endif
}

auto prometheus_draw() -> void
{
	auto& [this_frame_index_buffer, this_frame_index_count, this_frame_vertex_buffer, this_frame_vertex_count] = render_buffer;

	const auto command_list = g_draw_list.command_list();
	const auto vertex_list = g_draw_list.vertex_list();
	const auto index_list = g_draw_list.index_list();

	if (not this_frame_vertex_buffer or vertex_list.size() > this_frame_vertex_count)
	{
		// todo: grow factor
		this_frame_vertex_count = static_cast<UINT>(vertex_list.size()) + 5000;

		const D3D11_BUFFER_DESC buffer_desc{
				.ByteWidth = static_cast<UINT>(this_frame_vertex_count * sizeof(draw::DrawList::vertex_type)),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_VERTEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
		};
		check_hr_error(g_device->CreateBuffer(&buffer_desc, nullptr, this_frame_vertex_buffer.ReleaseAndGetAddressOf()));
	}
	if (not this_frame_index_buffer or index_list.size() > this_frame_index_count)
	{
		// todo: grow factor
		this_frame_index_count = static_cast<UINT>(index_list.size()) + 10000;

		const D3D11_BUFFER_DESC buffer_desc{
				.ByteWidth = static_cast<UINT>(this_frame_index_count * sizeof(draw::DrawList::index_type)),
				.Usage = D3D11_USAGE_DYNAMIC,
				.BindFlags = D3D11_BIND_INDEX_BUFFER,
				.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
				.MiscFlags = 0,
				.StructureByteStride = 0
		};
		check_hr_error(g_device->CreateBuffer(&buffer_desc, nullptr, this_frame_index_buffer.ReleaseAndGetAddressOf()));
	}

	// Upload vertex/index data into a single contiguous GPU buffer
	{
		D3D11_MAPPED_SUBRESOURCE mapped_vertex_resource;
		D3D11_MAPPED_SUBRESOURCE mapped_index_resource;
		check_hr_error(g_device_immediate_context->Map(this_frame_vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_resource));
		check_hr_error(g_device_immediate_context->Map(this_frame_index_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_index_resource));

		auto* mapped_vertex = static_cast<d3d_vertex_type*>(mapped_vertex_resource.pData);
		auto* mapped_index = static_cast<d3d_index_type*>(mapped_index_resource.pData);

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

		g_device_immediate_context->Unmap(this_frame_vertex_buffer.Get(), 0);
		g_device_immediate_context->Unmap(this_frame_index_buffer.Get(), 0);
	}

	// Setup orthographic projection matrix into our constant buffer
	{
		D3D11_MAPPED_SUBRESOURCE mapped_resource;
		check_hr_error(g_device_immediate_context->Map(g_vertex_projection_matrix.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));

		auto* mapped_projection_matrix = static_cast<d3d_projection_matrix_type*>(mapped_resource.pData);

		constexpr auto left = .0f;
		const auto right = static_cast<float>(g_window_width);
		constexpr auto top = .0f;
		const auto bottom = static_cast<float>(g_window_height);

		const d3d_projection_matrix_type mvp{
				{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
				{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
				{0.0f, 0.0f, 0.5f, 0.0f},
				{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
		};
		std::memcpy(mapped_projection_matrix, &mvp, sizeof(d3d_projection_matrix_type));

		g_device_immediate_context->Unmap(g_vertex_projection_matrix.Get(), 0);
	}

	// Setup viewport
	{
		const D3D11_VIEWPORT viewport{
				.TopLeftX = .0f,
				.TopLeftY = .0f,
				.Width = static_cast<FLOAT>(g_window_width),
				.Height = static_cast<FLOAT>(g_window_height),
				.MinDepth = 0,
				.MaxDepth = 1
		};
		g_device_immediate_context->RSSetViewports(1, &viewport);
	}

	// Bind shader and vertex buffers
	constexpr UINT stride = sizeof(d3d_vertex_type);
	constexpr UINT offset = 0;
	g_device_immediate_context->IASetInputLayout(g_vertex_input_layout.Get());
	g_device_immediate_context->IASetVertexBuffers(0, 1, this_frame_vertex_buffer.GetAddressOf(), &stride, &offset);
	g_device_immediate_context->IASetIndexBuffer(
		this_frame_index_buffer.Get(),
		// ReSharper disable once CppUnreachableCode
		sizeof(d3d_index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
		0
	);
	g_device_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_device_immediate_context->VSSetShader(g_vertex_shader.Get(), nullptr, 0);
	g_device_immediate_context->VSSetConstantBuffers(0, 1, g_vertex_projection_matrix.GetAddressOf());
	g_device_immediate_context->PSSetShader(g_pixel_shader.Get(), nullptr, 0);
	g_device_immediate_context->PSSetSamplers(0, 1, g_font_sampler.GetAddressOf());
	g_device_immediate_context->DSSetShader(nullptr, nullptr, 0);
	g_device_immediate_context->HSSetShader(nullptr, nullptr, 0);
	g_device_immediate_context->GSSetShader(nullptr, nullptr, 0);
	g_device_immediate_context->CSSetShader(nullptr, nullptr, 0);

	// Setup render state
	constexpr float blend_factor[]{0, 0, 0, 0};
	g_device_immediate_context->OMSetBlendState(g_blend_state.Get(), blend_factor, (std::numeric_limits<UINT>::max)());
	g_device_immediate_context->OMSetDepthStencilState(g_depth_stencil_state.Get(), 0);
	g_device_immediate_context->RSSetState(g_rasterizer_state.Get());

	for (const auto& [clip_rect, texture, index_offset, element_count]: command_list)
	{
		const auto [point, extent] = clip_rect;
		const D3D11_RECT rect{static_cast<LONG>(point.x), static_cast<LONG>(point.y), static_cast<LONG>(point.x + extent.width), static_cast<LONG>(point.y + extent.height)};
		g_device_immediate_context->RSSetScissorRects(1, &rect);

		GAL_PROMETHEUS_ERROR_DEBUG_ASSUME(texture != 0, "push_texture_id when create texture view");
		ID3D11ShaderResourceView* textures[]{reinterpret_cast<ID3D11ShaderResourceView*>(texture)}; // NOLINT(performance-no-int-to-ptr)
		g_device_immediate_context->PSSetShaderResources(0, 1, textures);

		// g_device_immediate_context->DrawIndexed(static_cast<UINT>(element_count), static_cast<UINT>(index_offset), 0);
		g_device_immediate_context->DrawIndexedInstanced(static_cast<UINT>(element_count), 1, static_cast<UINT>(index_offset), 0, 0);
	}
}

auto prometheus_shutdown() -> void
{
	print_time();
}
