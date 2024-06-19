#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <comdef.h>

#include "font.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
	using namespace gal::prometheus;
	using Microsoft::WRL::ComPtr;
	using const_window_type = std::add_const_t<HWND>;

	struct d3d_vertex_type
	{
		float position[2];
		float uv[2];
		std::uint32_t color;
	};

	using d3d_vertex_index_type = std::uint16_t;

	using d3d_constant_buffer_type = float[4][4];

	LONG g_window_position_left = 100;
	LONG g_window_position_top = 100;
	LONG g_window_width = 1280;
	LONG g_window_height = 960;
	LONG g_window_resize_width = 0;
	LONG g_window_resize_height = 0;

	ComPtr<ID3D11Device> g_device = nullptr;
	ComPtr<ID3D11DeviceContext> g_device_immediate_context = nullptr;
	ComPtr<IDXGISwapChain> g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;

	ComPtr<ID3D11RenderTargetView> g_render_target_view = nullptr;

	ComPtr<ID3D11VertexShader> g_vertex_shader = nullptr;
	ComPtr<ID3D11InputLayout> g_vertex_input_layout = nullptr;
	ComPtr<ID3D11Buffer> g_vertex_constant_buffer = nullptr;

	ComPtr<ID3D11PixelShader> g_pixel_shader = nullptr;

	ComPtr<ID3D11BlendState> g_blend_state = nullptr;
	ComPtr<ID3D11RasterizerState> g_rasterizer_state = nullptr;
	ComPtr<ID3D11DepthStencilState> g_depth_stencil_state = nullptr;

	ComPtr<ID3D11ShaderResourceView> g_font_texture_view = nullptr;
	ComPtr<ID3D11SamplerState> g_font_sampler = nullptr;

	ComPtr<ID3D11Buffer> g_vertex_buffer = nullptr;
	ComPtr<ID3D11Buffer> g_index_buffer = nullptr;
	std::size_t g_vertex_buffer_size = 0;
	std::size_t g_index_buffer_size = 0;

	template<bool Abort = true>
	auto check_hr_error(
		const HRESULT hr,
		const std::source_location& location = std::source_location::current()
	) -> std::conditional_t<Abort, void, bool>
	{
		if (SUCCEEDED(hr))
		{
			if constexpr (Abort)
			{
				return;
			}
			else
			{
				return true;
			}
		}

		const _com_error err(hr);
		std::println(std::cerr, "Error: {} --- at {}:{}", err.ErrorMessage(), location.file_name(), location.line());

		if constexpr (Abort)
		{
			std::abort();
		}
		else
		{
			return false;
		}
	}

	auto create_device(const_window_type window) -> bool;
	auto cleanup_device() -> void;

	auto create_render_target() -> void;
	auto cleanup_render_target() -> void;

	auto win32_init(const_window_type window) -> void;
	auto win32_new_frame(const_window_type window) -> void;
	auto win32_shutdown() -> void;

	auto d3d_init() -> void;
	auto d3d_new_frame() -> void;
	auto d3d_shutdown() -> void;

	auto prometheus_init() -> void;
	auto prometheus_new_frame() -> void;
	auto prometheus_render() -> void;
	auto prometheus_draw() -> void;
	auto prometheus_shutdown() -> void;

	auto WINAPI window_procedure(
		const_window_type window,
		const UINT message,
		const WPARAM w_param,
		const LPARAM l_param
	) -> LRESULT
	{
		switch (message)
		{
			case WM_SIZE:
			{
				if (w_param != SIZE_MINIMIZED)
				{
					g_window_resize_width = static_cast<LONG>(LOWORD(l_param));
					g_window_resize_height = static_cast<LONG>(HIWORD(l_param));
					return 0;
				}
			}
			case WM_DESTROY:
			{
				cleanup_device();
				PostQuitMessage(0);
				return 0;
			}
			default:
			{
				return DefWindowProc(window, message, w_param, l_param);
			}
		}
	}
} // namespace

int main(int, char**)
{
	// Register the window class
	const WNDCLASSEX window_class{
			.cbSize = sizeof(WNDCLASSEX),
			.style = CS_CLASSDC,
			.lpfnWndProc = window_procedure,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = GetModuleHandle(nullptr),
			.hIcon = nullptr,
			.hCursor = LoadCursor(nullptr, IDC_ARROW),
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = "GUI Playground",
			.hIconSm = nullptr
	};
	RegisterClassEx(&window_class);

	// Create the application's window
	const auto window = CreateWindow(
		window_class.lpszClassName,
		"GUI Playground Example",
		WS_OVERLAPPEDWINDOW,
		g_window_position_left,
		g_window_position_top,
		g_window_width,
		g_window_height,
		NULL,
		NULL,
		window_class.hInstance,
		NULL
	);

	// Initialize Direct3D
	if (not create_device(window))
	{
		cleanup_device();
		UnregisterClass(window_class.lpszClassName, window_class.hInstance);
		return -1;
	}

	// Show the window
	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	// Setup Platform/Renderer backends
	win32_init(window);
	d3d_init();
	prometheus_init();

	// Main loop
	{
		bool done = false;
		while (not done)
		{
			MSG message;
			while (PeekMessage(&message, nullptr, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&message);
				DispatchMessage(&message);
				if (message.message == WM_QUIT)
				{
					done = true;
				}
			}
			if (done)
			{
				break;
			}

			// Handle window being minimized or screen locked
			if (g_swap_chain_occluded and g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
			{
				Sleep(10);
				continue;
			}
			g_swap_chain_occluded = false;

			// Handle window resize (we don't resize directly in the WM_SIZE handler)
			if (g_window_resize_width != 0 and g_window_resize_height != 0)
			{
				cleanup_render_target();
				check_hr_error(g_swap_chain->ResizeBuffers(0, g_window_resize_width, g_window_resize_height, DXGI_FORMAT_UNKNOWN, 0));
				g_window_resize_width = 0;
				g_window_resize_height = 0;
				create_render_target();
			}

			win32_new_frame(window);
			d3d_new_frame();
			prometheus_new_frame();

			// Rendering
			prometheus_render();
			constexpr float clear_color_value[]{.45f, .55f, .65f, 1.f};
			g_device_immediate_context->OMSetRenderTargets(1, g_render_target_view.GetAddressOf(), nullptr);
			g_device_immediate_context->ClearRenderTargetView(g_render_target_view.Get(), clear_color_value);
			prometheus_draw();

			// Present with vsync
			const auto result = g_swap_chain->Present(1, 0);
			// Present without vsync
			// const auto result = g_swap_chain->Present(0, 0);
			check_hr_error<false>(result);
			g_swap_chain_occluded = result == DXGI_STATUS_OCCLUDED;
		}
	}

	win32_shutdown();
	d3d_shutdown();
	prometheus_shutdown();

	cleanup_device();
	DestroyWindow(window);
	UnregisterClass(window_class.lpszClassName, window_class.hInstance);

	return 0;
}

namespace
{
	auto create_device(const_window_type window) -> bool
	{
		// device and swap chain
		{
			const DXGI_SWAP_CHAIN_DESC swap_chain_desc{
					.BufferDesc =
					{.Width = 0,
					 .Height = 0,
					 .RefreshRate = {.Numerator = 60, .Denominator = 1},
					 .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
					 .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
					 .Scaling = DXGI_MODE_SCALING_UNSPECIFIED},
					.SampleDesc = {.Count = 1, .Quality = 0},
					.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
					.BufferCount = 2,
					.OutputWindow = window,
					.Windowed = TRUE,
					.SwapEffect = DXGI_SWAP_EFFECT_DISCARD,
					.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
			};

			constexpr UINT create_device_flags
			{
					#if defined(DEBUG) or defined(_DEBUG)
					D3D11_CREATE_DEVICE_DEBUG
					#endif
			};
			constexpr D3D_FEATURE_LEVEL feature_levels[]{
					D3D_FEATURE_LEVEL_11_0,
					D3D_FEATURE_LEVEL_10_0,
			};

			D3D_FEATURE_LEVEL feature_level;
			if (const auto result = D3D11CreateDeviceAndSwapChain(
					nullptr,
					D3D_DRIVER_TYPE_HARDWARE,
					nullptr,
					create_device_flags,
					feature_levels,
					2,
					D3D11_SDK_VERSION,
					&swap_chain_desc,
					g_swap_chain.ReleaseAndGetAddressOf(),
					g_device.ReleaseAndGetAddressOf(),
					&feature_level,
					g_device_immediate_context.ReleaseAndGetAddressOf()
				);
				result == DXGI_ERROR_UNSUPPORTED)
			{
				// Try high-performance WARP software driver if hardware is not available.
				if (not check_hr_error<false>(
					D3D11CreateDeviceAndSwapChain(
						nullptr,
						D3D_DRIVER_TYPE_WARP,
						nullptr,
						create_device_flags,
						feature_levels,
						2,
						D3D11_SDK_VERSION,
						&swap_chain_desc,
						g_swap_chain.ReleaseAndGetAddressOf(),
						g_device.ReleaseAndGetAddressOf(),
						&feature_level,
						g_device_immediate_context.ReleaseAndGetAddressOf()
					)
				))
				{
					return false;
				}
			}
		}

		// create render target
		create_render_target();

		return true;
	}

	auto cleanup_device() -> void
	{
		cleanup_render_target();

		// com ptr
		// do nothing here
	}

	auto create_render_target() -> void
	{
		ID3D11Texture2D* back_buffer;
		check_hr_error(g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)));
		check_hr_error(g_device->CreateRenderTargetView(back_buffer, nullptr, g_render_target_view.ReleaseAndGetAddressOf()));
		back_buffer->Release();
	}

	auto cleanup_render_target() -> void //
	{
		g_render_target_view.Reset();
	}

	auto win32_init(const_window_type window) -> void //
	{
		(void)window;
	}

	auto win32_new_frame(const_window_type window) -> void
	{
		RECT rect;
		GetClientRect(window, &rect);
		g_window_position_left = rect.left;
		g_window_position_top = rect.top;
		g_window_width = rect.right - rect.left;
		g_window_height = rect.bottom - rect.top;
	}

	auto win32_shutdown() -> void
	{
		// 
	}

	auto d3d_destroy_device_objects() -> void
	{
		// com ptr
		// do nothing here
	}

	auto d3d_create_device_objects() -> void
	{
		if (g_font_sampler)
		{
			d3d_destroy_device_objects();
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
						"	output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));"
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
						{.SemanticName = "POSITION",
						 .SemanticIndex = 0,
						 .Format = DXGI_FORMAT_R32G32_FLOAT,
						 .InputSlot = 0,
						 .AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, position)),
						 .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						 .InstanceDataStepRate = 0},
						{.SemanticName = "COLOR",
						 .SemanticIndex = 0,
						 .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
						 .InputSlot = 0,
						 .AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, color)),
						 .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						 .InstanceDataStepRate = 0},
						{.SemanticName = "TEXCOORD",
						 .SemanticIndex = 0,
						 .Format = DXGI_FORMAT_R32G32_FLOAT,
						 .InputSlot = 0,
						 .AlignedByteOffset = static_cast<UINT>(offsetof(d3d_vertex_type, uv)),
						 .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
						 .InstanceDataStepRate = 0},
				};
				check_hr_error(g_device->CreateInputLayout(
					input_element_desc,
					std::ranges::size(input_element_desc),
					shader_blob->GetBufferPointer(),
					shader_blob->GetBufferSize(),
					g_vertex_input_layout.ReleaseAndGetAddressOf()
				));

				// constant buffer
				constexpr D3D11_BUFFER_DESC constant_buffer_desc{
						.ByteWidth = sizeof(d3d_constant_buffer_type),
						.Usage = D3D11_USAGE_DYNAMIC,
						.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
						.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
						.MiscFlags = 0,
						.StructureByteStride = 0
				};
				check_hr_error(g_device->CreateBuffer(&constant_buffer_desc, nullptr, g_vertex_constant_buffer.ReleaseAndGetAddressOf()));
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
					{.StencilFailOp = D3D11_STENCIL_OP_KEEP,
					 .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
					 .StencilPassOp = D3D11_STENCIL_OP_KEEP,
					 .StencilFunc = D3D11_COMPARISON_ALWAYS},
					.BackFace =
					{.StencilFailOp = D3D11_STENCIL_OP_KEEP,
					 .StencilDepthFailOp = D3D11_STENCIL_OP_KEEP,
					 .StencilPassOp = D3D11_STENCIL_OP_KEEP,
					 .StencilFunc = D3D11_COMPARISON_ALWAYS}
			};

			check_hr_error(g_device->CreateDepthStencilState(&depth_stencil_desc, g_depth_stencil_state.ReleaseAndGetAddressOf()));
		}

		// Create font texture
		{
			const auto [pixels, width, height] = load_font();
			assert(pixels);

			const D3D11_TEXTURE2D_DESC texture_2d_desc{
					.Width = static_cast<UINT>(width),
					.Height = static_cast<UINT>(height),
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
					.pSysMem = pixels,
					.SysMemPitch = static_cast<UINT>(width) * 4,
					.SysMemSlicePitch = 0
			};

			ComPtr<ID3D11Texture2D> texture;
			check_hr_error(g_device->CreateTexture2D(&texture_2d_desc, &subresource_data, texture.GetAddressOf()));

			// Create texture view
			const D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{
					.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
					.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
					.Texture2D = {.MostDetailedMip = 0, .MipLevels = texture_2d_desc.MipLevels}
			};
			check_hr_error(
				g_device->CreateShaderResourceView(texture.Get(), &shader_resource_view_desc, g_font_texture_view.ReleaseAndGetAddressOf())
			);

			// Create texture sampler
			constexpr D3D11_SAMPLER_DESC sampler_desc{
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
	}

	auto d3d_init() -> void //
	{}

	auto d3d_new_frame() -> void
	{
		if (not g_font_sampler)
		{
			d3d_create_device_objects();
		}
	}

	auto d3d_shutdown() -> void //
	{
		d3d_destroy_device_objects();
	}

	using point_type = primitive::basic_point<float, 2>;
	using vertex_type = primitive::basic_vertex<point_type>;
	using vertex_list_type = primitive::basic_vertex_list<vertex_type>;
	using vertex_index_type = d3d_vertex_index_type;
	using vertex_index_list_type = std::vector<vertex_index_type>;

	static_assert(sizeof(d3d_vertex_type) == sizeof(vertex_type));

	vertex_list_type g_vertex_list;
	vertex_index_list_type g_vertex_index_list;

	auto prometheus_init() -> void //
	{}

	auto prometheus_new_frame() -> void //
	{
		g_vertex_list.clear();
		g_vertex_index_list.clear();
	}

	auto prometheus_render() -> void
	{
		// todo: fill index

		g_vertex_list.triangle({100, 100}, {150, 150}, {200, 100}, primitive::colors::blue);
		g_vertex_list.rect_filled({150, 150}, {200, 200}, primitive::colors::gold);
		g_vertex_list.rect_filled({200, 200}, {300, 300}, primitive::colors::red);

		constexpr vertex_list_type::rect_type rect{vertex_list_type::point_type{300, 300}, vertex_list_type::extent_type{200, 200}};
		g_vertex_list.rect(rect, primitive::colors::light_pink);
		g_vertex_list.circle(primitive::inscribed_circle(rect), primitive::colors::orange);
		g_vertex_list.circle(primitive::circumscribed_circle(rect), primitive::colors::orange);

		g_vertex_list.circle_filled({100, 400}, 100, primitive::colors::red);

		g_vertex_list.arc<primitive::ArcQuadrant::Q1>({400.f, 150.f}, 80, primitive::colors::red);
		g_vertex_list.arc_filled<primitive::ArcQuadrant::Q2>({400.f, 150.f}, 60, primitive::colors::green);
		g_vertex_list.arc<primitive::ArcQuadrant::Q3>({400.f, 150.f}, 40, primitive::colors::blue);
		g_vertex_list.arc_filled<primitive::ArcQuadrant::Q4>({400.f, 150.f}, 20, primitive::colors::yellow);
		g_vertex_list.circle_filled({400, 150}, 10, primitive::colors::gold);

		{
			constexpr decltype(primitive::colors::red) colors[]{
					primitive::colors::red,
					primitive::colors::blue,
					primitive::colors::green,
					primitive::colors::yellow,
					primitive::colors::pink,
			};

			static int tick = 0;
			const auto index = tick % std::ranges::size(colors);

			g_vertex_list.triangle({50, 50}, {75, 75}, {100, 50}, colors[index]);
			tick += 1;
		}
	}

	auto prometheus_draw() -> void
	{
		if (g_window_width <= 0 or g_window_height <= 0)
		{
			return;
		}

		if (not g_vertex_buffer or g_vertex_list.size() > g_vertex_buffer_size)
		{
			// todo: grow factor
			g_vertex_buffer_size = g_vertex_list.size() + 5000;

			const D3D11_BUFFER_DESC buffer_desc{
					.ByteWidth = static_cast<UINT>(g_vertex_buffer_size * sizeof(vertex_type)),
					.Usage = D3D11_USAGE_DYNAMIC,
					.BindFlags = D3D11_BIND_VERTEX_BUFFER,
					.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
					.MiscFlags = 0,
					.StructureByteStride = 0
			};
			check_hr_error(g_device->CreateBuffer(&buffer_desc, nullptr, g_vertex_buffer.ReleaseAndGetAddressOf()));
		}
		if (not g_index_buffer or g_vertex_index_list.size() > g_index_buffer_size)
		{
			// todo: grow factor
			g_index_buffer_size = g_vertex_index_list.size() + 10000;

			const D3D11_BUFFER_DESC buffer_desc{
					.ByteWidth = static_cast<UINT>(g_index_buffer_size * sizeof(vertex_index_type)),
					.Usage = D3D11_USAGE_DYNAMIC,
					.BindFlags = D3D11_BIND_INDEX_BUFFER,
					.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
					.MiscFlags = 0,
					.StructureByteStride = 0
			};
			check_hr_error(g_device->CreateBuffer(&buffer_desc, nullptr, g_index_buffer.ReleaseAndGetAddressOf()));
		}

		// Upload vertex/index data into a single contiguous GPU buffer
		{
			D3D11_MAPPED_SUBRESOURCE mapped_vertex_resource;
			D3D11_MAPPED_SUBRESOURCE mapped_index_resource;
			check_hr_error(g_device_immediate_context->Map(g_vertex_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_resource));
			check_hr_error(g_device_immediate_context->Map(g_index_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_index_resource));

			auto* mapped_vertex = static_cast<d3d_vertex_type*>(mapped_vertex_resource.pData);
			auto* mapped_index = static_cast<d3d_vertex_index_type*>(mapped_index_resource.pData);
			std::ranges::transform(
				g_vertex_list.vertices(),
				mapped_vertex,
				[](const vertex_type& vertex) -> d3d_vertex_type
				{
					return {.position = {vertex.position.x, vertex.position.y},
					        .uv = {vertex.uv.x, vertex.uv.y},
					        .color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)};
				}
			);
			std::ranges::copy(g_vertex_index_list, mapped_index);

			g_device_immediate_context->Unmap(g_vertex_buffer.Get(), 0);
			g_device_immediate_context->Unmap(g_index_buffer.Get(), 0);
		}

		// Setup orthographic projection matrix into our constant buffer
		{
			D3D11_MAPPED_SUBRESOURCE mapped_resource;
			check_hr_error(g_device_immediate_context->Map(g_vertex_constant_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource));

			auto* mapped_constant_buffer = static_cast<d3d_constant_buffer_type*>(mapped_resource.pData);

			const auto left = static_cast<float>(g_window_position_left);
			const auto right = static_cast<float>(g_window_position_left + g_window_width);
			const auto top = static_cast<float>(g_window_position_top);
			const auto bottom = static_cast<float>(g_window_position_top + g_window_height);

			const d3d_constant_buffer_type mvp{
					{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
					{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
					{0.0f, 0.0f, 0.5f, 0.0f},
					{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
			};
			std::memcpy(mapped_constant_buffer, &mvp, sizeof(d3d_constant_buffer_type));

			g_device_immediate_context->Unmap(g_vertex_constant_buffer.Get(), 0);
		}

		// Setup viewport
		{
			const D3D11_VIEWPORT viewport{
					.TopLeftX = static_cast<FLOAT>(g_window_position_left),
					.TopLeftY = static_cast<FLOAT>(g_window_position_top),
					.Width = static_cast<FLOAT>(g_window_width),
					.Height = static_cast<FLOAT>(g_window_height),
					.MinDepth = 0,
					.MaxDepth = 1
			};
			g_device_immediate_context->RSSetViewports(1, &viewport);
		}

		// todo
		if (static bool once = false;
			not once)
		{
			once = true;

			// Bind shader and vertex buffers
			constexpr UINT stride = sizeof(d3d_vertex_type);
			constexpr UINT offset = 0;
			g_device_immediate_context->IASetInputLayout(g_vertex_input_layout.Get());
			g_device_immediate_context->IASetVertexBuffers(0, 1, g_vertex_buffer.GetAddressOf(), &stride, &offset);
			g_device_immediate_context->IASetIndexBuffer(
				g_index_buffer.Get(),
				// ReSharper disable once CppUnreachableCode
				sizeof(d3d_vertex_index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
				0
			);
			g_device_immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_device_immediate_context->VSSetShader(g_vertex_shader.Get(), nullptr, 0);
			g_device_immediate_context->VSSetConstantBuffers(0, 1, g_vertex_constant_buffer.GetAddressOf());
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

			g_device_immediate_context->PSSetShaderResources(0, 1, g_font_texture_view.GetAddressOf());
		}

		// todo
		g_device_immediate_context->Draw(static_cast<UINT>(g_vertex_list.size()), 0);
		// g_device_immediate_context->DrawIndexed(static_cast<UINT>(g_vertex_index_list.size()), 0, 0);
	}

	auto prometheus_shutdown() -> void //
	{}
}
