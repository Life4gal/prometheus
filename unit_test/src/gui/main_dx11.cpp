#include "def.hpp"
#include "dx_error_handler.hpp"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <comdef.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cassert>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
	using namespace gal::prometheus;
	using Microsoft::WRL::ComPtr;
}

ComPtr<ID3D11Device> g_device = nullptr;
ComPtr<ID3D11DeviceContext> g_device_immediate_context = nullptr;

int g_window_width = 1280;
int g_window_height = 960;

double g_last_time = 0;
std::uint64_t g_frame_count = 0;
float g_fps = 0;

auto g_draw_list_shared_data = std::make_shared<gui::DrawListSharedData>();
gui::DrawList g_draw_list;

extern auto prometheus_init() -> void;
extern auto prometheus_new_frame() -> void;
extern auto prometheus_render() -> void;
extern auto prometheus_draw() -> void;
extern auto prometheus_shutdown() -> void;

namespace
{
	int g_window_position_left = 200;
	int g_window_position_top = 200;
	int g_window_resize_width = 0;
	int g_window_resize_height = 0;

	ComPtr<IDXGISwapChain> g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;

	ComPtr<ID3D11RenderTargetView> g_render_target_view = nullptr;

	auto glfw_error_callback(const int error, const char* description) -> void
	{
		std::println("GLFW ERROR({}): {}", error, description);
	}

	auto create_device(GLFWwindow& window) -> bool;
	auto cleanup_device() -> void;

	auto create_render_target() -> void;
	auto cleanup_render_target() -> void;

	auto win32_init(GLFWwindow& window) -> void;
	auto win32_new_frame(GLFWwindow& window) -> void;
	auto win32_shutdown() -> void;

	auto d3d_init() -> void;
	auto d3d_new_frame() -> void;
	auto d3d_shutdown() -> void;
} // namespace

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);

	if (not glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(g_window_width, g_window_height, "GUI Playground Example(DX11)", nullptr, nullptr);
	if (window == nullptr)
	{
		glfwTerminate();
		return -1;
	}

	// Initialize Direct3D
	if (not create_device(*window))
	{
		cleanup_device();
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	const auto range = gui::glyph_range_simplified_chinese_common();
	g_draw_list_shared_data->set_default_font(gui::load_font(R"(C:\Windows\Fonts\msyh.ttc)", 18, range));

	// Setup Platform/Renderer backends
	win32_init(*window);
	d3d_init();
	prometheus_init();

	glfwSetWindowPos(window, g_window_position_left, g_window_position_top);
	glfwShowWindow(window);

	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

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

		win32_new_frame(*window);
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

	win32_shutdown();
	d3d_shutdown();
	prometheus_shutdown();

	cleanup_device();
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

namespace
{
	auto create_device(GLFWwindow& window) -> bool
	{
		print_time();

		// device and swap chain
		{
			const DXGI_SWAP_CHAIN_DESC swap_chain_desc{
					.BufferDesc =
					{
							.Width = 0,
							.Height = 0,
							.RefreshRate = {.Numerator = 60, .Denominator = 1},
							.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
							.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
							.Scaling = DXGI_MODE_SCALING_UNSPECIFIED
					},
					.SampleDesc = {.Count = 1, .Quality = 0},
					.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
					.BufferCount = 2,
					.OutputWindow = glfwGetWin32Window(&window),
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
					static_cast<UINT>(std::ranges::size(feature_levels)),
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
						static_cast<UINT>(std::ranges::size(feature_levels)),
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
		print_time();

		cleanup_render_target();

		// com ptr
		// do nothing here
	}

	auto create_render_target() -> void
	{
		print_time();

		ComPtr<ID3D11Texture2D> back_buffer;
		check_hr_error(g_swap_chain->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())));
		check_hr_error(g_device->CreateRenderTargetView(back_buffer.Get(), nullptr, g_render_target_view.ReleaseAndGetAddressOf()));
	}

	auto cleanup_render_target() -> void //
	{
		print_time();

		g_render_target_view.Reset();
	}

	auto win32_init(GLFWwindow& window) -> void //
	{
		print_time();

		glfwSetFramebufferSizeCallback(
			&window,
			// ReSharper disable once CppParameterMayBeConstPtrOrRef
			[](GLFWwindow* w, const int width, const int height) -> void
			{
				(void)w;
				g_window_resize_width = width;
				g_window_resize_height = height;
			}
		);

		g_last_time = glfwGetTime();
	}

	auto win32_new_frame(GLFWwindow& window) -> void
	{
		glfwGetWindowPos(&window, &g_window_position_left, &g_window_position_top);
		glfwGetFramebufferSize(&window, &g_window_width, &g_window_height);

		const auto current_time = glfwGetTime();
		const auto elapsed = current_time - g_last_time;
		g_frame_count += 1;

		if (elapsed > .5f)
		{
			g_fps = static_cast<float>(static_cast<double>(g_frame_count) / elapsed);
			g_frame_count = 0;
			g_last_time = current_time;
		}
	}

	auto win32_shutdown() -> void
	{
		print_time();
	}

	auto d3d_init() -> void //
	{
		print_time();
	}

	auto d3d_new_frame() -> void
	{
		//
	}

	auto d3d_shutdown() -> void //
	{
		print_time();
	}
}
