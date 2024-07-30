#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <comdef.h>

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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

	struct d3d_frame_context_type
	{
		ComPtr<ID3D12CommandAllocator> command_allocator;
		UINT64 fence_value;
	};

	struct d3d_render_buffer_type
	{
		ComPtr<ID3D12Resource> index;
		UINT index_count;
		ComPtr<ID3D12Resource> vertex;
		UINT vertex_count;
	};

	constexpr std::size_t num_frames_in_flight = 3;
	constexpr std::size_t num_back_buffers = 3;

	// note: overflow(max + 1 => 0)
	UINT g_frame_index = (std::numeric_limits<UINT>::max)();
	d3d_frame_context_type g_frame_context[num_frames_in_flight] = {};

	// note: overflow(max + 1 => 0)
	UINT g_frame_resource_index = (std::numeric_limits<UINT>::max)();
	d3d_render_buffer_type g_frame_resource[num_frames_in_flight] = {};

	ComPtr<ID3D12Device> g_device = nullptr;
	ComPtr<ID3D12DescriptorHeap> g_render_target_view_descriptor_heap = nullptr;
	ComPtr<ID3D12DescriptorHeap> g_shader_resource_view_descriptor_heap = nullptr;
	ComPtr<ID3D12CommandQueue> g_command_queue = nullptr;
	ComPtr<ID3D12GraphicsCommandList> g_command_list = nullptr;
	ComPtr<ID3D12Fence> g_fence = nullptr;
	HANDLE g_fence_event = nullptr;
	UINT64 g_fence_last_signaled_value = 0;
	ComPtr<IDXGISwapChain3> g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;
	HANDLE g_swap_chain_waitable_object = nullptr;
	ComPtr<ID3D12Resource> g_main_render_target_resource[num_back_buffers] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE g_main_render_target_descriptor[num_back_buffers] = {};

	ComPtr<ID3D12RootSignature> g_root_signature = nullptr;
	ComPtr<ID3D12PipelineState> g_pipeline_state = nullptr;
	DXGI_FORMAT g_render_target_view_format = DXGI_FORMAT_R8G8B8A8_UNORM;

	auto g_draw_list_shared_data = std::make_shared<gui::DrawListSharedData>();

	ComPtr<ID3D12Resource> g_font_texture_resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE g_font_cpu_descriptor = {};
	D3D12_GPU_DESCRIPTOR_HANDLE g_font_gpu_descriptor = {};

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
			#if defined(DEBUG) or defined(_DEBUG)
			__debugbreak();
			#endif
			std::abort();
		}
		else
		{
			return false;
		}
	}

	auto create_device(HWND window) -> bool;
	auto cleanup_device() -> void;

	auto create_render_target() -> void;
	auto cleanup_render_target() -> void;

	auto wait_for_last_submitted_frame() -> void;
	auto wait_for_next_frame_resources() -> d3d_frame_context_type&;

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

	auto WINAPI window_procedure(const_window_type window, const UINT msg, const WPARAM w_param, const LPARAM l_param) -> LRESULT
	{
		switch (msg)
		{
			case WM_SIZE:
			{
				if (g_device != nullptr && w_param != SIZE_MINIMIZED)
				{
					wait_for_last_submitted_frame();
					cleanup_render_target();
					check_hr_error(
						g_swap_chain->ResizeBuffers(
							0,
							(UINT)LOWORD(l_param),
							(UINT)HIWORD(l_param),
							DXGI_FORMAT_UNKNOWN,
							DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
						)
					);
					create_render_target();
				}
				return 0;
			}
			case WM_DESTROY:
			{
				PostQuitMessage(0);
				return 0;
			}
			default:
			{
				return DefWindowProc(window, msg, w_param, l_param);
			}
		}
	}
} // namespace

int main(int, char**)
{
	// Register the window class
	const WNDCLASSEX window_class{
			.cbSize = sizeof(WNDCLASSEXW),
			.style = CS_CLASSDC,
			.lpfnWndProc = window_procedure,
			.cbClsExtra = 0,
			.cbWndExtra = 0,
			.hInstance = GetModuleHandle(nullptr),
			.hIcon = nullptr,
			.hCursor = nullptr,
			.hbrBackground = nullptr,
			.lpszMenuName = nullptr,
			.lpszClassName = "GUI Playground",
			.hIconSm = nullptr
	};
	RegisterClassEx(&window_class);

	const auto window = CreateWindow(
		window_class.lpszClassName,
		"GUI Playground Example(DX12)",
		WS_OVERLAPPEDWINDOW,
		g_window_position_left,
		g_window_position_top,
		g_window_width,
		g_window_height,
		nullptr,
		nullptr,
		window_class.hInstance,
		nullptr
	);

	// Initialize Direct3D
	if (not create_device(window))
	{
		cleanup_device();
		UnregisterClass(window_class.lpszClassName, window_class.hInstance);
		return 1;
	}

	const auto range = gui::glyph_range_simplified_chinese_common();
	g_draw_list_shared_data->set_default_font(gui::load_font(R"(C:\Windows\Fonts\msyh.ttc)", 18, range));

	// Setup Platform/Renderer backends
	win32_init(window);
	d3d_init();
	prometheus_init();

	// Show the window
	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	// Main loop
	{
		bool done = false;
		while (not done)
		{
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
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
				Sleep(10);
				continue;
			}
			g_swap_chain_occluded = false;

			win32_new_frame(window);
			d3d_new_frame();
			prometheus_new_frame();

			// Rendering
			prometheus_render();

			auto& [frame_command_allocator, frame_fence_value] = wait_for_next_frame_resources();
			const auto back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();
			(void)frame_command_allocator->Reset();

			D3D12_RESOURCE_BARRIER barrier{
					.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
					.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
					.Transition =
					{
							.pResource = g_main_render_target_resource[back_buffer_index].Get(),
							.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
							.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
							.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
					}
			};
			(void)g_command_list->Reset(frame_command_allocator.Get(), nullptr);
			g_command_list->ResourceBarrier(1, &barrier);

			constexpr float clear_color_with_alpha[4]{.45f, .55f, .6f, 1.f};
			ID3D12DescriptorHeap* descriptor_heaps[]{g_shader_resource_view_descriptor_heap.Get()};

			g_command_list->ClearRenderTargetView(g_main_render_target_descriptor[back_buffer_index], clear_color_with_alpha, 0, nullptr);
			g_command_list->OMSetRenderTargets(1, &g_main_render_target_descriptor[back_buffer_index], FALSE, nullptr);
			g_command_list->SetDescriptorHeaps(1, descriptor_heaps);

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
	}

	wait_for_last_submitted_frame();

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
	INT64 g_ticks_per_second = 0;
	INT64 g_last_time = 0;
	INT64 g_frame_count = 0;
	float g_fps = 0;

	static_assert(sizeof(gui::DrawList::vertex_type) == sizeof(d3d_vertex_type));
	static_assert(sizeof(gui::DrawList::index_type) == sizeof(d3d_vertex_index_type));

	gui::DrawList g_draw_list;
	ComPtr<ID3D12Resource> g_pic_texture_resource;
	D3D12_GPU_DESCRIPTOR_HANDLE g_pic_texture_handle;

	auto create_device(const_window_type window) -> bool
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

			// todo: cannot run program normally
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
			for (auto& h: g_main_render_target_descriptor)
			{
				h = rtv_handle;
				rtv_handle.ptr += rtv_descriptor_size;
			}
		}

		{
			constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{
					.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
					// note: default font + extra pic
					// @see load_texture_from_file
					.NumDescriptors = 1 + 1,
					.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
					.NodeMask = 0
			};
			check_hr_error(g_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(g_shader_resource_view_descriptor_heap.GetAddressOf())));
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

		ComPtr<IDXGIFactory4> dxgi_factory = nullptr;
		ComPtr<IDXGISwapChain1> swap_chain1 = nullptr;
		check_hr_error(CreateDXGIFactory1(IID_PPV_ARGS(dxgi_factory.GetAddressOf())));
		check_hr_error(dxgi_factory->CreateSwapChainForHwnd(g_command_queue.Get(), window, &swap_chain_desc, nullptr, nullptr, swap_chain1.GetAddressOf()));
		check_hr_error(swap_chain1->QueryInterface(IID_PPV_ARGS(g_swap_chain.GetAddressOf())));
		check_hr_error(g_swap_chain->SetMaximumFrameLatency(num_back_buffers));
		g_swap_chain_waitable_object = g_swap_chain->GetFrameLatencyWaitableObject();

		create_render_target();
		return true;
	}

	auto cleanup_device() -> void
	{
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
		for (UINT i = 0; i < num_back_buffers; i++)
		{
			ComPtr<ID3D12Resource> back_buffer = nullptr;
			check_hr_error(g_swap_chain->GetBuffer(i, IID_PPV_ARGS(back_buffer.GetAddressOf())));
			g_device->CreateRenderTargetView(back_buffer.Get(), nullptr, g_main_render_target_descriptor[i]);
			g_main_render_target_resource[i] = back_buffer;
		}
	}

	auto cleanup_render_target() -> void
	{
		wait_for_last_submitted_frame();

		for (auto& resource: g_main_render_target_resource)
		{
			resource = nullptr;
		}
	}

	auto wait_for_last_submitted_frame() -> void
	{
		d3d_frame_context_type& frame_context = g_frame_context[g_frame_index % num_frames_in_flight];

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

	auto wait_for_next_frame_resources() -> d3d_frame_context_type&
	{
		g_frame_index = g_frame_index + 1;

		HANDLE waitable_objects[] = {g_swap_chain_waitable_object, nullptr};
		DWORD num_waitable_objects = 1;

		d3d_frame_context_type& frame_context = g_frame_context[g_frame_index % num_frames_in_flight];
		if (const auto fence_value = frame_context.fence_value;
			fence_value != 0) // means no fence was signaled
		{
			frame_context.fence_value = 0;
			(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
			waitable_objects[1] = g_fence_event;
			num_waitable_objects = 2;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects, TRUE, INFINITE);

		return frame_context;
	}

	auto win32_init(const_window_type window) -> void
	{
		(void)window;

		QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&g_ticks_per_second));
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&g_last_time));
	}

	auto win32_new_frame(const_window_type window) -> void
	{
		RECT rect;
		GetClientRect(window, &rect);
		g_window_position_left = rect.left;
		g_window_position_top = rect.top;
		g_window_width = rect.right - rect.left;
		g_window_height = rect.bottom - rect.top;

		INT64 current_time;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&current_time));
		const auto elapsed = static_cast<float>(current_time - g_last_time) / static_cast<float>(g_ticks_per_second);
		g_frame_count += 1;

		if (elapsed > .5f)
		{
			g_fps = static_cast<float>(g_frame_count) / elapsed;
			g_frame_count = 0;
			g_last_time = current_time;
		}
	}

	auto win32_shutdown() -> void
	{
		//
	}

	[[nodiscard]] auto create_fonts_texture() -> bool
	{
		const auto& font = g_draw_list_shared_data->get_default_font();
		const auto pixels = font.texture_data.get();
		const auto width = font.texture_size.width;
		const auto height = font.texture_size.height;
		assert(pixels);

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

		const auto upload_pitch = (static_cast<UINT>(width * 4) + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		const auto upload_size = static_cast<UINT>(height) * upload_pitch;

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
		for (UINT i = 0; i < static_cast<UINT>(height); ++i)
		{
			auto* dest = static_cast<std::uint8_t*>(mapped_data) + static_cast<std::ptrdiff_t>(upload_pitch * i);
			auto* source = reinterpret_cast<std::uint8_t*>(pixels) + static_cast<std::ptrdiff_t>(width * i * 4);
			const auto size = width * 4;
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
								.Width = static_cast<UINT>(width),
								.Height = static_cast<UINT>(height),
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

		g_device->CreateShaderResourceView(texture.Get(), &resource_view_desc, g_font_cpu_descriptor);
		g_font_texture_resource = texture;

		g_draw_list_shared_data->get_default_font().texture_id = static_cast<gui::font_type::texture_id_type>(g_font_gpu_descriptor.ptr);

		return true;
	}

	auto d3d_destroy_device_objects() -> void
	{
		// com ptr
		// do nothing here
	}

	auto d3d_create_device_objects() -> bool
	{
		if (g_device == nullptr)
		{
			return false;
		}

		if (g_pipeline_state)
		{
			d3d_destroy_device_objects();
		}

		// Create the root signature
		{
			// d3d_constant_buffer_type
			constexpr D3D12_ROOT_PARAMETER param_0{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
					.Constants = {.ShaderRegister = 0, .RegisterSpace = 0, .Num32BitValues = sizeof(d3d_constant_buffer_type) / sizeof(float)},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX
			};

			// g_font_gpu_descriptor
			constexpr D3D12_DESCRIPTOR_RANGE range{
					.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
					.NumDescriptors = 1,
					.BaseShaderRegister = 0,
					.RegisterSpace = 0,
					.OffsetInDescriptorsFromTableStart = 0
			};
			const D3D12_ROOT_PARAMETER param_1{
					.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
					.DescriptorTable = {.NumDescriptorRanges = 1, .pDescriptorRanges = &range},
					.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			};
			// @see `prometheus_draw` -> `g_command_list->SetGraphicsRootXxx`
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
					.Flags =
					D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
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
					reinterpret_cast<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(GetProcAddress(d3d12_dll, "D3D12SerializeRootSignature")); // NOLINT(clang-diagnostic-cast-function-type-strict)
			if (serialize_root_signature_function == nullptr)
			{
				return false;
			}

			ComPtr<ID3DBlob> blob = nullptr;
			check_hr_error(serialize_root_signature_function(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, blob.GetAddressOf(), nullptr));
			check_hr_error(
				g_device->CreateRootSignature(
					0,
					blob->GetBufferPointer(),
					blob->GetBufferSize(),
					IID_PPV_ARGS(g_root_signature.GetAddressOf())
				)
			);
		}

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
		constexpr D3D12_INPUT_ELEMENT_DESC input_element_desc[]{
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

		D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_state_desc{
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
				.RTVFormats = {g_render_target_view_format},
				.DSVFormat = {},
				.SampleDesc = {.Count = 1, .Quality = 0},
				.NodeMask = 1,
				.CachedPSO = {.pCachedBlob = nullptr, .CachedBlobSizeInBytes = 0},
				.Flags = D3D12_PIPELINE_STATE_FLAG_NONE
		};

		check_hr_error(g_device->CreateGraphicsPipelineState(&pipeline_state_desc, IID_PPV_ARGS(g_pipeline_state.GetAddressOf())));

		return create_fonts_texture();
	}

	auto d3d_init() -> void
	{
		g_font_cpu_descriptor = g_shader_resource_view_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		g_font_gpu_descriptor = g_shader_resource_view_descriptor_heap->GetGPUDescriptorHandleForHeapStart();

		for (auto& [index, index_count, vertex, vertex_count]: g_frame_resource)
		{
			index = nullptr;
			index_count = 0;
			vertex = nullptr;
			vertex_count = 0;
		}
	}

	auto d3d_new_frame() -> void
	{
		if (g_pipeline_state == nullptr) //
		{
			d3d_create_device_objects();
		}
	}

	auto d3d_shutdown() -> void
	{
		d3d_destroy_device_objects();
	}

	[[nodiscard]] auto load_texture_from_file(const std::string_view filename) -> bool
	{
		int image_width;
		int image_height;

		auto* data = stbi_load(filename.data(), &image_width, &image_height, nullptr, 4);
		if (data == nullptr)
		{
			return false;
		}

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
				.Width = static_cast<UINT64>(image_width),
				.Height = static_cast<UINT>(image_height),
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

		const auto upload_pitch = (static_cast<UINT>(image_width * 4) + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
		const auto upload_size = static_cast<UINT>(image_height) * upload_pitch;

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
		for (UINT i = 0; i < static_cast<UINT>(image_height); ++i)
		{
			auto* dest = static_cast<std::uint8_t*>(mapped_data) + static_cast<std::ptrdiff_t>(upload_pitch * i);
			auto* source = reinterpret_cast<std::uint8_t*>(data) + static_cast<std::ptrdiff_t>(image_width * i * 4);
			const auto size = image_width * 4;
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
								.Width = static_cast<UINT>(image_width),
								.Height = static_cast<UINT>(image_height),
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

		D3D12_CPU_DESCRIPTOR_HANDLE extra_pic_handle = g_shader_resource_view_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
		extra_pic_handle.ptr += increment_size;

		g_device->CreateShaderResourceView(texture.Get(), &resource_view_desc, extra_pic_handle);

		g_pic_texture_resource = texture;
		g_pic_texture_handle = g_shader_resource_view_descriptor_heap->GetGPUDescriptorHandleForHeapStart();
		g_pic_texture_handle.ptr += increment_size;

		stbi_image_free(data);

		return true;
	}

	auto prometheus_init() -> void
	{
		using functional::operators::operator|;
		g_draw_list.draw_list_flag(gui::DrawListFlag::ANTI_ALIASED_LINE | gui::DrawListFlag::ANTI_ALIASED_FILL);
		g_draw_list.shared_data(g_draw_list_shared_data);

		// load extra texture
		assert(load_texture_from_file(ASSETS_PATH_PIC));
	}

	auto prometheus_new_frame() -> void //
	{
		g_draw_list.reset();
		g_draw_list.push_clip_rect(
			{static_cast<float>(g_window_position_left), static_cast<float>(g_window_position_top)},
			{static_cast<float>(g_window_position_left + g_window_width), static_cast<float>(g_window_position_top + g_window_height)},
			false
		);
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
		g_draw_list.image_rounded(static_cast<gui::DrawList::texture_id_type>(g_pic_texture_handle.ptr), {900, 350, 1200, 650}, 10);
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
					.Width = this_frame_index_count * sizeof(d3d_vertex_index_type),
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
			constexpr D3D12_RANGE range{.Begin = 0, .End = 0};
			check_hr_error(this_frame_vertex_buffer->Map(0, &range, &mapped_vertex_resource));
			check_hr_error(this_frame_index_buffer->Map(0, &range, &mapped_index_resource));

			auto* mapped_vertex = static_cast<d3d_vertex_type*>(mapped_vertex_resource);
			auto* mapped_index = static_cast<d3d_vertex_index_type*>(mapped_index_resource);

			std::ranges::transform(
				vertex_list,
				mapped_vertex,
				[](const gui::DrawList::vertex_type& vertex) -> d3d_vertex_type
				{
					return {
							.position = {vertex.position.x, vertex.position.y},
							.uv = {vertex.uv.x, vertex.uv.y},
							.color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>)
					};
				}
			);
			std::ranges::copy(index_list, mapped_index);

			this_frame_vertex_buffer->Unmap(0, &range);
			this_frame_index_buffer->Unmap(0, &range);
		}

		// Setup orthographic projection matrix into our constant buffer
		d3d_constant_buffer_type vertex_constant_buffer;
		{
			const auto left = static_cast<float>(g_window_position_left);
			const auto right = static_cast<float>(g_window_position_left + g_window_width);
			const auto top = static_cast<float>(g_window_position_top);
			const auto bottom = static_cast<float>(g_window_position_top + g_window_height);

			const float mvp[4][4]{
					{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
					{0.0f, 2.0f / (top - bottom), 0.0f, 0.0f},
					{0.0f, 0.0f, 0.5f, 0.0f},
					{(right + left) / (left - right), (top + bottom) / (bottom - top), 0.5f, 1.0f},
			};
			std::memcpy(vertex_constant_buffer, mvp, sizeof(mvp));
		}

		// Setup viewport
		{
			const auto left = static_cast<float>(g_window_position_left);
			const auto top = static_cast<float>(g_window_position_top);

			const D3D12_VIEWPORT viewport{
					.TopLeftX = left,
					.TopLeftY = top,
					.Width = static_cast<FLOAT>(g_window_width),
					.Height = static_cast<FLOAT>(g_window_height),
					.MinDepth = .0f,
					.MaxDepth = 1.f
			};
			g_command_list->RSSetViewports(1, &viewport);
		}

		// Bind shader and vertex buffers
		{
			const D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view{
					.BufferLocation = this_frame.vertex->GetGPUVirtualAddress(),
					.SizeInBytes = this_frame.vertex_count * static_cast<UINT>(sizeof(d3d_vertex_type)),
					.StrideInBytes = sizeof(d3d_vertex_type)
			};
			g_command_list->IASetVertexBuffers(0, 1, &vertex_buffer_view);

			const D3D12_INDEX_BUFFER_VIEW index_buffer_view{
					.BufferLocation = this_frame.index->GetGPUVirtualAddress(),
					.SizeInBytes = this_frame.index_count * static_cast<UINT>(sizeof(d3d_vertex_index_type)),
					// ReSharper disable once CppUnreachableCode
					.Format = sizeof(d3d_vertex_index_type) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
			};
			g_command_list->IASetIndexBuffer(&index_buffer_view);
			g_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			g_command_list->SetPipelineState(g_pipeline_state.Get());
			g_command_list->SetGraphicsRootSignature(g_root_signature.Get());
			g_command_list->SetGraphicsRoot32BitConstants(0, 16, &vertex_constant_buffer, 0);
		}

		// Setup blend factor
		constexpr float blend_factor[4]{.0f, .0f, .0f, .0f};
		g_command_list->OMSetBlendFactor(blend_factor);

		// The following code is no longer needed, because we bind the handle based on the command.
		// Bind font texture
		// {
		// 	g_command_list->SetGraphicsRootDescriptorTable(1, g_font_gpu_descriptor);
		// }

		// const D3D12_RECT rect{g_window_position_left, g_window_position_top, g_window_position_left + g_window_width, g_window_position_top + g_window_height};
		// g_command_list->RSSetScissorRects(1, &rect);

		// g_command_list->DrawIndexedInstanced(static_cast<UINT>(index_list.size()), 1, 0, 0, 0);
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

	auto prometheus_shutdown() -> void //
	{
		//
	}
}
