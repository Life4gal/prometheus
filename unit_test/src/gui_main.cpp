import std;

#include <d3d12.h>
#include <dxgi1_4.h>
#include <cassert>

#pragma comment(lib, "d3d12.lib")
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
	// using namespace gal::prometheus;

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
	ID3D12DescriptorHeap* g_pd3d_srv_desc = nullptr;
	ID3D12CommandQueue* g_d3d_command_queue = nullptr;
	ID3D12GraphicsCommandList* g_d3d_command_list = nullptr;
	ID3D12Fence* g_fence = nullptr;
	HANDLE g_fence_event = nullptr;
	UINT64 g_fence_last_signaled_value = 0;
	IDXGISwapChain3* g_swap_chain = nullptr;
	bool g_swap_chain_occluded = false;
	HANDLE g_h_swap_chain_waitable_object = nullptr;
	ID3D12Resource* g_main_render_target_resource[num_back_buffers] = {};
	D3D12_CPU_DESCRIPTOR_HANDLE g_main_render_target_descriptor[num_back_buffers] = {};

	bool create_d3d_device(HWND window);
	void cleanup_d3d_device();
	void create_render_target();
	void cleanup_render_target();
	void wait_for_last_submitted_frame();
	d3d_frame_context* wait_for_next_frame_resources();
	void draw_vertices_data();

	LRESULT WINAPI my_window_procedure(HWND window, UINT msg, WPARAM w_param, LPARAM l_param);

	bool create_d3d_device(HWND window)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC1 sd;
		{
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = num_back_buffers;
			sd.Width = 0;
			sd.Height = 0;
			sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			sd.Scaling = DXGI_SCALING_STRETCH;
			sd.Stereo = FALSE;
		}

		// [DEBUG] Enable debug interface
		#ifdef DX12_ENABLE_DEBUG_LAYER
		ID3D12Debug* dx12_debug = nullptr;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dx12_debug)))) { dx12_debug->EnableDebugLayer(); }
		#endif

		// Create device
		if (constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
			D3D12CreateDevice(nullptr, feature_level, IID_PPV_ARGS(&g_d3d_device)) != S_OK) { return false; }

		// [DEBUG] Setup debug interface to break on any warnings/errors
		#ifdef DX12_ENABLE_DEBUG_LAYER
		if (dx12_debug != nullptr)
		{
			ID3D12InfoQueue* info_queue = nullptr;
			(void)g_d3d_device->QueryInterface(IID_PPV_ARGS(&info_queue));
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			(void)info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
			info_queue->Release();
			dx12_debug->Release();
		}
		#endif

		{
			if (constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			                                              .NumDescriptors = num_back_buffers,
			                                              .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			                                              .NodeMask = 1};
				g_d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_d3d_rtv_desc_heap)) != S_OK) { return false; }

			const auto rtv_descriptor_size = g_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_d3d_rtv_desc_heap->GetCPUDescriptorHandleForHeapStart();
			for (auto& h: g_main_render_target_descriptor)
			{
				h = rtv_handle;
				rtv_handle.ptr += rtv_descriptor_size;
			}
		}

		if (constexpr D3D12_DESCRIPTOR_HEAP_DESC desc{.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		                                              .NumDescriptors = 1,
		                                              .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		                                              .NodeMask = 0};
			g_d3d_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3d_srv_desc)) != S_OK) { return false; }


		if (constexpr D3D12_COMMAND_QUEUE_DESC desc{.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
		                                            .Priority = 0,
		                                            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		                                            .NodeMask = 1};
			g_d3d_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_d3d_command_queue)) != S_OK) { return false; }


		for (auto& [command_allocator, _]: g_frame_context)
			if (g_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)) !=
			    S_OK) { return false; }

		if (g_d3d_device->CreateCommandList(0,
		                                    D3D12_COMMAND_LIST_TYPE_DIRECT,
		                                    g_frame_context[0].command_allocator,
		                                    nullptr,
		                                    IID_PPV_ARGS(&g_d3d_command_list)) != S_OK ||
		    g_d3d_command_list->Close() != S_OK) { return false; }

		if (g_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK) return false;

		g_fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (g_fence_event == nullptr) return false;

		{
			IDXGIFactory4* dxgi_factory = nullptr;
			IDXGISwapChain1* swap_chain1 = nullptr;
			if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)) != S_OK) return false;
			if (dxgi_factory->CreateSwapChainForHwnd(g_d3d_command_queue, window, &sd, nullptr, nullptr, &swap_chain1) != S_OK) return false;
			if (swap_chain1->QueryInterface(IID_PPV_ARGS(&g_swap_chain)) != S_OK) return false;
			swap_chain1->Release();
			dxgi_factory->Release();
			(void)g_swap_chain->SetMaximumFrameLatency(num_back_buffers);
			g_h_swap_chain_waitable_object = g_swap_chain->GetFrameLatencyWaitableObject();
		}

		create_render_target();
		return true;
	}

	void cleanup_d3d_device()
	{
		cleanup_render_target();
		if (g_swap_chain)
		{
			(void)g_swap_chain->SetFullscreenState(false, nullptr);
			g_swap_chain->Release();
			g_swap_chain = nullptr;
		}
		if (g_h_swap_chain_waitable_object != nullptr) { CloseHandle(g_h_swap_chain_waitable_object); }
		for (auto& [command_allocator, _]: g_frame_context)
			if (command_allocator)
			{
				command_allocator->Release();
				command_allocator = nullptr;
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
		if (g_pd3d_srv_desc)
		{
			g_pd3d_srv_desc->Release();
			g_pd3d_srv_desc = nullptr;
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

	void create_render_target()
	{
		for (UINT i = 0; i < num_back_buffers; i++)
		{
			ID3D12Resource* back_buffer = nullptr;
			(void)g_swap_chain->GetBuffer(i, IID_PPV_ARGS(&back_buffer));
			g_d3d_device->CreateRenderTargetView(back_buffer, nullptr, g_main_render_target_descriptor[i]);
			g_main_render_target_resource[i] = back_buffer;
		}
	}

	void cleanup_render_target()
	{
		wait_for_last_submitted_frame();

		for (auto& resource: g_main_render_target_resource)
			if (resource)
			{
				resource->Release();
				resource = nullptr;
			}
	}

	void wait_for_last_submitted_frame()
	{
		d3d_frame_context* frame_context = &g_frame_context[g_frame_index % num_frames_in_flight];

		const UINT64 fence_value = frame_context->fence_value;
		if (fence_value == 0) return; // No fence was signaled

		frame_context->fence_value = 0;
		if (g_fence->GetCompletedValue() >= fence_value) return;

		(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
		WaitForSingleObject(g_fence_event, INFINITE);
	}

	d3d_frame_context* wait_for_next_frame_resources()
	{
		const UINT next_frame_index = g_frame_index + 1;
		g_frame_index = next_frame_index;

		HANDLE waitable_objects[] = {g_h_swap_chain_waitable_object, nullptr};
		DWORD num_waitable_objects = 1;

		d3d_frame_context* frame_context = &g_frame_context[next_frame_index % num_frames_in_flight];
		if (const UINT64 fence_value = frame_context->fence_value;
			fence_value != 0) // means no fence was signaled
		{
			frame_context->fence_value = 0;
			(void)g_fence->SetEventOnCompletion(fence_value, g_fence_event);
			waitable_objects[1] = g_fence_event;
			num_waitable_objects = 2;
		}

		WaitForMultipleObjects(num_waitable_objects, waitable_objects, TRUE, INFINITE);

		return frame_context;
	}

	void draw_vertices_data()
	{
		// todo
	}

	LRESULT WINAPI my_window_procedure(HWND window, const UINT msg, const WPARAM w_param, const LPARAM l_param)
	{
		switch (msg)
		{
			case WM_SIZE:
			{
				if (g_d3d_device != nullptr && w_param != SIZE_MINIMIZED)
				{
					wait_for_last_submitted_frame();
					cleanup_render_target();
					const HRESULT result = g_swap_chain->ResizeBuffers(0,
					                                                   (UINT)LOWORD(l_param),
					                                                   (UINT)HIWORD(l_param),
					                                                   DXGI_FORMAT_UNKNOWN,
					                                                   DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
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
		}
		return ::DefWindowProcW(window, msg, w_param, l_param);
	}
}

int main(int, char**)
{
	const WNDCLASSEXW wc =
	{sizeof(wc), CS_CLASSDC, my_window_procedure, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"GUI Playground",
	 nullptr};
	::RegisterClassExW(&wc);
	HWND window = ::CreateWindowW(wc.lpszClassName,
	                              L"GUI Playground Example",
	                              WS_OVERLAPPEDWINDOW,
	                              100,
	                              100,
	                              1280,
	                              800,
	                              nullptr,
	                              nullptr,
	                              wc.hInstance,
	                              nullptr);

	// Initialize Direct3D
	if (!create_d3d_device(window))
	{
		cleanup_d3d_device();
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(window, SW_SHOWDEFAULT);
	::UpdateWindow(window);

	// Main loop
	bool done = false;
	while (!done)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// See the my_window_procedure() function below for our to dispatch events to the Win32 backend.
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT) done = true;
		}
		if (done) break;

		// Handle window screen locked
		if (g_swap_chain_occluded && g_swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(10);
			continue;
		}
		g_swap_chain_occluded = false;

		d3d_frame_context* frame_context = wait_for_next_frame_resources();
		const UINT back_buffer_index = g_swap_chain->GetCurrentBackBufferIndex();
		(void)frame_context->command_allocator->Reset();

		D3D12_RESOURCE_BARRIER barrier = {};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = g_main_render_target_resource[back_buffer_index];
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		(void)g_d3d_command_list->Reset(frame_context->command_allocator, nullptr);
		g_d3d_command_list->ResourceBarrier(1, &barrier);

		constexpr float clear_color_with_alpha[4] = {
				.45f,
				.55f,
				.6f,
				1.f
		};
		g_d3d_command_list->ClearRenderTargetView(g_main_render_target_descriptor[back_buffer_index], clear_color_with_alpha, 0, nullptr);
		g_d3d_command_list->OMSetRenderTargets(1, &g_main_render_target_descriptor[back_buffer_index], FALSE, nullptr);
		g_d3d_command_list->SetDescriptorHeaps(1, &g_pd3d_srv_desc);
		draw_vertices_data();
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

	cleanup_d3d_device();
	::DestroyWindow(window);
	::UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
