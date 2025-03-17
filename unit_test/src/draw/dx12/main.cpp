#include "../win/def.hpp"
#include "../common/print_time.hpp"

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <cassert>

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

using Microsoft::WRL::ComPtr;

extern constexpr std::size_t num_frames_in_flight = 3;

ComPtr<ID3D12Device> g_device = nullptr;
ComPtr<ID3D12GraphicsCommandList> g_command_list = nullptr;

int g_window_width = 1280;
int g_window_height = 960;

double g_last_time = 0;
std::uint64_t g_frame_count = 0;
float g_fps = 0;

extern auto glfw_callback_setup(GLFWwindow& w) -> void;

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

	struct frame_context_type
	{
		ComPtr<ID3D12CommandAllocator> command_allocator;
		UINT64 fence_value;
	};

	constexpr std::size_t num_back_buffers = 3;

	ComPtr<ID3D12DescriptorHeap> g_render_target_view_descriptor_heap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE g_render_target_descriptor[num_back_buffers] = {};
	ComPtr<ID3D12Resource> g_render_target_resource[num_back_buffers] = {};

	ComPtr<ID3D12CommandQueue> g_command_queue = nullptr;

	// note: overflow(max + 1 => 0)
	UINT g_frame_index = (std::numeric_limits<UINT>::max)();
	frame_context_type g_frame_context[num_frames_in_flight] = {};

	ComPtr<ID3D12Fence> g_fence = nullptr;
	HANDLE g_fence_event = nullptr;
	UINT64 g_fence_last_signaled_value = 0;

	ComPtr<IDXGISwapChain3> g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;
	HANDLE g_swap_chain_waitable_object = nullptr;

	auto glfw_error_callback(const int error, const char* description) -> void
	{
		std::println(stderr, "GLFW ERROR({}): {}", error, description);
	}

	auto create_device(GLFWwindow& window) -> bool;
	auto cleanup_device() -> void;

	auto create_render_target() -> void;
	auto cleanup_render_target() -> void;

	auto wait_for_last_submitted_frame() -> void;
	auto wait_for_next_frame_resources() -> frame_context_type&;

	auto win32_init(GLFWwindow& window) -> void;
	auto win32_new_frame(GLFWwindow& window) -> void;
	auto win32_shutdown() -> void;

	auto d3d_init() -> void;
	auto d3d_new_frame() -> void;
	auto d3d_shutdown() -> void;
}

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);

	if (not glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(g_window_width, g_window_height, "GUI Playground Example(DX12)", nullptr, nullptr);
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
		return 1;
	}

	// Setup Platform/Renderer backends
	win32_init(*window);
	d3d_init();
	prometheus_init();

	glfw_callback_setup(*window);

	glfwSetWindowPos(window, g_window_position_left, g_window_position_top);
	glfwShowWindow(window);

	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Handle window screen locked
		if (g_swap_chain_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			Sleep(10);
			continue;
		}
		g_swap_chain_occluded = false;

		// Handle window resize (we don't resize directly in the WM_SIZE handler)
		if (g_window_resize_width != 0 and g_window_resize_height != 0)
		{
			wait_for_last_submitted_frame();
			cleanup_render_target();
			check_hr_error(g_swap_chain->ResizeBuffers(0, g_window_resize_width, g_window_resize_height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT));
			g_window_resize_width = 0;
			g_window_resize_height = 0;
			create_render_target();
		}

		win32_new_frame(*window);
		d3d_new_frame();
		prometheus_new_frame();

		// Rendering
		prometheus_render();

		auto& [frame_command_allocator, frame_fence_value] = wait_for_next_frame_resources();
		const auto back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();
		check_hr_error(frame_command_allocator->Reset());

		D3D12_RESOURCE_BARRIER barrier{
				.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
				.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
				.Transition =
				{
						.pResource = g_render_target_resource[back_buffer_index].Get(),
						.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
						.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
						.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
				}
		};
		check_hr_error(g_command_list->Reset(frame_command_allocator.Get(), nullptr));
		g_command_list->ResourceBarrier(1, &barrier);

		constexpr float clear_color_with_alpha[4]{.45f, .55f, .6f, 1.f};

		g_command_list->ClearRenderTargetView(g_render_target_descriptor[back_buffer_index], clear_color_with_alpha, 0, nullptr);
		g_command_list->OMSetRenderTargets(1, &g_render_target_descriptor[back_buffer_index], FALSE, nullptr);

		prometheus_draw();

		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		g_command_list->ResourceBarrier(1, &barrier);
		check_hr_error(g_command_list->Close());

		ID3D12CommandList* command_lists[]{g_command_list.Get()};
		g_command_queue->ExecuteCommandLists(1, command_lists);

		// Present
		const auto hr = g_swap_chain->Present(1, 0); // Present with vsync
		// const auto hr = g_pSwapChain->Present(0, 0); // Present without vsync
		g_swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);

		const auto fence_value = g_fence_last_signaled_value + 1;
		(void)g_command_queue->Signal(g_fence.Get(), fence_value);
		g_fence_last_signaled_value = fence_value;
		frame_fence_value = fence_value;
	}

	wait_for_last_submitted_frame();

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
		ComPtr<ID3D12Debug> dx12_debug = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(dx12_debug.GetAddressOf()))))
		{
			dx12_debug->EnableDebugLayer();
		}
		#endif

		// Create device
		constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
		check_hr_error(D3D12CreateDevice(nullptr, feature_level, IID_PPV_ARGS(g_device.GetAddressOf())));

		#ifdef DX12_ENABLE_DEBUG_LAYER
		if (dx12_debug != nullptr)
		{
			// [DEBUG] Setup debug interface to break on any warnings/errors
			if (ComPtr<ID3D12InfoQueue> info_queue = nullptr;
				SUCCEEDED(g_device->QueryInterface(IID_PPV_ARGS(info_queue.GetAddressOf()))))
			{
				(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
				(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
				(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			}

			// todo
			// `g_device->CreateDescriptorHeap` failed
			// D3D12: Removing Device.
			// D3D12 MESSAGE:
			//	ID3D12Device::RemoveDevice: Device removal has been triggered for the following reason (DXGI_ERROR_DEVICE_RESET:
			//		The hardware took an unreasonable amount of time to execute a command on a different Device Context, or the hardware crashed/hung.
			//		As a result, the TDR (Timeout Detection and Recovery) mechanism has been triggered.
			//		The current Device Context was NOT executing commands when the hang occurred. However, the current video memory and Device Context could not be completely recovered.
			//		The application may want to just respawn itself, as the other application may no longer be around to cause this again).
			//		[ EXECUTION MESSAGE #234: DEVICE_REMOVAL_PROCESS_NOT_AT_FAULT]
			// Enable GPU-based validation (optional but useful)
			// if (ComPtr<ID3D12Debug1> dx12_debug1 = nullptr;
			// 	SUCCEEDED(dx12_debug->QueryInterface(IID_PPV_ARGS(dx12_debug1.GetAddressOf()))))
			// {
			// 	dx12_debug1->SetEnableGPUBasedValidation(true);
			// }
		}

		if (ComPtr<IDXGIInfoQueue> dxgi_info_queue;
			SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_info_queue.GetAddressOf()))))
		{
			(void)dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			(void)dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
			(void)dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
		}
		#endif

		{
			constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
					.NumDescriptors = num_back_buffers,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
					.NodeMask = 1
			};
			if (const auto result = g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(g_render_target_view_descriptor_heap.GetAddressOf()));
				result != S_OK)
			{
				check_hr_error(g_device->GetDeviceRemovedReason());
				check_hr_error(result);

				return false;
			}

			const auto rtv_descriptor_size = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_render_target_view_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
			for (auto& h: g_render_target_descriptor)
			{
				h = rtv_handle;
				rtv_handle.ptr += rtv_descriptor_size;
			}
		}

		{
			constexpr D3D12_COMMAND_QUEUE_DESC desc{
					.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
					.Priority = 0,
					.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
					.NodeMask = 1
			};
			check_hr_error(g_device->CreateCommandQueue(&desc, IID_PPV_ARGS(g_command_queue.GetAddressOf())));
		}

		for (auto& [command_allocator, _]: g_frame_context)
		{
			check_hr_error(
				g_device->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(command_allocator.GetAddressOf())
				)
			);
		}

		check_hr_error(
			g_device->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				g_frame_context[0].command_allocator.Get(),
				nullptr,
				IID_PPV_ARGS(g_command_list.GetAddressOf())
			)
		);
		check_hr_error(g_command_list->Close());

		check_hr_error(g_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(g_fence.GetAddressOf())));

		g_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (g_fence_event == nullptr)
		{
			return false;
		}

		ComPtr<IDXGIFactory4> dxgi_factory;
		ComPtr<IDXGISwapChain1> swap_chain1;
		check_hr_error(CreateDXGIFactory1(IID_PPV_ARGS(dxgi_factory.GetAddressOf())));
		check_hr_error(dxgi_factory->CreateSwapChainForHwnd(g_command_queue.Get(), glfwGetWin32Window(&window), &swap_chain_desc, nullptr, nullptr, swap_chain1.GetAddressOf()));
		check_hr_error(swap_chain1->QueryInterface(IID_PPV_ARGS(g_swap_chain.GetAddressOf())));
		check_hr_error(g_swap_chain->SetMaximumFrameLatency(num_back_buffers));
		g_swap_chain_waitable_object = g_swap_chain->GetFrameLatencyWaitableObject();

		create_render_target();
		return true;
	}

	auto cleanup_device() -> void
	{
		print_time();

		cleanup_render_target();
		if (g_swap_chain)
		{
			(void)g_swap_chain->SetFullscreenState(false, nullptr);
		}
		if (g_swap_chain_waitable_object != nullptr)
		{
			CloseHandle(g_swap_chain_waitable_object);
		}

		// #ifdef DX12_ENABLE_DEBUG_LAYER
		// if (ComPtr<IDXGIDebug1> dxgi_debug = nullptr;
		// 	SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
		// {
		// 	(void)dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
		// }
		// #endif
	}

	auto create_render_target() -> void
	{
		print_time();

		for (UINT i = 0; i < num_back_buffers; i++)
		{
			ComPtr<ID3D12Resource> back_buffer = nullptr;
			check_hr_error(g_swap_chain->GetBuffer(i, IID_PPV_ARGS(back_buffer.GetAddressOf())));
			g_device->CreateRenderTargetView(back_buffer.Get(), nullptr, g_render_target_descriptor[i]);
			g_render_target_resource[i] = back_buffer;
		}
	}

	auto cleanup_render_target() -> void
	{
		print_time();

		wait_for_last_submitted_frame();

		for (auto& resource: g_render_target_resource)
		{
			// ComPtr
			resource = nullptr;
		}
	}

	auto wait_for_last_submitted_frame() -> void
	{
		auto& frame_context = g_frame_context[g_frame_index % num_frames_in_flight];

		const auto fence_value = frame_context.fence_value;
		if (fence_value == 0)
		{
			// No fence was signaled
			return;
		}

		frame_context.fence_value = 0;
		if (g_fence->GetCompletedValue() >= fence_value)
		{
			return;
		}

		(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
		WaitForSingleObject(g_fence_event, INFINITE);
	}

	auto wait_for_next_frame_resources() -> frame_context_type&
	{
		g_frame_index += 1;

		HANDLE waitable_objects[] = {g_swap_chain_waitable_object, nullptr};
		DWORD num_waitable_objects = 1;

		auto& frame_context = g_frame_context[g_frame_index % num_frames_in_flight];
		if (const auto fence_value = frame_context.fence_value;
			// means no fence was signaled
			fence_value != 0)
		{
			frame_context.fence_value = 0;
			(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
			waitable_objects[1] = g_fence_event;
			num_waitable_objects = 2;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects, TRUE, INFINITE);

		return frame_context;
	}

	auto win32_init(GLFWwindow& window) -> void
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

	auto d3d_init() -> void
	{
		print_time();
	}

	auto d3d_new_frame() -> void
	{
		//
	}

	auto d3d_shutdown() -> void
	{
		print_time();
	}
}
