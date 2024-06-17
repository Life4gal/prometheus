#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <print>
#include <vector>
#include <cassert>
#include <source_location>

#include "font.hpp"

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace
{
	using namespace gal::prometheus;

	using point_type = primitive::basic_point<float, 2>;
	using rect_type = primitive::basic_rect<float, 2>;
	using vertex_type = primitive::basic_vertex<point_type>;
	using vertex_index_type = std::uint16_t;

	using vertex_list_type = primitive::basic_vertex_list<vertex_type>;
	// todo
	using vertex_index_list_type = std::vector<vertex_index_type>;

	struct draw_list_type
	{
		vertex_list_type vertex_list;
		vertex_index_list_type index_list;
	};

	struct draw_data_type
	{
		rect_type display_rect;

		std::vector<draw_list_type> draw_lists;

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

	draw_data_type g_draw_data;

	namespace detail
	{
		struct frame_type
		{
			VkCommandPool command_pool;
			VkCommandBuffer command_buffer;
			VkFence fence;
			VkImage back_buffer;
			VkImageView back_buffer_view;
			VkFramebuffer frame_buffer;
		};

		struct frame_semaphore_type
		{
			VkSemaphore image_acquired_semaphore;
			VkSemaphore render_complete_semaphore;
		};

		auto destroy_frame(const VkDevice device, frame_type& frame, const VkAllocationCallbacks* allocation_callbacks) -> void
		{
			vkDestroyFence(device, frame.fence, allocation_callbacks);
			vkFreeCommandBuffers(device, frame.command_pool, 1, &frame.command_buffer);
			vkDestroyCommandPool(device, frame.command_pool, allocation_callbacks);
			frame.fence = VK_NULL_HANDLE;
			frame.command_buffer = VK_NULL_HANDLE;
			frame.command_pool = VK_NULL_HANDLE;

			// vkDestroyImage(device, frame.back_buffer, &allocation_callbacks);
			vkDestroyImageView(device, frame.back_buffer_view, allocation_callbacks);
			vkDestroyFramebuffer(device, frame.frame_buffer, allocation_callbacks);
			frame.back_buffer = VK_NULL_HANDLE;
			frame.back_buffer_view = VK_NULL_HANDLE;
			frame.frame_buffer = VK_NULL_HANDLE;
		}

		auto destroy_frame_semaphore(
			const VkDevice device,
			frame_semaphore_type& frame_semaphore,
			const VkAllocationCallbacks* allocation_callbacks
		) -> void
		{
			vkDestroySemaphore(device, frame_semaphore.image_acquired_semaphore, allocation_callbacks);
			vkDestroySemaphore(device, frame_semaphore.render_complete_semaphore, allocation_callbacks);
			frame_semaphore.image_acquired_semaphore = VK_NULL_HANDLE;
			frame_semaphore.render_complete_semaphore = VK_NULL_HANDLE;
		}

		struct frame_render_buffer_type
		{
			VkDeviceMemory vertex_buffer_memory{VK_NULL_HANDLE};
			VkDeviceSize vertex_count{0};
			VkBuffer vertex_buffer{VK_NULL_HANDLE};

			VkDeviceMemory index_buffer_memory{VK_NULL_HANDLE};
			VkDeviceSize index_count{0};
			VkBuffer index_buffer{VK_NULL_HANDLE};
		};
	}

	GLFWwindow* g_window;
	int g_window_width = 1280;
	int g_window_height = 720;
	int g_window_frame_buffer_width;
	int g_window_frame_buffer_height;

	VkAllocationCallbacks* g_allocation_callbacks;

	VkSurfaceKHR g_window_surface;
	VkSurfaceFormatKHR g_window_surface_format;
	VkPresentModeKHR g_window_present_mode;
	// assert(g_window_min_image_count >= 2)
	std::uint32_t g_window_min_image_count{2};
	VkSwapchainKHR g_window_swap_chain;
	bool g_window_swap_chain_rebuild_required;

	bool g_window_clear_enable{true};
	VkClearValue g_window_clear_value{.45f, .55f, .65f, 1.f};

	std::unique_ptr<detail::frame_type[]> g_window_frames;
	std::uint32_t g_window_frame_current_index;
	std::uint32_t g_window_frame_total_count;
	constexpr auto g_window_frames_maker = []
	{
		g_window_frames = std::make_unique<detail::frame_type[]>(g_window_frame_total_count);
	};
	constexpr auto g_window_frames_destroyer = [](const VkDevice device, const VkAllocationCallbacks* allocation_callbacks)
	{
		std::ranges::for_each(
			std::ranges::subrange{g_window_frames.get(), g_window_frames.get() + g_window_frame_total_count},
			[&](detail::frame_type& frame) { destroy_frame(device, frame, allocation_callbacks); }
		);

		g_window_frames.reset();
		g_window_frame_current_index = 0;
		g_window_frame_total_count = 0;
	};

	std::unique_ptr<detail::frame_semaphore_type[]> g_window_frame_semaphores;
	std::uint32_t g_window_frame_semaphore_current_index;
	std::uint32_t g_window_frame_semaphore_total_count;
	constexpr auto g_window_frame_semaphores_maker = []
	{
		g_window_frame_semaphores = std::make_unique<detail::frame_semaphore_type[]>(g_window_frame_semaphore_total_count);
	};
	constexpr auto g_window_frame_semaphores_destroyer = [](const VkDevice device, const VkAllocationCallbacks* allocation_callbacks)
	{
		std::ranges::for_each(
			std::ranges::subrange{g_window_frame_semaphores.get(), g_window_frame_semaphores.get() + g_window_frame_semaphore_total_count},
			[&](detail::frame_semaphore_type& frame_semaphore) { destroy_frame_semaphore(device, frame_semaphore, allocation_callbacks); }
		);

		g_window_frame_semaphores.reset();
		g_window_frame_semaphore_current_index = 0;
		g_window_frame_semaphore_total_count = 0;
	};

	std::unique_ptr<detail::frame_render_buffer_type[]> g_window_render_buffer;
	std::uint32_t g_window_render_buffer_current_index;
	std::uint32_t g_window_render_buffer_total_count;
	constexpr auto g_window_render_buffer_maker = []
	{
		g_window_render_buffer = std::make_unique<detail::frame_render_buffer_type[]>(g_window_render_buffer_total_count);
	};
	constexpr auto g_window_render_buffer_destroyer = [](const VkDevice device, const VkAllocationCallbacks* allocation_callbacks)
	{
		std::ranges::for_each(
			std::ranges::subrange{g_window_render_buffer.get(), g_window_render_buffer.get() + g_window_render_buffer_total_count},
			[&](const detail::frame_render_buffer_type& frame) -> void
			{
				if (frame.vertex_buffer)
				{
					vkDestroyBuffer(device, frame.vertex_buffer, allocation_callbacks);
				}
				if (frame.vertex_buffer_memory)
				{
					vkFreeMemory(device, frame.vertex_buffer_memory, allocation_callbacks);
				}

				if (frame.index_buffer)
				{
					vkDestroyBuffer(device, frame.index_buffer, allocation_callbacks);
				}
				if (frame.index_buffer_memory)
				{
					vkFreeMemory(device, frame.index_buffer_memory, allocation_callbacks);
				}
			}
		);
		g_window_render_buffer.reset();
		g_window_render_buffer_current_index = 0;
		g_window_render_buffer_total_count = 0;
	};

	VkInstance g_instance;
	VkDebugReportCallbackEXT g_debug_report_callback;

	VkPhysicalDevice g_physical_device;
	VkDevice g_device;

	std::uint32_t g_queue_family;
	VkQueue g_queue;

	VkDescriptorPool g_descriptor_pool;

	VkSampler g_font_sampler;
	VkDeviceMemory g_font_memory;
	VkImage g_font_image;
	VkImageView g_font_image_view;
	VkDescriptorSet g_font_descriptor_set;
	VkCommandPool g_font_command_pool;
	VkCommandBuffer g_font_command_buffer;

	VkDescriptorSetLayout g_pipeline_descriptor_set_layout;
	VkPipelineLayout g_pipeline_layout;
	VkShaderModule g_pipeline_shader_module_vertex;
	VkShaderModule g_pipeline_shader_module_fragment;
	VkSampleCountFlagBits g_pipeline_rasterization_msaa{VK_SAMPLE_COUNT_1_BIT};
	VkPipelineCreateFlags g_pipeline_create_flags;
	VkRenderPass g_pipeline_render_pass;
	VkPipeline g_pipeline;
	// optional
	VkPipelineCache g_pipeline_cache;
	std::uint32_t g_pipeline_sub_pass;
	bool g_pipeline_use_dynamic_rendering;
	VkPipelineRenderingCreateInfoKHR g_pipeline_rendering_create_info;

	auto glfw_error_callback(const int error, const char* message) -> void //
	{
		std::println(std::cerr, "GLFW Error {}: {}", error, message);
	}

	auto vulkan_check_error(const VkResult result, const std::source_location& location = std::source_location::current()) -> void
	{
		if (result == VK_SUCCESS)
		{
			return;
		}

		std::println(std::cerr, "VULKAN Error: {} -- at {}:{}\n", meta::name_of(result), location.file_name(), location.line());
		// todo
		std::abort();
	}

	auto VKAPI_CALL vulkan_debug_report(
		const VkDebugReportFlagsEXT flags,
		const VkDebugReportObjectTypeEXT object_type,
		const std::uint64_t object,
		const std::size_t location,
		const std::int32_t message_code,
		const char* layer_prefix,
		const char* message,
		[[maybe_unused]] void* user_data
	) -> VkBool32
	{
		std::println(
			std::cerr,
			"Vulkan debug report: \n\t flags({}) \n\t object_type({}) \n\t object(0x{:x}) \n\t location({}) \n\t message_code({}) \n\t "
			"layer_prefix({}) \n\t message({})\n",
			flags,
			meta::name_of(object_type),
			object,
			location,
			message_code,
			layer_prefix,
			message
		);
		return VK_FALSE;
	}

	auto vulkan_setup(std::vector<const char*>& extensions) -> void;

	auto vulkan_create_or_resize_window() -> void;

	auto vulkan_setup_window() -> bool;

	auto init() -> void;

	auto new_frame() -> void;

	auto frame_render() -> void;

	auto frame_present() -> void;

	auto shutdown() -> void;

	auto vulkan_cleanup_window() -> void;

	auto vulkan_cleanup() -> void;
}

int main(int, char**)
{
	glfwSetErrorCallback(glfw_error_callback);
	if (not glfwInit())
	{
		std::println(std::cerr, "GLFW: glfwInit failed");
		return -1;
	}

	// Create window with Vulkan context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	g_window = glfwCreateWindow(g_window_width, g_window_height, "Vulkan+GLFW GUI Playground", nullptr, nullptr);
	if (not glfwVulkanSupported())
	{
		std::println(std::cerr, "GLFW: Vulkan Not Supported");
		return -1;
	}

	std::uint32_t extensions_count;
	const char** extensions_raw = glfwGetRequiredInstanceExtensions(&extensions_count);
	std::vector<const char*> extensions{extensions_raw, extensions_raw + extensions_count};
	vulkan_setup(extensions);

	// Create Window Surface
	vulkan_check_error(glfwCreateWindowSurface(g_instance, g_window, g_allocation_callbacks, &g_window_surface));

	// Create frame buffer
	glfwGetFramebufferSize(g_window, &g_window_frame_buffer_width, &g_window_frame_buffer_height);
	if (not vulkan_setup_window())
	{
		std::println(std::cerr, "Vulkan: vulkan_setup_window failed");
		return -1;
	}

	// Setup Platform/Renderer backends
	init();

	// user data here
	{
		g_draw_data.display_rect = {0, 0, static_cast<float>(g_window_width), static_cast<float>(g_window_height)};
		auto& draw_list = g_draw_data.draw_lists.emplace_back();
		draw_list.vertex_list.triangle({100, 100}, {150, 150}, {200, 200}, primitive::colors::gold);
		draw_list.index_list.push_back(0);
		draw_list.index_list.push_back(1);
		draw_list.index_list.push_back(2);
	}

	// Main loop
	while (not glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		if (g_window_swap_chain_rebuild_required)
		{
			glfwGetFramebufferSize(g_window, &g_window_frame_buffer_width, &g_window_frame_buffer_height);
			if (g_window_frame_buffer_width > 0 and g_window_frame_buffer_height > 0)
			{
				vulkan_create_or_resize_window();
				g_window_frame_current_index = 0;
				g_window_swap_chain_rebuild_required = false;
			}
		}

		new_frame();

		if (g_window_width > 0 and g_window_height > 0)
		{
			frame_render();
			frame_present();
		}
	}

	// Cleanup
	vulkan_check_error(vkDeviceWaitIdle(g_device));
	shutdown();

	vulkan_cleanup_window();
	vulkan_cleanup();

	glfwDestroyWindow(g_window);
	glfwTerminate();

	return 0;
}

namespace
{
	[[nodiscard]] auto memory_type(const VkMemoryPropertyFlags property_flag, const std::uint32_t type_bits) -> std::uint32_t
	{
		VkPhysicalDeviceMemoryProperties memory_properties;
		vkGetPhysicalDeviceMemoryProperties(g_physical_device, &memory_properties);

		for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
		{
			if ((memory_properties.memoryTypes[i].propertyFlags & property_flag) == property_flag and type_bits & (1 << i))
			{
				return i;
			}
		}
		return std::numeric_limits<std::uint32_t>::max();
	}

	auto vulkan_setup(std::vector<const char*>& extensions) -> void
	{
		// Create Vulkan Instance
		{
			VkInstanceCreateInfo instance_create_info{
					.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.pApplicationInfo = nullptr,
					.enabledLayerCount = 0,
					.ppEnabledLayerNames = nullptr,
					.enabledExtensionCount = 0,
					.ppEnabledExtensionNames = nullptr,
			};

			// Enumerate available extensions
			std::uint32_t properties_count;
			vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, nullptr);
			std::vector<VkExtensionProperties> properties;
			properties.resize(properties_count);
			vulkan_check_error(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data()));

			// Enable required extensions
			if (std::ranges::contains(
				properties,
				std::string_view{VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
				&VkExtensionProperties::extensionName
			))
			{
				extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
			}
			if (std::ranges::contains(
				properties,
				std::string_view{VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME},
				&VkExtensionProperties::extensionName
			))
			{
				extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
				instance_create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
			}

			// Enabling validation layers
			constexpr const char* layer_validation[]{"VK_LAYER_KHRONOS_validation"};
			instance_create_info.enabledLayerCount = 1;
			instance_create_info.ppEnabledLayerNames = layer_validation;
			extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

			// Create Vulkan Instance
			instance_create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
			instance_create_info.ppEnabledExtensionNames = extensions.data();
			vulkan_check_error(vkCreateInstance(&instance_create_info, g_allocation_callbacks, &g_instance));

			// setup the debug report callback
			const auto create_debug_report_callback =
					reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(g_instance, "vkCreateDebugReportCallbackEXT"));
			assert(create_debug_report_callback);
			VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info{
					.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
					.pNext = nullptr,
					.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
					.pfnCallback = vulkan_debug_report,
					.pUserData = nullptr
			};
			vulkan_check_error(
				create_debug_report_callback(g_instance, &debug_report_callback_create_info, g_allocation_callbacks, &g_debug_report_callback)
			);
		}

		// Select Physical Device (GPU)
		{
			std::uint32_t gpu_count;
			vulkan_check_error(vkEnumeratePhysicalDevices(g_instance, &gpu_count, nullptr));
			assert(gpu_count != 0);
			std::vector<VkPhysicalDevice> gpus;
			gpus.resize(gpu_count);
			vulkan_check_error(vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpus.data()));

			for (const VkPhysicalDevice& device: gpus)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(device, &properties);

				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					g_physical_device = device;
					return;
				}
			}

			// Use first GPU (Integrated) is a Discrete one is not available.
			g_physical_device = gpus.front();
		}

		// Select graphics queue family
		{
			std::uint32_t queue_family_properties_count;
			vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_properties_count, nullptr);
			std::vector<VkQueueFamilyProperties> queue_family_properties;
			queue_family_properties.resize(queue_family_properties_count);
			vkGetPhysicalDeviceQueueFamilyProperties(g_physical_device, &queue_family_properties_count, queue_family_properties.data());

			for (std::uint32_t i = 0; i < queue_family_properties_count; ++i)
			{
				if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					g_queue_family = i;
					break;
				}
			}
		}

		// Create Logical Device (with 1 queue)
		{
			std::vector device_extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

			// Enumerate physical device extension
			std::uint32_t extension_properties_count;
			vkEnumerateDeviceExtensionProperties(g_physical_device, nullptr, &extension_properties_count, nullptr);
			std::vector<VkExtensionProperties> extension_properties;
			extension_properties.resize(extension_properties_count);
			vkEnumerateDeviceExtensionProperties(g_physical_device, nullptr, &extension_properties_count, extension_properties.data());
			#if defined(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)
			if (std::ranges::contains(extension_properties, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME, &VkExtensionProperties::extensionName))
			{
				extension_properties.push_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
			}
			#endif

			constexpr float queue_priority[]{
					1.f,
			};
			const VkDeviceQueueCreateInfo device_queue_create_info{
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueFamilyIndex = g_queue_family,
					.queueCount = 1,
					.pQueuePriorities = queue_priority
			};
			const VkDeviceCreateInfo device_create_info{
					.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.queueCreateInfoCount = 1,
					.pQueueCreateInfos = &device_queue_create_info,
					.enabledLayerCount = 0,
					.ppEnabledLayerNames = nullptr,
					.enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size()),
					.ppEnabledExtensionNames = device_extensions.data(),
					.pEnabledFeatures = nullptr
			};
			vulkan_check_error(vkCreateDevice(g_physical_device, &device_create_info, g_allocation_callbacks, &g_device));
			vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);
		}

		// Create Descriptor Pool
		{
			// a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)

			constexpr VkDescriptorPoolSize descriptor_pool_size[]{
					{.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = 1},
			};
			const VkDescriptorPoolCreateInfo descriptor_pool_create_info{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					.pNext = nullptr,
					.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
					.maxSets = 1,
					.poolSizeCount = std::ranges::size(descriptor_pool_size),
					.pPoolSizes = descriptor_pool_size
			};

			vulkan_check_error(vkCreateDescriptorPool(g_device, &descriptor_pool_create_info, g_allocation_callbacks, &g_descriptor_pool));
		}
	}

	auto vulkan_create_or_resize_window() -> void
	{
		// swap-chain
		{
			auto old_swap_chain = std::exchange(g_window_swap_chain, VK_NULL_HANDLE);
			vulkan_check_error(vkDeviceWaitIdle(g_device));

			g_window_frames_destroyer(g_device, g_allocation_callbacks);
			g_window_frame_semaphores_destroyer(g_device, g_allocation_callbacks);

			if (g_pipeline_render_pass != VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(g_device, g_pipeline_render_pass, g_allocation_callbacks);
				g_pipeline_render_pass = VK_NULL_HANDLE;
			}
			if (g_pipeline != VK_NULL_HANDLE)
			{
				vkDestroyPipeline(g_device, g_pipeline, g_allocation_callbacks);
				g_pipeline = VK_NULL_HANDLE;
			}

			// Create SwapChain
			{
				VkSurfaceCapabilitiesKHR surface_capabilities;
				vulkan_check_error(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_physical_device, g_window_surface, &surface_capabilities));

				// If min image count was not specified, request different count of images dependent on selected present mode
				auto image_count = g_window_min_image_count;
				if (image_count < surface_capabilities.minImageCount)
				{
					image_count = surface_capabilities.minImageCount;
				}
				if (surface_capabilities.maxImageCount != 0 and image_count > surface_capabilities.maxImageCount)
				{
					image_count = surface_capabilities.maxImageCount;
				}

				if (surface_capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max())
				{
					g_window_width = static_cast<int>(surface_capabilities.currentExtent.width);
					g_window_height = static_cast<int>(surface_capabilities.currentExtent.height);
				}

				VkSwapchainCreateInfoKHR swap_chain_create_info{
						.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
						.pNext = nullptr,
						.flags = 0,
						.surface = g_window_surface,
						.minImageCount = image_count,
						.imageFormat = g_window_surface_format.format,
						.imageColorSpace = g_window_surface_format.colorSpace,
						.imageExtent = {.width = static_cast<std::uint32_t>(g_window_width), .height = static_cast<std::uint32_t>(g_window_height)},
						.imageArrayLayers = 1,
						.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
						.queueFamilyIndexCount = 0,
						.pQueueFamilyIndices = nullptr,
						.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
						.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
						.presentMode = g_window_present_mode,
						.clipped = VK_TRUE,
						.oldSwapchain = old_swap_chain
				};
				vulkan_check_error(vkCreateSwapchainKHR(g_device, &swap_chain_create_info, g_allocation_callbacks, &g_window_swap_chain));

				vulkan_check_error(vkGetSwapchainImagesKHR(g_device, g_window_swap_chain, &g_window_frame_total_count, nullptr));
				g_window_frame_semaphore_total_count = g_window_frame_total_count + 1;

				g_window_frames_maker();
				g_window_frame_semaphores_maker();

				VkImage back_buffer[16]{};
				vulkan_check_error(vkGetSwapchainImagesKHR(g_device, g_window_swap_chain, &g_window_frame_total_count, back_buffer));
				for (std::uint32_t i = 0; i < g_window_frame_total_count; ++i)
				{
					g_window_frames[i].back_buffer = back_buffer[i];
				}
			}

			if (old_swap_chain)
			{
				vkDestroySwapchainKHR(g_device, old_swap_chain, g_allocation_callbacks);
			}

			// Create Render Pass
			if (not g_pipeline_use_dynamic_rendering)
			{
				const VkAttachmentDescription attachment_description{
						.flags = 0,
						.format = g_window_surface_format.format,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.loadOp = g_window_clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
						.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
						.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
						.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				};

				constexpr VkAttachmentReference attachment_reference{.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

				const VkSubpassDescription sub_pass_description{
						.flags = 0,
						.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
						.inputAttachmentCount = 0,
						.pInputAttachments = nullptr,
						.colorAttachmentCount = 1,
						.pColorAttachments = &attachment_reference,
						.pResolveAttachments = nullptr,
						.pDepthStencilAttachment = nullptr,
						.preserveAttachmentCount = 0,
						.pPreserveAttachments = nullptr
				};

				constexpr VkSubpassDependency sub_pass_dependency{
						.srcSubpass = VK_SUBPASS_EXTERNAL,
						.dstSubpass = 0,
						.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
						.srcAccessMask = 0,
						.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
						.dependencyFlags = 0
				};

				const VkRenderPassCreateInfo render_pass_create_info{
						.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.attachmentCount = 1,
						.pAttachments = &attachment_description,
						.subpassCount = 1,
						.pSubpasses = &sub_pass_description,
						.dependencyCount = 1,
						.pDependencies = &sub_pass_dependency
				};

				vulkan_check_error(vkCreateRenderPass(g_device, &render_pass_create_info, g_allocation_callbacks, &g_pipeline_render_pass));
			}

			// Create Image Views
			{
				VkImageViewCreateInfo image_view_create_info{
						.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.image = VK_NULL_HANDLE,
						.viewType = VK_IMAGE_VIEW_TYPE_2D,
						.format = g_window_surface_format.format,
						.components =
						{.r = VK_COMPONENT_SWIZZLE_R, .g = VK_COMPONENT_SWIZZLE_G, .b = VK_COMPONENT_SWIZZLE_B, .a = VK_COMPONENT_SWIZZLE_A},
						.subresourceRange =
						{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1}
				};

				std::ranges::for_each(
					std::ranges::subrange{g_window_frames.get(), g_window_frames.get() + g_window_frame_total_count},
					[&](detail::frame_type& frame)
					{
						image_view_create_info.image = frame.back_buffer;
						vulkan_check_error(vkCreateImageView(g_device, &image_view_create_info, g_allocation_callbacks, &frame.back_buffer_view));
					}
				);
			}

			// Create Frame Buffer
			if (not g_pipeline_use_dynamic_rendering)
			{
				VkFramebufferCreateInfo frame_buffer_create_info{
						.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.renderPass = g_pipeline_render_pass,
						.attachmentCount = 1,
						.pAttachments = VK_NULL_HANDLE,
						.width = static_cast<std::uint32_t>(g_window_width),
						.height = static_cast<std::uint32_t>(g_window_height),
						.layers = 1
				};

				std::ranges::for_each(
					std::ranges::subrange{g_window_frames.get(), g_window_frames.get() + g_window_frame_total_count},
					[&](detail::frame_type& frame)
					{
						frame_buffer_create_info.pAttachments = &frame.back_buffer_view;
						vulkan_check_error(vkCreateFramebuffer(g_device, &frame_buffer_create_info, g_allocation_callbacks, &frame.frame_buffer));
					}
				);
			}
		}

		// command buffer
		{
			std::ranges::for_each(
				std::ranges::subrange{g_window_frames.get(), g_window_frames.get() + g_window_frame_total_count},
				[&](detail::frame_type& frame)
				{
					{
						const VkCommandPoolCreateInfo command_pool_create_info{
								.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
								.pNext = nullptr,
								.flags = 0,
								.queueFamilyIndex = g_queue_family
						};
						vulkan_check_error(vkCreateCommandPool(g_device, &command_pool_create_info, g_allocation_callbacks, &frame.command_pool));
					}
					{
						const VkCommandBufferAllocateInfo command_buffer_allocate_info{
								.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
								.pNext = nullptr,
								.commandPool = frame.command_pool,
								.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
								.commandBufferCount = 1
						};
						vulkan_check_error(
							vkAllocateCommandBuffers(g_device, &command_buffer_allocate_info, &frame.command_buffer)
						);
					}
					{
						constexpr VkFenceCreateInfo fence_create_info{
								.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT
						};
						vulkan_check_error(vkCreateFence(g_device, &fence_create_info, g_allocation_callbacks, &frame.fence));
					}
				}
			);

			std::ranges::for_each(
				std::ranges::subrange{g_window_frame_semaphores.get(), g_window_frame_semaphores.get() + g_window_frame_semaphore_total_count},
				[&](detail::frame_semaphore_type& frame_semaphore)
				{
					constexpr VkSemaphoreCreateInfo semaphore_create_info{
							.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0
					};
					vulkan_check_error(vkCreateSemaphore(
						g_device,
						&semaphore_create_info,
						g_allocation_callbacks,
						&frame_semaphore.image_acquired_semaphore
					));
					vulkan_check_error(
						vkCreateSemaphore(
							g_device,
							&semaphore_create_info,
							g_allocation_callbacks,
							&frame_semaphore.render_complete_semaphore
						));
				}
			);
		}
	}

	auto vulkan_setup_window() -> bool
	{
		// Check for WSI support
		{
			VkBool32 surface_support_result;
			vkGetPhysicalDeviceSurfaceSupportKHR(g_physical_device, g_queue_family, g_window_surface, &surface_support_result);
			if (surface_support_result != VK_TRUE)
			{
				std::println(std::cerr, "Vulkan Error: no WSI support on physical device");
				return false;
			}
		}

		// Select Surface Format
		{
			constexpr VkFormat request_surface_image_format[]{
					VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM
			};
			constexpr VkColorSpaceKHR request_surface_color_space{VK_COLORSPACE_SRGB_NONLINEAR_KHR};

			std::uint32_t surface_format_count;
			vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_window_surface, &surface_format_count, nullptr);
			std::vector<VkSurfaceFormatKHR> surface_format;
			surface_format.resize(surface_format_count);
			vkGetPhysicalDeviceSurfaceFormatsKHR(g_physical_device, g_window_surface, &surface_format_count, surface_format.data());

			// First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
			if (surface_format_count == 1)
			{
				if (surface_format.front().format == VK_FORMAT_UNDEFINED)
				{
					g_window_surface_format.format = request_surface_image_format[0];
					g_window_surface_format.colorSpace = request_surface_color_space;
				}
				else
				{
					g_window_surface_format = surface_format.front();
				}
			}
			else
			{
				// Request several formats, the first found will be used
				if (const auto result = [&]() -> std::optional<VkSurfaceFormatKHR>
					{
						for (const VkFormat& request_format: request_surface_image_format)
						{
							if (const auto it = std::ranges::find_if(
									surface_format,
									[&](const VkSurfaceFormatKHR& format) noexcept -> bool
									{
										return format.format == request_format and format.colorSpace == request_surface_color_space;
									}
								);
								it != surface_format.end())
							{
								return std::make_optional(*it);
							}
						}

						return std::nullopt;
					}();
					result.has_value())
				{
					g_window_surface_format = *result;
				}
				else
				{
					// If none of the requested image formats could be found, use the first available
					g_window_surface_format = surface_format.front();
				}
			}
		}

		// Select Present Mode
		{
			// todo: unlimited frame rate?
			constexpr VkPresentModeKHR request_present_mode[]
			{
					#if 1
					VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR,
					#endif
					VK_PRESENT_MODE_FIFO_KHR
			};

			// Request a certain mode and confirm that it is available. If not use VK_PRESENT_MODE_FIFO_KHR which is mandatory
			std::uint32_t surface_present_mode_count;
			vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical_device, g_window_surface, &surface_present_mode_count, nullptr);
			std::vector<VkPresentModeKHR> surface_present_mode;
			surface_present_mode.resize(surface_present_mode_count);
			vkGetPhysicalDeviceSurfacePresentModesKHR(g_physical_device, g_window_surface, &surface_present_mode_count, surface_present_mode.data());

			if (const auto mode_it = std::ranges::find_if(
					request_present_mode,
					[&](const VkPresentModeKHR& request_mode) noexcept -> bool { return std::ranges::contains(surface_present_mode, request_mode); }
				);
				mode_it != std::ranges::end(request_present_mode))
			{
				g_window_present_mode = *mode_it;
			}
			else
			{
				g_window_present_mode = VK_PRESENT_MODE_FIFO_KHR;
			}
		}

		vulkan_create_or_resize_window();
		return true;
	}

	GLFWwindowfocusfun g_glfw_callback_window_focus;
	GLFWcursorenterfun g_glfw_callback_window_cursor_enter;
	GLFWcursorposfun g_glfw_callback_window_cursor_position;
	GLFWmousebuttonfun g_glfw_callback_window_mouse_button;
	GLFWscrollfun g_glfw_callback_window_scroll;
	GLFWkeyfun g_glfw_callback_window_key;
	GLFWcharfun g_glfw_callback_window_char;
	GLFWmonitorfun g_glfw_callback_window_monitor;

	auto glfw_init() -> void
	{
		static auto callback_window_focus = [](GLFWwindow* window, const int focused)
		{
			std::println(std::cout, "callback_window_focus: window: 0x{:x}, focused: {}", reinterpret_cast<std::uintptr_t>(window), focused != 0);

			if (g_glfw_callback_window_focus)
			{
				g_glfw_callback_window_focus(window, focused);
			}
		};
		static auto callback_window_cursor_enter = [](GLFWwindow* window, const int entered)
		{
			std::println(std::cout,
			             "callback_window_cursor_enter: window: 0x{:x}, entered: {}",
			             reinterpret_cast<std::uintptr_t>(window),
			             entered != 0);

			if (g_glfw_callback_window_cursor_enter)
			{
				g_glfw_callback_window_cursor_enter(window, entered);
			}
		};
		static auto callback_window_cursor_position = [](GLFWwindow* window, const double x, const double y)
		{
			std::println(std::cout, "callback_window_cursor_position: window: 0x{:x}, x: {}, y: {}", reinterpret_cast<std::uintptr_t>(window), x, y);

			if (g_glfw_callback_window_cursor_position)
			{
				g_glfw_callback_window_cursor_position(window, x, y);
			}
		};
		static auto callback_window_mouse_button = [](GLFWwindow* window, const int button, const int action, const int mods)
		{
			std::println(
				std::cout,
				"callback_window_mouse_button: window: 0x{:x}, button: {}, action: {}, mods: {}",
				reinterpret_cast<std::uintptr_t>(window),
				button,
				action,
				mods
			);

			if (g_glfw_callback_window_mouse_button)
			{
				g_glfw_callback_window_mouse_button(window, button, action, mods);
			}
		};
		static auto callback_window_scroll = [](GLFWwindow* window, const double x, const double y)
		{
			std::println(std::cout, "callback_window_scroll: window: 0x{:x}, x: {}, y: {}", reinterpret_cast<std::uintptr_t>(window), x, y);

			if (g_glfw_callback_window_scroll)
			{
				g_glfw_callback_window_scroll(window, x, y);
			}
		};
		static auto callback_window_key = [](GLFWwindow* window, const int key_code, const int scan_code, const int action, const int mods)
		{
			std::println(
				std::cout,
				"callback_window_scroll: window: 0x{:x}, key_code: {}, scan_code: {}, action: {}, mods: {}",
				reinterpret_cast<std::uintptr_t>(window),
				key_code,
				scan_code,
				action,
				mods
			);

			if (g_glfw_callback_window_key)
			{
				g_glfw_callback_window_key(window, key_code, scan_code, action, mods);
			}
		};
		static auto callback_window_char = [](GLFWwindow* window, const unsigned int codepoint)
		{
			std::println(std::cout, "callback_window_char: window: 0x{:x}, codepoint: 0x{:x}", reinterpret_cast<std::uintptr_t>(window), codepoint);

			if (g_glfw_callback_window_char)
			{
				g_glfw_callback_window_char(window, codepoint);
			}
		};
		static auto callback_window_monitor = [](GLFWmonitor* monitor, const int event)
		{
			std::println(std::cout, "callback_window_monitor: monitor: 0x{:x}, event: {}", reinterpret_cast<std::uintptr_t>(monitor), event);

			if (g_glfw_callback_window_monitor)
			{
				g_glfw_callback_window_monitor(monitor, event);
			}
		};

		g_glfw_callback_window_focus = glfwSetWindowFocusCallback(g_window, callback_window_focus);
		g_glfw_callback_window_cursor_enter = glfwSetCursorEnterCallback(g_window, callback_window_cursor_enter);
		g_glfw_callback_window_cursor_position = glfwSetCursorPosCallback(g_window, callback_window_cursor_position);
		g_glfw_callback_window_mouse_button = glfwSetMouseButtonCallback(g_window, callback_window_mouse_button);
		g_glfw_callback_window_scroll = glfwSetScrollCallback(g_window, callback_window_scroll);
		g_glfw_callback_window_key = glfwSetKeyCallback(g_window, callback_window_key);
		g_glfw_callback_window_char = glfwSetCharCallback(g_window, callback_window_char);
		g_glfw_callback_window_monitor = glfwSetMonitorCallback(callback_window_monitor);
	}

	auto vulkan_init() -> void
	{
		if (g_font_sampler == VK_NULL_HANDLE)
		{
			constexpr VkSamplerCreateInfo sampler_create_info{
					.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.magFilter = VK_FILTER_LINEAR,
					.minFilter = VK_FILTER_LINEAR,
					.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
					.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
					.mipLodBias = .0f,
					.anisotropyEnable = 0,
					.maxAnisotropy = 1.f,
					.compareEnable = VK_FALSE,
					.compareOp = VK_COMPARE_OP_NEVER,
					.minLod = -1000,
					.maxLod = 1000,
					.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
					.unnormalizedCoordinates = VK_FALSE
			};
			vulkan_check_error(vkCreateSampler(g_device, &sampler_create_info, g_allocation_callbacks, &g_font_sampler));
		}

		if (g_pipeline_descriptor_set_layout == VK_NULL_HANDLE)
		{
			constexpr VkDescriptorSetLayoutBinding descriptor_set_layout_binding{
					.binding = 0,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
					.pImmutableSamplers = nullptr
			};
			const VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.bindingCount = 1,
					.pBindings = &descriptor_set_layout_binding
			};
			vulkan_check_error(vkCreateDescriptorSetLayout(
				g_device,
				&descriptor_set_layout_create_info,
				g_allocation_callbacks,
				&g_pipeline_descriptor_set_layout
			));
		}

		if (g_pipeline_layout == VK_NULL_HANDLE)
		{
			constexpr VkPushConstantRange push_constant_range{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = sizeof(float) * 0,
			                                                  .size = sizeof(float) * 4};
			const VkPipelineLayoutCreateInfo pipeline_layout_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.setLayoutCount = 1,
					.pSetLayouts = &g_pipeline_descriptor_set_layout,
					.pushConstantRangeCount = 1,
					.pPushConstantRanges = &push_constant_range
			};
			vulkan_check_error(vkCreatePipelineLayout(g_device, &pipeline_layout_create_info, g_allocation_callbacks, &g_pipeline_layout));
		}

		if (g_pipeline_shader_module_vertex == VK_NULL_HANDLE)
		{
			// #version 450 core
			// layout(location = 0) in vec2 aPos;
			// layout(location = 1) in vec2 aUV;
			// layout(location = 2) in vec4 aColor;
			// layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
			//
			// out gl_PerVertex { vec4 gl_Position; };
			// layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
			//
			// void main()
			// {
			//     Out.Color = aColor;
			//     Out.UV = aUV;
			//     gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
			// }

			constexpr static uint32_t shader_data[] = {
					0x07230203, 0x00010000, 0x00080001, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47,
					0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000a000f, 0x00000000, 0x00000004, 0x6e69616d,
					0x00000000, 0x0000000b, 0x0000000f, 0x00000015, 0x0000001b, 0x0000001c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
					0x00000004, 0x6e69616d, 0x00000000, 0x00030005, 0x00000009, 0x00000000, 0x00050006, 0x00000009, 0x00000000, 0x6f6c6f43,
					0x00000072, 0x00040006, 0x00000009, 0x00000001, 0x00005655, 0x00030005, 0x0000000b, 0x0074754f, 0x00040005, 0x0000000f,
					0x6c6f4361, 0x0000726f, 0x00030005, 0x00000015, 0x00565561, 0x00060005, 0x00000019, 0x505f6c67, 0x65567265, 0x78657472,
					0x00000000, 0x00060006, 0x00000019, 0x00000000, 0x505f6c67, 0x7469736f, 0x006e6f69, 0x00030005, 0x0000001b, 0x00000000,
					0x00040005, 0x0000001c, 0x736f5061, 0x00000000, 0x00060005, 0x0000001e, 0x73755075, 0x6e6f4368, 0x6e617473, 0x00000074,
					0x00050006, 0x0000001e, 0x00000000, 0x61635375, 0x0000656c, 0x00060006, 0x0000001e, 0x00000001, 0x61725475, 0x616c736e,
					0x00006574, 0x00030005, 0x00000020, 0x00006370, 0x00040047, 0x0000000b, 0x0000001e, 0x00000000, 0x00040047, 0x0000000f,
					0x0000001e, 0x00000002, 0x00040047, 0x00000015, 0x0000001e, 0x00000001, 0x00050048, 0x00000019, 0x00000000, 0x0000000b,
					0x00000000, 0x00030047, 0x00000019, 0x00000002, 0x00040047, 0x0000001c, 0x0000001e, 0x00000000, 0x00050048, 0x0000001e,
					0x00000000, 0x00000023, 0x00000000, 0x00050048, 0x0000001e, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x0000001e,
					0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
					0x00000007, 0x00000006, 0x00000004, 0x00040017, 0x00000008, 0x00000006, 0x00000002, 0x0004001e, 0x00000009, 0x00000007,
					0x00000008, 0x00040020, 0x0000000a, 0x00000003, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000003, 0x00040015,
					0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040020, 0x0000000e, 0x00000001,
					0x00000007, 0x0004003b, 0x0000000e, 0x0000000f, 0x00000001, 0x00040020, 0x00000011, 0x00000003, 0x00000007, 0x0004002b,
					0x0000000c, 0x00000013, 0x00000001, 0x00040020, 0x00000014, 0x00000001, 0x00000008, 0x0004003b, 0x00000014, 0x00000015,
					0x00000001, 0x00040020, 0x00000017, 0x00000003, 0x00000008, 0x0003001e, 0x00000019, 0x00000007, 0x00040020, 0x0000001a,
					0x00000003, 0x00000019, 0x0004003b, 0x0000001a, 0x0000001b, 0x00000003, 0x0004003b, 0x00000014, 0x0000001c, 0x00000001,
					0x0004001e, 0x0000001e, 0x00000008, 0x00000008, 0x00040020, 0x0000001f, 0x00000009, 0x0000001e, 0x0004003b, 0x0000001f,
					0x00000020, 0x00000009, 0x00040020, 0x00000021, 0x00000009, 0x00000008, 0x0004002b, 0x00000006, 0x00000028, 0x00000000,
					0x0004002b, 0x00000006, 0x00000029, 0x3f800000, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8,
					0x00000005, 0x0004003d, 0x00000007, 0x00000010, 0x0000000f, 0x00050041, 0x00000011, 0x00000012, 0x0000000b, 0x0000000d,
					0x0003003e, 0x00000012, 0x00000010, 0x0004003d, 0x00000008, 0x00000016, 0x00000015, 0x00050041, 0x00000017, 0x00000018,
					0x0000000b, 0x00000013, 0x0003003e, 0x00000018, 0x00000016, 0x0004003d, 0x00000008, 0x0000001d, 0x0000001c, 0x00050041,
					0x00000021, 0x00000022, 0x00000020, 0x0000000d, 0x0004003d, 0x00000008, 0x00000023, 0x00000022, 0x00050085, 0x00000008,
					0x00000024, 0x0000001d, 0x00000023, 0x00050041, 0x00000021, 0x00000025, 0x00000020, 0x00000013, 0x0004003d, 0x00000008,
					0x00000026, 0x00000025, 0x00050081, 0x00000008, 0x00000027, 0x00000024, 0x00000026, 0x00050051, 0x00000006, 0x0000002a,
					0x00000027, 0x00000000, 0x00050051, 0x00000006, 0x0000002b, 0x00000027, 0x00000001, 0x00070050, 0x00000007, 0x0000002c,
					0x0000002a, 0x0000002b, 0x00000028, 0x00000029, 0x00050041, 0x00000011, 0x0000002d, 0x0000001b, 0x0000000d, 0x0003003e,
					0x0000002d, 0x0000002c, 0x000100fd, 0x00010038
			};

			const VkShaderModuleCreateInfo shader_module_create_info{
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.codeSize = sizeof(shader_data),
					.pCode = shader_data
			};
			vulkan_check_error(vkCreateShaderModule(
					g_device,
					&shader_module_create_info,
					g_allocation_callbacks,
					&g_pipeline_shader_module_vertex
				)
			);
		}
		if (g_pipeline_shader_module_fragment == VK_NULL_HANDLE)
		{
			// #version 450 core
			// layout(location = 0) out vec4 fColor;
			// layout(set=0, binding=0) uniform sampler2D sTexture;
			// layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
			// void main()
			// {
			//     fColor = In.Color * texture(sTexture, In.UV.st);
			// }
			constexpr static uint32_t shader_data[] = {
					0x07230203, 0x00010000, 0x00080001, 0x0000001e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47,
					0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
					0x00000000, 0x00000009, 0x0000000d, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005,
					0x00000004, 0x6e69616d, 0x00000000, 0x00040005, 0x00000009, 0x6c6f4366, 0x0000726f, 0x00030005, 0x0000000b, 0x00000000,
					0x00050006, 0x0000000b, 0x00000000, 0x6f6c6f43, 0x00000072, 0x00040006, 0x0000000b, 0x00000001, 0x00005655, 0x00030005,
					0x0000000d, 0x00006e49, 0x00050005, 0x00000016, 0x78655473, 0x65727574, 0x00000000, 0x00040047, 0x00000009, 0x0000001e,
					0x00000000, 0x00040047, 0x0000000d, 0x0000001e, 0x00000000, 0x00040047, 0x00000016, 0x00000022, 0x00000000, 0x00040047,
					0x00000016, 0x00000021, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
					0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b,
					0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006, 0x00000002, 0x0004001e, 0x0000000b, 0x00000007,
					0x0000000a, 0x00040020, 0x0000000c, 0x00000001, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000001, 0x00040015,
					0x0000000e, 0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040020, 0x00000010, 0x00000001,
					0x00000007, 0x00090019, 0x00000013, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
					0x0003001b, 0x00000014, 0x00000013, 0x00040020, 0x00000015, 0x00000000, 0x00000014, 0x0004003b, 0x00000015, 0x00000016,
					0x00000000, 0x0004002b, 0x0000000e, 0x00000018, 0x00000001, 0x00040020, 0x00000019, 0x00000001, 0x0000000a, 0x00050036,
					0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x00000010, 0x00000011, 0x0000000d,
					0x0000000f, 0x0004003d, 0x00000007, 0x00000012, 0x00000011, 0x0004003d, 0x00000014, 0x00000017, 0x00000016, 0x00050041,
					0x00000019, 0x0000001a, 0x0000000d, 0x00000018, 0x0004003d, 0x0000000a, 0x0000001b, 0x0000001a, 0x00050057, 0x00000007,
					0x0000001c, 0x00000017, 0x0000001b, 0x00050085, 0x00000007, 0x0000001d, 0x00000012, 0x0000001c, 0x0003003e, 0x00000009,
					0x0000001d, 0x000100fd, 0x00010038
			};

			const VkShaderModuleCreateInfo shader_module_create_info{
					.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.codeSize = sizeof(shader_data),
					.pCode = shader_data
			};
			vulkan_check_error(vkCreateShaderModule(
					g_device,
					&shader_module_create_info,
					g_allocation_callbacks,
					&g_pipeline_shader_module_fragment
				)
			);
		}

		if (g_pipeline == VK_NULL_HANDLE)
		{
			const VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info_vertex{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = g_pipeline_shader_module_vertex,
					.pName = "main",
					.pSpecializationInfo = nullptr
			};
			const VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info_fragment{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = g_pipeline_shader_module_fragment,
					.pName = "main",
					.pSpecializationInfo = nullptr
			};
			const VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info[2]
			{
					pipeline_shader_stage_create_info_vertex,
					pipeline_shader_stage_create_info_fragment
			};

			constexpr VkVertexInputBindingDescription vertex_input_binding_description{
					.binding = 0, .stride = sizeof(vertex_type), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
			};
			constexpr VkVertexInputAttributeDescription vertex_input_attribute_description_position{
					.location = 0,
					.binding = vertex_input_binding_description.binding,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(vertex_type, position)
			};
			constexpr VkVertexInputAttributeDescription vertex_input_attribute_description_uv{
					.location = 1,
					.binding = vertex_input_binding_description.binding,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = offsetof(vertex_type, uv)
			};
			constexpr VkVertexInputAttributeDescription vertex_input_attribute_description_color{
					.location = 2,
					.binding = vertex_input_binding_description.binding,
					.format = VK_FORMAT_R8G8B8A8_UNORM,
					.offset = offsetof(vertex_type, color)
			};
			constexpr VkVertexInputAttributeDescription vertex_input_attribute_descriptions[3]{
					vertex_input_attribute_description_position,
					vertex_input_attribute_description_uv,
					vertex_input_attribute_description_color
			};
			const VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.vertexBindingDescriptionCount = 1,
					.pVertexBindingDescriptions = &vertex_input_binding_description,
					.vertexAttributeDescriptionCount = 3,
					.pVertexAttributeDescriptions = vertex_input_attribute_descriptions
			};

			constexpr VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
					.primitiveRestartEnable = VK_FALSE
			};

			constexpr VkPipelineViewportStateCreateInfo pipeline_viewport_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.viewportCount = 1,
					.pViewports = nullptr,
					.scissorCount = 1,
					.pScissors = nullptr
			};

			constexpr VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.depthClampEnable = VK_FALSE,
					.rasterizerDiscardEnable = VK_FALSE,
					.polygonMode = VK_POLYGON_MODE_FILL,
					.cullMode = VK_CULL_MODE_NONE,
					.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
					.depthBiasEnable = VK_FALSE,
					.depthBiasConstantFactor = .0f,
					.depthBiasClamp = .0f,
					.depthBiasSlopeFactor = .0f,
					.lineWidth = 1.f
			};

			const VkPipelineMultisampleStateCreateInfo pipeline_multi_sample_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.rasterizationSamples = g_pipeline_rasterization_msaa,
					.sampleShadingEnable = false,
					.minSampleShading = .0f,
					.pSampleMask = nullptr,
					.alphaToCoverageEnable = VK_FALSE,
					.alphaToOneEnable = VK_FALSE
			};

			constexpr VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.depthTestEnable = VK_FALSE,
					.depthWriteEnable = VK_FALSE,
					.depthCompareOp = VK_COMPARE_OP_NEVER,
					.depthBoundsTestEnable = VK_FALSE,
					.stencilTestEnable = VK_FALSE,
					.front = {},
					.back = {},
					.minDepthBounds = .0f,
					.maxDepthBounds = .0f
			};

			constexpr VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state{
					.blendEnable = VK_TRUE,
					.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
			};
			const VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.logicOpEnable = VK_FALSE,
					.logicOp = VK_LOGIC_OP_CLEAR,
					.attachmentCount = 1,
					.pAttachments = &pipeline_color_blend_attachment_state,
					.blendConstants = {}
			};

			constexpr VkDynamicState dynamic_state[2]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
			const VkPipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.dynamicStateCount = std::ranges::size(dynamic_state),
					.pDynamicStates = dynamic_state
			};

			VkGraphicsPipelineCreateInfo pipeline_create_info{
					.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
					.pNext = nullptr,
					.flags = g_pipeline_create_flags,
					.stageCount = 2,
					.pStages = pipeline_shader_stage_create_info,
					.pVertexInputState = &pipeline_vertex_input_state_create_info,
					.pInputAssemblyState = &pipeline_input_assembly_state_create_info,
					.pTessellationState = nullptr,
					.pViewportState = &pipeline_viewport_state_create_info,
					.pRasterizationState = &pipeline_rasterization_state_create_info,
					.pMultisampleState = &pipeline_multi_sample_state_create_info,
					.pDepthStencilState = &pipeline_depth_stencil_state_create_info,
					.pColorBlendState = &pipeline_color_blend_state_create_info,
					.pDynamicState = &pipeline_dynamic_state_create_info,
					.layout = g_pipeline_layout,
					.renderPass = g_pipeline_render_pass,
					.subpass = g_pipeline_sub_pass,
					.basePipelineHandle = VK_NULL_HANDLE,
					.basePipelineIndex = 0
			};
			if (g_pipeline_use_dynamic_rendering)
			{
				assert(g_pipeline_rendering_create_info.sType == VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR &&
					"pipeline_rendering_create_info sType must be VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR");
				assert(g_pipeline_rendering_create_info.pNext == nullptr && "pipeline_rendering_create_info.pNext must be NULL");

				pipeline_create_info.pNext = &g_pipeline_rendering_create_info;
				pipeline_create_info.renderPass = VK_NULL_HANDLE;
			}

			vulkan_check_error(vkCreateGraphicsPipelines(g_device, g_pipeline_cache, 1, &pipeline_create_info, g_allocation_callbacks, &g_pipeline));
		}
	}

	auto init() -> void
	{
		glfw_init();
		vulkan_init();
	}

	auto destroy_font_texture() -> void
	{
		if (g_font_descriptor_set != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(g_device, g_descriptor_pool, 1, &g_font_descriptor_set);
			g_font_descriptor_set = VK_NULL_HANDLE;
		}
		if (g_font_image_view != VK_NULL_HANDLE)
		{
			vkDestroyImageView(g_device, g_font_image_view, g_allocation_callbacks);
			g_font_image_view = VK_NULL_HANDLE;
		}
		if (g_font_image != VK_NULL_HANDLE)
		{
			vkDestroyImage(g_device, g_font_image, g_allocation_callbacks);
			g_font_image = VK_NULL_HANDLE;
		}
		if (g_font_memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(g_device, g_font_memory, g_allocation_callbacks);
			g_font_memory = VK_NULL_HANDLE;
		}
	}

	auto create_font_texture() -> void
	{
		if (g_font_descriptor_set != VK_NULL_HANDLE)
		{
			return;
		}

		// Destroy existing texture (if any)
		if (g_font_memory or g_font_image or g_font_image_view)
		{
			vkQueueWaitIdle(g_queue);
			destroy_font_texture();
		}

		// Create command pool/buffer
		if (g_font_command_pool == VK_NULL_HANDLE)
		{
			const VkCommandPoolCreateInfo command_pool_create_info{
					.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .pNext = nullptr, .flags = 0,
					.queueFamilyIndex = g_queue_family
			};
			vulkan_check_error(vkCreateCommandPool(
					g_device,
					&command_pool_create_info,
					g_allocation_callbacks,
					&g_font_command_pool
				)
			);
		}
		if (g_font_command_buffer == VK_NULL_HANDLE)
		{
			const VkCommandBufferAllocateInfo command_buffer_allocate_info{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					.pNext = nullptr,
					.commandPool = g_font_command_pool,
					.commandBufferCount = 1
			};
			vulkan_check_error(vkAllocateCommandBuffers(g_device, &command_buffer_allocate_info, &g_font_command_buffer));
		}

		// Start command buffer
		{
			vulkan_check_error(vkResetCommandPool(g_device, g_font_command_pool, 0));

			constexpr VkCommandBufferBeginInfo command_buffer_begin_info{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.pNext = nullptr,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
					.pInheritanceInfo = nullptr
			};
			vulkan_check_error(vkBeginCommandBuffer(g_font_command_buffer, &command_buffer_begin_info));
		}

		// todo: RGBA[8+8+8+8]
		const auto [pixels, width, height] = load_font();
		const auto upload_size = width * height * 4 * sizeof(unsigned char);

		// Create Image
		{
			const VkImageCreateInfo image_create_info{
					.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.imageType = VK_IMAGE_TYPE_2D,
					.format = VK_FORMAT_R8G8B8A8_UNORM,
					.extent = {.width = static_cast<std::uint32_t>(width), .height = static_cast<std::uint32_t>(height), .depth = 1},
					.mipLevels = 1,
					.arrayLayers = 1,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.tiling = VK_IMAGE_TILING_OPTIMAL,
					.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 0,
					.pQueueFamilyIndices = nullptr,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
			};
			vulkan_check_error(vkCreateImage(g_device, &image_create_info, g_allocation_callbacks, &g_font_image));

			VkMemoryRequirements memory_requirements;
			vkGetImageMemoryRequirements(g_device, g_font_image, &memory_requirements);

			const VkMemoryAllocateInfo memory_allocate_info{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.pNext = nullptr,
					.allocationSize = memory_requirements.size,
					.memoryTypeIndex = memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits)
			};
			vulkan_check_error(vkAllocateMemory(g_device, &memory_allocate_info, g_allocation_callbacks, &g_font_memory));
			vulkan_check_error(vkBindImageMemory(g_device, g_font_image, g_font_memory, 0));
		}

		// Create Image View
		{
			const VkImageViewCreateInfo image_view_create_info{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.image = g_font_image,
					.viewType = VK_IMAGE_VIEW_TYPE_2D,
					.format = VK_FORMAT_R8G8B8A8_UNORM,
					.components = {},
					.subresourceRange =
					{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = 0,
							.layerCount = 1,
					}
			};
			vulkan_check_error(vkCreateImageView(g_device, &image_view_create_info, g_allocation_callbacks, &g_font_image_view));
		}

		// Create Descriptor Set
		{
			const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
					.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					.pNext = nullptr,
					.descriptorPool = g_descriptor_pool,
					.descriptorSetCount = 1,
					.pSetLayouts = &g_pipeline_descriptor_set_layout
			};
			vulkan_check_error(vkAllocateDescriptorSets(g_device, &descriptor_set_allocate_info, &g_font_descriptor_set));

			const VkDescriptorImageInfo descriptor_image_info{
					.sampler = g_font_sampler, .imageView = g_font_image_view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
			const VkWriteDescriptorSet write_descriptor_set{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = g_font_descriptor_set,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &descriptor_image_info,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
			};
			vkUpdateDescriptorSets(g_device, 1, &write_descriptor_set, 0, nullptr);
		}

		// Create Upload Buffer
		{
			const VkBufferCreateInfo buffer_create_info{
					.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					.pNext = nullptr,
					.flags = 0,
					.size = upload_size,
					.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					.queueFamilyIndexCount = 0,
					.pQueueFamilyIndices = nullptr
			};

			VkBuffer upload_buffer;
			vulkan_check_error(vkCreateBuffer(g_device, &buffer_create_info, g_allocation_callbacks, &upload_buffer));

			VkMemoryRequirements memory_requirements;
			vkGetBufferMemoryRequirements(g_device, upload_buffer, &memory_requirements);

			const VkMemoryAllocateInfo memory_allocate_info{
					.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					.pNext = nullptr,
					.allocationSize = memory_requirements.size,
					.memoryTypeIndex = memory_type(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_requirements.memoryTypeBits)
			};

			VkDeviceMemory upload_buffer_memory;
			vulkan_check_error(vkAllocateMemory(g_device, &memory_allocate_info, g_allocation_callbacks, &upload_buffer_memory)
			);
			vulkan_check_error(vkBindBufferMemory(g_device, upload_buffer, upload_buffer_memory, 0));

			// Upload to Buffer
			unsigned char* mapped_memory;
			vulkan_check_error(vkMapMemory(g_device, upload_buffer_memory, 0, upload_size, 0, reinterpret_cast<void**>(&mapped_memory))
			);
			std::memcpy(mapped_memory, pixels, upload_size);
			const VkMappedMemoryRange mapped_memory_range{
					.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
					.pNext = nullptr,
					.memory = upload_buffer_memory,
					.offset = 0,
					.size = upload_size
			};
			vulkan_check_error(vkFlushMappedMemoryRanges(g_device, 1, &mapped_memory_range));
			vkUnmapMemory(g_device, upload_buffer_memory);

			// Copy to Image
			const VkImageMemoryBarrier copy_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = 0,
					.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = g_font_image,
					.subresourceRange =
					{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
					}
			};
			vkCmdPipelineBarrier(
				g_font_command_buffer,
				VK_PIPELINE_STAGE_HOST_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&copy_barrier
			);

			const VkBufferImageCopy region{
					.bufferOffset = 0,
					.bufferRowLength = 0,
					.bufferImageHeight = 0,
					.imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
					.imageOffset = 0,
					.imageExtent = {.width = static_cast<std::uint32_t>(width), .height = static_cast<std::uint32_t>(height), .depth = 1}
			};
			vkCmdCopyBufferToImage(
				g_font_command_buffer,
				upload_buffer,
				g_font_image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&region
			);

			const VkImageMemoryBarrier use_barrier{
					.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
					.pNext = nullptr,
					.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
					.image = g_font_image,
					.subresourceRange =
					{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
					}
			};
			vkCmdPipelineBarrier(
				g_font_command_buffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0,
				nullptr,
				0,
				nullptr,
				1,
				&use_barrier
			);

			// End command buffer
			const VkSubmitInfo submit_info{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.pNext = nullptr,
					.waitSemaphoreCount = 0,
					.pWaitSemaphores = nullptr,
					.pWaitDstStageMask = nullptr,
					.commandBufferCount = 1,
					.pCommandBuffers = &g_font_command_buffer,
					.signalSemaphoreCount = 0,
					.pSignalSemaphores = nullptr
			};
			vulkan_check_error(vkEndCommandBuffer(g_font_command_buffer));
			vulkan_check_error(vkQueueSubmit(g_queue, 1, &submit_info, VK_NULL_HANDLE));
			vulkan_check_error(vkQueueWaitIdle(g_queue));

			vkDestroyBuffer(g_device, upload_buffer, g_allocation_callbacks);
			vkFreeMemory(g_device, upload_buffer_memory, g_allocation_callbacks);
		}
	}

	auto new_frame() -> void
	{
		// checked by callee
		create_font_texture();

		glfwGetWindowSize(g_window, &g_window_width, &g_window_height);
		glfwGetFramebufferSize(g_window, &g_window_frame_buffer_width, &g_window_frame_buffer_height);
	}

	auto frame_render() -> void
	{
		const auto image_acquired_semaphore = g_window_frame_semaphores[g_window_frame_semaphore_current_index].image_acquired_semaphore;
		const auto render_complete_semaphore = g_window_frame_semaphores[g_window_frame_semaphore_current_index].render_complete_semaphore;

		if (const auto result = vkAcquireNextImageKHR(
				g_device,
				g_window_swap_chain,
				std::numeric_limits<std::uint64_t>::max(),
				image_acquired_semaphore,
				VK_NULL_HANDLE,
				&g_window_frame_current_index
			);
			result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
		{
			g_window_swap_chain_rebuild_required = true;
			return;
		}
		else
		{
			vulkan_check_error(result);
		}

		const auto& this_frame = g_window_frames[g_window_frame_current_index];
		{
			vulkan_check_error(vkWaitForFences(g_device, 1, &this_frame.fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max()));
			vulkan_check_error(vkResetFences(g_device, 1, &this_frame.fence));
		}
		{
			vulkan_check_error(vkResetCommandPool(g_device, this_frame.command_pool, 0));

			constexpr VkCommandBufferBeginInfo command_buffer_begin_info{
					.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					.pNext = nullptr,
					.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
					.pInheritanceInfo = nullptr
			};
			vulkan_check_error(vkBeginCommandBuffer(this_frame.command_buffer, &command_buffer_begin_info));
		}
		{
			const VkRenderPassBeginInfo render_pass_begin_info{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = nullptr,
					.renderPass = g_pipeline_render_pass,
					.framebuffer = this_frame.frame_buffer,
					.renderArea =
					{.offset = {.x = 0, .y = 0},
					 .extent = {.width = static_cast<std::uint32_t>(g_window_width), .height = static_cast<std::uint32_t>(g_window_height)}},
					.clearValueCount = 1,
					.pClearValues = &g_window_clear_value
			};
			vkCmdBeginRenderPass(this_frame.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
		}

		if (g_window_render_buffer == nullptr)
		{
			g_window_render_buffer_total_count = g_window_frame_total_count;
			g_window_render_buffer_maker();
		}
		g_window_render_buffer_current_index = (g_window_render_buffer_current_index + 1) % g_window_render_buffer_total_count;

		auto& this_render_buffer = g_window_render_buffer[g_window_render_buffer_current_index];
		{
			if (g_draw_data.total_vertex_size() > 0)
			{
				static auto resize_buffer =
						[](VkBuffer& buffer, VkDeviceMemory& memory, const VkDeviceSize new_size, const VkBufferUsageFlagBits usage)
				{
					if (buffer != VK_NULL_HANDLE)
					{
						vkDestroyBuffer(g_device, buffer, g_allocation_callbacks);
					}
					if (memory != VK_NULL_HANDLE)
					{
						vkFreeMemory(g_device, memory, g_allocation_callbacks);
					}

					const VkBufferCreateInfo buffer_create_info{
							.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
							.pNext = nullptr,
							.flags = 0,
							.size = new_size,
							.usage = static_cast<VkBufferUsageFlags>(usage),
							.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
							.queueFamilyIndexCount = 0,
							.pQueueFamilyIndices = nullptr
					};
					vulkan_check_error(vkCreateBuffer(g_device, &buffer_create_info, g_allocation_callbacks, &buffer));

					VkMemoryRequirements memory_requirements;
					vkGetBufferMemoryRequirements(g_device, buffer, &memory_requirements);

					const VkMemoryAllocateInfo memory_allocate_info{
							.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
							.pNext = nullptr,
							.allocationSize = memory_requirements.size,
							.memoryTypeIndex = memory_type(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_requirements.memoryTypeBits)
					};
					vulkan_check_error(vkAllocateMemory(g_device, &memory_allocate_info, g_allocation_callbacks, &memory));

					vulkan_check_error(vkBindBufferMemory(g_device, buffer, memory, 0));
				};

				// Create or resize the vertex/index buffers
				const auto vertex_size = g_draw_data.total_vertex_size() * sizeof(vertex_type);
				const auto index_size = g_draw_data.total_index_size() * sizeof(vertex_index_type);

				if (this_render_buffer.vertex_buffer == VK_NULL_HANDLE or this_render_buffer.vertex_count < vertex_size)
				{
					resize_buffer(
						this_render_buffer.vertex_buffer,
						this_render_buffer.vertex_buffer_memory,
						vertex_size,
						VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
					);
				}
				if (this_render_buffer.index_buffer == VK_NULL_HANDLE or this_render_buffer.index_count < index_size)
				{
					resize_buffer(
						this_render_buffer.index_buffer,
						this_render_buffer.index_buffer_memory,
						index_size,
						VK_BUFFER_USAGE_INDEX_BUFFER_BIT
					);
				}

				// Upload vertex/index data into a single contiguous GPU buffer
				{
					struct target_vertex_type
					{
						float position[2];
						float uv[2];
						std::uint32_t color;
					};
					static_assert(sizeof(target_vertex_type) == sizeof(vertex_type));

					target_vertex_type* mapped_vertex = nullptr;
					vertex_index_type* mapped_index = nullptr;
					vulkan_check_error(vkMapMemory(
						g_device,
						this_render_buffer.vertex_buffer_memory,
						0,
						vertex_size,
						0,
						reinterpret_cast<void**>(&mapped_vertex)
					));
					vulkan_check_error(
						vkMapMemory(g_device, this_render_buffer.index_buffer_memory, 0, index_size, 0, reinterpret_cast<void**>(&mapped_index))
					);

					for (const auto& [vertex_list, index_list]: g_draw_data.draw_lists)
					{
						std::ranges::transform(
							vertex_list.vertices(),
							mapped_vertex,
							[](const auto& vertex) -> target_vertex_type
							{
								return {.position = {vertex.position.x, vertex.position.y},
								        .uv = {vertex.uv.x, vertex.uv.y},
								        .color = vertex.color.to(primitive::color_format<primitive::ColorFormat::A_B_G_R>{})
								};
							}
						);
						std::ranges::copy(index_list, mapped_index);

						mapped_vertex += vertex_list.size();
						mapped_index += index_list.size();
					}

					const VkMappedMemoryRange mapped_memory_range_vertex{
							.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
							.pNext = nullptr,
							.memory = this_render_buffer.vertex_buffer_memory,
							.offset = 0,
							.size = VK_WHOLE_SIZE
					};
					const VkMappedMemoryRange mapped_memory_range_index{
							.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
							.pNext = nullptr,
							.memory = this_render_buffer.index_buffer_memory,
							.offset = 0,
							.size = VK_WHOLE_SIZE
					};
					const VkMappedMemoryRange mapped_memory_range[2]{mapped_memory_range_vertex, mapped_memory_range_index};
					vulkan_check_error(vkFlushMappedMemoryRanges(g_device, 2, mapped_memory_range));

					vkUnmapMemory(g_device, this_render_buffer.vertex_buffer_memory);
					vkUnmapMemory(g_device, this_render_buffer.index_buffer_memory);
				}
			}

			// Setup desired Vulkan state
			{
				// Bind pipeline
				vkCmdBindPipeline(this_frame.command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_pipeline);

				// Bind Vertex And Index Buffer
				if (g_draw_data.total_vertex_size() > 0)
				{
					constexpr VkDeviceSize offset[1]{};
					vkCmdBindVertexBuffers(this_frame.command_buffer, 0, 1, &this_render_buffer.vertex_buffer, offset);
					vkCmdBindIndexBuffer(
						this_frame.command_buffer,
						this_render_buffer.index_buffer,
						0,
						sizeof(vertex_index_type) == 2 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32
					);
				}

				// Setup viewport
				{
					const VkViewport viewport{
							.x = 0,
							.y = 0,
							.width = static_cast<float>(g_window_frame_buffer_width),
							.height = static_cast<float>(g_window_frame_buffer_height),
							.minDepth = .0f,
							.maxDepth = 1.f
					};
					vkCmdSetViewport(this_frame.command_buffer, 0, 1, &viewport);
				}

				// Setup scale and translation
				{
					const float scale[2]{2.f / g_draw_data.display_rect.width(), 2.f / g_draw_data.display_rect.height()};
					const float translate[2]{
							-1.f - g_draw_data.display_rect.left_top().x * scale[0], -1.f - g_draw_data.display_rect.left_top().y * scale[1]
					};

					vkCmdPushConstants(
						this_frame.command_buffer,
						g_pipeline_layout,
						VK_SHADER_STAGE_VERTEX_BIT,
						sizeof(float) * 0,
						sizeof(float) * 2,
						scale
					);
					vkCmdPushConstants(
						this_frame.command_buffer,
						g_pipeline_layout,
						VK_SHADER_STAGE_VERTEX_BIT,
						sizeof(float) * 2,
						sizeof(float) * 2,
						translate
					);
				}
			}

			// Render command lists
			{
				std::ranges::for_each(
					g_draw_data.draw_lists,
					[&this_frame,
						display_rect = g_draw_data.display_rect,
						offset_vertex = static_cast<std::uint32_t>(0),
						offset_index = static_cast<std::uint32_t>(0)
					](const draw_list_type& draw_list) mutable
					{
						// todo: clip rect
						const VkRect2D scissor{
								.offset =
								{.x = static_cast<std::int32_t>(display_rect.left_top().x),
								 .y = static_cast<std::int32_t>(display_rect.left_top().y)},
								.extent =
								{.width = static_cast<std::uint32_t>(display_rect.width()),
								 .height = static_cast<std::uint32_t>(display_rect.height())}
						};
						vkCmdSetScissor(this_frame.command_buffer, 0, 1, &scissor);

						// Bind DescriptorSet with font
						vkCmdBindDescriptorSets(
							this_frame.command_buffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							g_pipeline_layout,
							0,
							1,
							&g_font_descriptor_set,
							0,
							nullptr
						);

						// Draw
						vkCmdDrawIndexed(this_frame.command_buffer, draw_list.index_list.size(), 1, offset_index, offset_vertex, 0);

						offset_vertex += draw_list.vertex_list.size();
						offset_index += draw_list.index_list.size();
					}
				);
			}

			const VkRect2D scissor = {.offset = {.x = 0, .y = 0},
			                          .extent = {.width = static_cast<std::uint32_t>(g_window_frame_buffer_width),
			                                     .height = static_cast<std::uint32_t>(g_window_frame_buffer_height)}};
			vkCmdSetScissor(this_frame.command_buffer, 0, 1, &scissor);
		}

		// Submit command buffer
		vkCmdEndRenderPass(this_frame.command_buffer);
		{
			constexpr VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			const VkSubmitInfo submit_info{
					.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					.pNext = nullptr,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &image_acquired_semaphore,
					.pWaitDstStageMask = &wait_stage,
					.commandBufferCount = 1,
					.pCommandBuffers = &this_frame.command_buffer,
					.signalSemaphoreCount = 1,
					.pSignalSemaphores = &render_complete_semaphore
			};

			vulkan_check_error(vkEndCommandBuffer(this_frame.command_buffer));
			vulkan_check_error(vkQueueSubmit(g_queue, 1, &submit_info, this_frame.fence));
		}
	}

	auto frame_present() -> void
	{
		if (g_window_swap_chain_rebuild_required)
		{
			return;
		}

		const auto render_complete_semaphore = g_window_frame_semaphores[g_window_frame_semaphore_current_index].render_complete_semaphore;
		const VkPresentInfoKHR present_info{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &render_complete_semaphore,
				.swapchainCount = 1,
				.pSwapchains = &g_window_swap_chain,
				.pImageIndices = &g_window_frame_current_index,
				.pResults = nullptr
		};

		if (const auto result = vkQueuePresentKHR(g_queue, &present_info); result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR)
		{
			g_window_swap_chain_rebuild_required = true;
			return;
		}
		else
		{
			vulkan_check_error(result);
		}

		g_window_frame_semaphore_current_index = (g_window_frame_semaphore_current_index + 1) % g_window_frame_semaphore_total_count;
	}

	auto destroy_device_objects() -> void
	{
		// ==============
		// render buffer
		g_window_render_buffer_destroyer(g_device, g_allocation_callbacks);

		// ==============
		// font
		destroy_font_texture();
		if (g_font_sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(g_device, g_font_sampler, g_allocation_callbacks);
			g_font_sampler = VK_NULL_HANDLE;
		}
		if (g_font_command_buffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(g_device, g_font_command_pool, 1, &g_font_command_buffer);
			g_font_command_buffer = VK_NULL_HANDLE;
		}
		if (g_font_command_pool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(g_device, g_font_command_pool, g_allocation_callbacks);
			g_font_command_pool = VK_NULL_HANDLE;
		}

		// ==============
		// pipeline
		if (g_pipeline_shader_module_vertex != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(g_device, g_pipeline_shader_module_vertex, g_allocation_callbacks);
			g_pipeline_shader_module_vertex = VK_NULL_HANDLE;
		}
		if (g_pipeline_shader_module_fragment != VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(g_device, g_pipeline_shader_module_fragment, g_allocation_callbacks);
			g_pipeline_shader_module_fragment = VK_NULL_HANDLE;
		}
		if (g_pipeline_layout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(g_device, g_pipeline_layout, g_allocation_callbacks);
			g_pipeline_layout = VK_NULL_HANDLE;
		}
		if (g_pipeline_descriptor_set_layout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(g_device, g_pipeline_descriptor_set_layout, g_allocation_callbacks);
			g_pipeline_descriptor_set_layout = VK_NULL_HANDLE;
		}
		if (g_pipeline)
		{
			vkDestroyPipeline(g_device, g_pipeline, g_allocation_callbacks);
			g_pipeline = VK_NULL_HANDLE;
		}
	}

	auto shutdown() -> void
	{
		destroy_device_objects();

		glfwSetWindowFocusCallback(g_window, g_glfw_callback_window_focus);
		glfwSetCursorEnterCallback(g_window, g_glfw_callback_window_cursor_enter);
		glfwSetCursorPosCallback(g_window, g_glfw_callback_window_cursor_position);
		glfwSetMouseButtonCallback(g_window, g_glfw_callback_window_mouse_button);
		glfwSetScrollCallback(g_window, g_glfw_callback_window_scroll);
		glfwSetKeyCallback(g_window, g_glfw_callback_window_key);
		glfwSetCharCallback(g_window, g_glfw_callback_window_char);
		glfwSetMonitorCallback(g_glfw_callback_window_monitor);
	}

	auto vulkan_cleanup_window() -> void
	{
		vulkan_check_error(vkDeviceWaitIdle(g_device));

		g_window_frames_destroyer(g_device, g_allocation_callbacks);
		g_window_frame_semaphores_destroyer(g_device, g_allocation_callbacks);

		vkDestroyPipeline(g_device, g_pipeline, g_allocation_callbacks);
		vkDestroyRenderPass(g_device, g_pipeline_render_pass, g_allocation_callbacks);
		vkDestroySwapchainKHR(g_device, g_window_swap_chain, g_allocation_callbacks);
		vkDestroySurfaceKHR(g_instance, g_window_surface, g_allocation_callbacks);
	}

	auto vulkan_cleanup() -> void
	{
		vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocation_callbacks);

		const auto destroy_debug_report_callback =
				reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(g_instance, "vkDestroyDebugReportCallbackEXT"));
		destroy_debug_report_callback(g_instance, g_debug_report_callback, g_allocation_callbacks);

		vkDestroyDevice(g_device, g_allocation_callbacks);
		vkDestroyInstance(g_instance, g_allocation_callbacks);
	}
}
