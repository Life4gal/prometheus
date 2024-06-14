#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <vector>
#include <cassert>
#include <print>
#include <iostream>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "font.hpp"

namespace
{
	using namespace gal::prometheus;

	using point_type = primitive::basic_point<float, 2>;
	using rect_type = primitive::basic_rect<float, 2>;
	using vertex_type = primitive::basic_vertex<point_type>;
	using vertex_index_type = std::uint16_t;

	namespace my_glfw
	{
		struct data_type
		{
			GLFWwindow* window;
			int width;
			int height;
		};

		data_type g_data;

		auto error_callback(const int error, const char* message) -> void //
		{
			std::println(std::cerr, "GLFW Error {}: {}", error, message);
		}

		[[nodiscard]] auto init(GLFWwindow* window) -> bool
		{
			g_data.window = window;
			return true;
		}

		auto shutdown() -> void //
		{
			// todo
		}

		auto new_frame() -> void //
		{
			glfwGetWindowSize(g_data.window, &g_data.width, &g_data.height);
		}
	} // namespace my_glfw

	namespace my_vulkan
	{
		struct window_type
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

			VkSurfaceKHR surface{VK_NULL_HANDLE};
			VkSurfaceFormatKHR surface_format{};

			// error if not set
			VkPresentModeKHR present_mode{VK_PRESENT_MODE_MAX_ENUM_KHR};

			[[nodiscard]] auto min_image_count_of_present_mode() const noexcept -> std::uint32_t
			{
				if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return 3;
				}
				if (present_mode == VK_PRESENT_MODE_FIFO_KHR || present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
				{
					return 2;
				}
				if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				{
					return 1;
				}

				std::unreachable();
			}

			VkSwapchainKHR swap_chain{VK_NULL_HANDLE};

			int width{0};
			int height{0};

			VkRenderPass render_pass{VK_NULL_HANDLE};
			// The window pipeline may use a different VkRenderPass than the one passed in `init_info_type`
			VkPipeline pipeline{VK_NULL_HANDLE};

			bool use_dynamic_rendering{false};
			bool clear_enable{true};
			VkClearValue clear_value{};

			std::unique_ptr<frame_type[]> frames;
			std::uint32_t frame_current_index;
			std::uint32_t frame_total_count;

			std::unique_ptr<frame_semaphore_type[]> frame_semaphores;
			std::uint32_t frame_semaphore_current_index;
			std::uint32_t frame_semaphore_total_count;

		private:
			static auto destroy_frame(const VkDevice device, frame_type& frame, const VkAllocationCallbacks* allocation_callbacks) -> void
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

		public:
			auto create_frames() -> void //
			{
				frames = std::make_unique<frame_type[]>(frame_total_count);
			}

			auto destroy_frames(const VkDevice device, const VkAllocationCallbacks* allocation_callbacks) -> void
			{
				std::ranges::for_each(
					std::ranges::subrange{frames.get(), frames.get() + frame_total_count},
					[&](frame_type& frame) { destroy_frame(device, frame, allocation_callbacks); }
				);

				frames.reset();
				frame_current_index = 0;
				frame_total_count = 0;
			}

		private:
			static auto destroy_frame_semaphore(
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

		public:
			auto create_frame_semaphores() -> void //
			{
				frame_semaphores = std::make_unique<frame_semaphore_type[]>(frame_semaphore_total_count);
			}

			auto destroy_frame_semaphores(const VkDevice device, const VkAllocationCallbacks* allocation_callbacks) -> void
			{
				std::ranges::for_each(
					std::ranges::subrange{frame_semaphores.get(), frame_semaphores.get() + frame_semaphore_total_count},
					[&](frame_semaphore_type& frame_semaphore) { destroy_frame_semaphore(device, frame_semaphore, allocation_callbacks); }
				);

				frame_semaphores.reset();
				frame_semaphore_current_index = 0;
				frame_semaphore_total_count = 0;
			}
		};

		struct init_info_type
		{
			VkInstance instance{VK_NULL_HANDLE};

			VkPhysicalDevice physical_device{VK_NULL_HANDLE};
			VkDevice device{VK_NULL_HANDLE};

			std::uint32_t queue_family{0};
			VkQueue queue{VK_NULL_HANDLE};

			// VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT
			VkDescriptorPool descriptor_pool{VK_NULL_HANDLE};

			// Ignored if using dynamic rendering
			VkRenderPass render_pass{VK_NULL_HANDLE};

			VkSampleCountFlagBits msaa_samples{VK_SAMPLE_COUNT_1_BIT};

			VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
			std::uint32_t sub_pass{0};

			bool use_dynamic_rendering{false};
			VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};

			const VkAllocationCallbacks* allocation_callbacks{nullptr};
			void (*check_result_callback)(VkResult){nullptr};
		};

		struct data_type
		{
			init_info_type init_info{};

			VkDeviceSize memory_buffer_alignment{256};
			VkPipelineCreateFlags pipeline_create_flags{0};
			VkDescriptorSetLayout descriptor_set_layout{VK_NULL_HANDLE};
			VkPipelineLayout pipeline_layout{VK_NULL_HANDLE};
			VkShaderModule shader_module_vertex{VK_NULL_HANDLE};
			VkShaderModule shader_module_fragment{VK_NULL_HANDLE};
			VkPipeline pipeline{VK_NULL_HANDLE};

			VkSampler font_sampler{VK_NULL_HANDLE};
			VkDeviceMemory font_memory{VK_NULL_HANDLE};
			VkImage font_image{VK_NULL_HANDLE};
			VkImageView font_view{VK_NULL_HANDLE};
			VkDescriptorSet font_descriptor_set{VK_NULL_HANDLE};
			VkCommandPool font_command_pool{VK_NULL_HANDLE};
			VkCommandBuffer font_command_buffer{VK_NULL_HANDLE};

			struct frame_render_buffer_type
			{
				VkDeviceMemory vertex_buffer_memory{VK_NULL_HANDLE};
				VkDeviceSize vertex_count{0};
				VkBuffer vertex_buffer{VK_NULL_HANDLE};

				VkDeviceMemory index_buffer_memory{VK_NULL_HANDLE};
				VkDeviceSize index_count{0};
				VkBuffer index_buffer{VK_NULL_HANDLE};
			};

			using window_render_buffer_type = std::vector<frame_render_buffer_type>;
			window_render_buffer_type window_render_buffer;
		};

		window_type g_window;
		data_type g_data;

		auto check_error(const VkResult result) -> void
		{
			if (result == VK_SUCCESS)
			{
				return;
			}

			std::println("VULKAN Error: {}", meta::name_of(result));
			// todo
			std::abort();
		}

		[[nodiscard]] auto memory_type(const VkMemoryPropertyFlags property_flag, const std::uint32_t type_bits) -> std::uint32_t
		{
			auto& init_info = g_data.init_info;

			VkPhysicalDeviceMemoryProperties memory_properties;
			vkGetPhysicalDeviceMemoryProperties(init_info.physical_device, &memory_properties);

			for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
			{
				if ((memory_properties.memoryTypes[i].propertyFlags & property_flag) == property_flag and type_bits & (1 << i))
				{
					return i;
				}
			}
			return std::numeric_limits<std::uint32_t>::max();
		}

		auto create_or_resize_window(
			VkInstance instance,
			VkPhysicalDevice physical_device,
			VkDevice device,
			std::uint32_t queue_family,
			const VkAllocationCallbacks* allocation_callbacks,
			int width,
			int height,
			std::uint32_t min_image_count
		) -> void;

		auto setup_vulkan_window(
			const VkInstance instance,
			const VkPhysicalDevice physical_device,
			const VkDevice device,
			const std::uint32_t queue_family,
			const VkAllocationCallbacks* allocation_callbacks,
			const VkSurfaceKHR surface,
			const int width,
			const int height,
			const std::uint32_t min_image_count
		) -> bool
		{
			auto& window = g_window;

			window.surface = surface;

			// Check for WSI support
			{
				VkBool32 surface_support_result;
				vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family, window.surface, &surface_support_result);
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
				vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, window.surface, &surface_format_count, nullptr);
				std::vector<VkSurfaceFormatKHR> surface_format;
				surface_format.resize(surface_format_count);
				vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, window.surface, &surface_format_count, surface_format.data());

				// First check if only one format, VK_FORMAT_UNDEFINED, is available, which would imply that any format is available
				if (surface_format_count == 1)
				{
					if (surface_format.front().format == VK_FORMAT_UNDEFINED)
					{
						window.surface_format.format = request_surface_image_format[0];
						window.surface_format.colorSpace = request_surface_color_space;
					}
					else
					{
						window.surface_format = surface_format.front();
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
						window.surface_format = *result;
					}
					else
					{
						// If none of the requested image formats could be found, use the first available
						window.surface_format = surface_format.front();
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
				vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, window.surface, &surface_present_mode_count, nullptr);
				std::vector<VkPresentModeKHR> surface_present_mode;
				surface_present_mode.resize(surface_present_mode_count);
				vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, window.surface, &surface_present_mode_count, surface_present_mode.data());

				if (const auto mode_it = std::ranges::find_if(
						request_present_mode,
						[&](const VkPresentModeKHR& request_mode) noexcept -> bool
						{
							return std::ranges::contains(surface_present_mode, request_mode);
						}
					);
					mode_it != std::ranges::end(request_present_mode))
				{
					window.present_mode = *mode_it;
				}
				else
				{
					window.present_mode = VK_PRESENT_MODE_FIFO_KHR;
				}
			}

			create_or_resize_window(instance, physical_device, device, queue_family, allocation_callbacks, width, height, min_image_count);
			return true;
		}

		auto create_or_resize_window(
			const VkInstance instance,
			const VkPhysicalDevice physical_device,
			const VkDevice device,
			const std::uint32_t queue_family,
			const VkAllocationCallbacks* allocation_callbacks,
			const int width,
			const int height,
			const std::uint32_t min_image_count
		) -> void
		{
			auto& window = g_window;

			// swap-chain
			{
				auto old_swap_chain = std::exchange(g_window.swap_chain, VK_NULL_HANDLE);
				check_error(vkDeviceWaitIdle(device));

				window.destroy_frames(device, allocation_callbacks);
				window.destroy_frame_semaphores(device, allocation_callbacks);

				if (window.render_pass != VK_NULL_HANDLE)
				{
					vkDestroyRenderPass(device, window.render_pass, allocation_callbacks);
					window.render_pass = VK_NULL_HANDLE;
				}
				if (window.pipeline != VK_NULL_HANDLE)
				{
					vkDestroyPipeline(device, window.pipeline, allocation_callbacks);
					window.pipeline = VK_NULL_HANDLE;
				}

				// Create SwapChain
				{
					VkSurfaceCapabilitiesKHR surface_capabilities;
					check_error(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, window.surface, &surface_capabilities));

					// If min image count was not specified, request different count of images dependent on selected present mode
					auto image_count = min_image_count == 0 ? window.min_image_count_of_present_mode() : min_image_count;
					if (image_count < surface_capabilities.minImageCount)
					{
						image_count = surface_capabilities.minImageCount;
					}
					if (surface_capabilities.maxImageCount != 0 and image_count > surface_capabilities.maxImageCount)
					{
						image_count = surface_capabilities.maxImageCount;
					}

					if (surface_capabilities.currentExtent.width == std::numeric_limits<std::uint32_t>::max())
					{
						window.width = width;
						window.height = height;
					}
					else
					{
						window.width = static_cast<int>(surface_capabilities.currentExtent.width);
						window.height = static_cast<int>(surface_capabilities.currentExtent.height);
					}

					VkSwapchainCreateInfoKHR swap_chain_create_info{
							.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
							.pNext = nullptr,
							.flags = 0,
							.surface = window.surface,
							.minImageCount = image_count,
							.imageFormat = window.surface_format.format,
							.imageColorSpace = window.surface_format.colorSpace,
							.imageExtent = {.width = static_cast<std::uint32_t>(window.width), .height = static_cast<std::uint32_t>(window.height)},
							.imageArrayLayers = 1,
							.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
							.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
							.queueFamilyIndexCount = 0,
							.pQueueFamilyIndices = nullptr,
							.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
							.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
							.presentMode = window.present_mode,
							.clipped = VK_TRUE,
							.oldSwapchain = old_swap_chain
					};
					check_error(vkCreateSwapchainKHR(device, &swap_chain_create_info, allocation_callbacks, &window.swap_chain));

					check_error(vkGetSwapchainImagesKHR(device, window.swap_chain, &window.frame_total_count, nullptr));
					window.frame_semaphore_total_count = window.frame_total_count + 1;

					window.create_frames();
					window.create_frame_semaphores();

					VkImage back_buffer[16]{};
					check_error(vkGetSwapchainImagesKHR(device, window.swap_chain, &window.frame_total_count, back_buffer));
					for (std::uint32_t i = 0; i < window.frame_total_count; ++i)
					{
						window.frames[i].back_buffer = back_buffer[i];
					}
				}

				if (old_swap_chain)
				{
					vkDestroySwapchainKHR(device, old_swap_chain, allocation_callbacks);
				}

				// Create Render Pass
				if (not window.use_dynamic_rendering)
				{
					const VkAttachmentDescription attachment_description{
							.flags = 0,
							.format = window.surface_format.format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = window.clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
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

					check_error(vkCreateRenderPass(device, &render_pass_create_info, allocation_callbacks, &window.render_pass));
				}

				// Create Image Views
				{
					VkImageViewCreateInfo image_view_create_info{
							.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
							.pNext = nullptr,
							.flags = 0,
							.image = VK_NULL_HANDLE,
							.viewType = VK_IMAGE_VIEW_TYPE_2D,
							.format = window.surface_format.format,
							.components =
							{.r = VK_COMPONENT_SWIZZLE_R,
							 .g = VK_COMPONENT_SWIZZLE_G,
							 .b = VK_COMPONENT_SWIZZLE_B,
							 .a = VK_COMPONENT_SWIZZLE_A},
							.subresourceRange =
							{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
							}
					};

					std::ranges::for_each(
						std::ranges::subrange{window.frames.get(), window.frames.get() + window.frame_total_count},
						[&](auto& frame)
						{
							image_view_create_info.image = frame.back_buffer;
							check_error(vkCreateImageView(device, &image_view_create_info, allocation_callbacks, &frame.back_buffer_view));
						}
					);
				}

				// Create Frame Buffer
				if (not window.use_dynamic_rendering)
				{
					VkFramebufferCreateInfo frame_buffer_create_info{
							.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
							.pNext = nullptr,
							.flags = 0,
							.renderPass = window.render_pass,
							.attachmentCount = 1,
							.pAttachments = VK_NULL_HANDLE,
							.width = static_cast<std::uint32_t>(window.width),
							.height = static_cast<std::uint32_t>(window.height),
							.layers = 1
					};

					std::ranges::for_each(
						std::ranges::subrange{window.frames.get(), window.frames.get() + window.frame_total_count},
						[&](auto& frame)
						{
							frame_buffer_create_info.pAttachments = &frame.back_buffer_view;
							check_error(vkCreateFramebuffer(device, &frame_buffer_create_info, allocation_callbacks, &frame.frame_buffer));
						}
					);
				}
			}

			// command buffer
			{
				std::ranges::for_each(
					std::ranges::subrange{window.frames.get(), window.frames.get() + window.frame_total_count},
					[&](auto& frame)
					{
						{
							const VkCommandPoolCreateInfo command_pool_create_info{
									.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
									.pNext = nullptr,
									.flags = 0,
									.queueFamilyIndex = queue_family
							};
							check_error(vkCreateCommandPool(device, &command_pool_create_info, allocation_callbacks, &frame.command_pool));
						}
						{
							const VkCommandBufferAllocateInfo command_buffer_allocate_info{
									.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
									.pNext = nullptr,
									.commandPool = frame.command_pool,
									.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
									.commandBufferCount = 1
							};
							check_error(
								vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &frame.command_buffer)
							);
						}
						{
							constexpr VkFenceCreateInfo fence_create_info{
									.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, .pNext = nullptr, .flags = VK_FENCE_CREATE_SIGNALED_BIT
							};
							check_error(vkCreateFence(device, &fence_create_info, allocation_callbacks, &frame.fence));
						}
					}
				);

				std::ranges::for_each(
					std::ranges::subrange{window.frame_semaphores.get(), window.frame_semaphores.get() + window.frame_semaphore_total_count},
					[&](auto& frame_semaphore)
					{
						constexpr VkSemaphoreCreateInfo semaphore_create_info{
								.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, .pNext = nullptr, .flags = 0
						};
						check_error(vkCreateSemaphore(
							device,
							&semaphore_create_info,
							allocation_callbacks,
							&frame_semaphore.image_acquired_semaphore
						));
						check_error(vkCreateSemaphore(
							device,
							&semaphore_create_info,
							allocation_callbacks,
							&frame_semaphore.render_complete_semaphore
						));
					}
				);
			}
		}

		auto create_device_objects() -> bool
		{
			auto& data = g_data;
			auto& init_info = g_data.init_info;

			if (data.descriptor_set_layout == VK_NULL_HANDLE)
			{
				VkDescriptorSetLayoutBinding binding{
						.binding = 0,
						.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
						.descriptorCount = 1,
						.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
						.pImmutableSamplers = nullptr
				};
				VkDescriptorSetLayoutCreateInfo info{
						.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.bindingCount = 1,
						.pBindings = &binding
				};
				const auto result = vkCreateDescriptorSetLayout(init_info.device, &info, init_info.allocation_callbacks, &data.descriptor_set_layout);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			if (data.pipeline_layout == VK_NULL_HANDLE)
			{
				VkPushConstantRange range{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = sizeof(float) * 0, .size = sizeof(float) * 4};
				VkPipelineLayoutCreateInfo info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.setLayoutCount = 1,
						.pSetLayouts = &data.descriptor_set_layout,
						.pushConstantRangeCount = 1,
						.pPushConstantRanges = &range
				};
				const auto result = vkCreatePipelineLayout(init_info.device, &info, init_info.allocation_callbacks, &data.pipeline_layout);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			if (data.shader_module_vertex == VK_NULL_HANDLE)
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

				const VkShaderModuleCreateInfo info{
						.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.codeSize = sizeof(shader_data),
						.pCode = shader_data
				};
				const auto result = vkCreateShaderModule(init_info.device, &info, init_info.allocation_callbacks, &data.shader_module_vertex);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			if (data.shader_module_fragment == VK_NULL_HANDLE)
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

				const VkShaderModuleCreateInfo info{
						.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.codeSize = sizeof(shader_data),
						.pCode = shader_data
				};
				const auto result = vkCreateShaderModule(init_info.device, &info, init_info.allocation_callbacks, &data.shader_module_fragment);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			if (data.pipeline == VK_NULL_HANDLE)
			{
				const VkPipelineShaderStageCreateInfo stage_vertex{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.stage = VK_SHADER_STAGE_VERTEX_BIT,
						.module = data.shader_module_vertex,
						.pName = "main",
						.pSpecializationInfo = nullptr
				};
				const VkPipelineShaderStageCreateInfo stage_fragment{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
						.module = data.shader_module_fragment,
						.pName = "main",
						.pSpecializationInfo = nullptr
				};
				const VkPipelineShaderStageCreateInfo stages[2]{stage_vertex, stage_fragment};

				constexpr VkVertexInputBindingDescription vertex_input_binding_description{
						.binding = 0, .stride = sizeof(vertex_type), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
				};
				constexpr VkVertexInputAttributeDescription vertex_input_attribute_description{
						.location = 0, .binding = vertex_input_binding_description.binding, .format = VK_FORMAT_R32G32_SFLOAT,
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
						vertex_input_attribute_description, vertex_input_attribute_description_uv, vertex_input_attribute_description_color
				};
				const VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.vertexBindingDescriptionCount = 1,
						.pVertexBindingDescriptions = &vertex_input_binding_description,
						.vertexAttributeDescriptionCount = 3,
						.pVertexAttributeDescriptions = vertex_input_attribute_descriptions
				};

				constexpr VkPipelineInputAssemblyStateCreateInfo assembly_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
						.primitiveRestartEnable = VK_FALSE
				};

				constexpr VkPipelineViewportStateCreateInfo viewport_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.viewportCount = 1,
						.pViewports = nullptr,
						.scissorCount = 1,
						.pScissors = nullptr
				};

				constexpr VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{
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

				const VkPipelineMultisampleStateCreateInfo multi_sample_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.rasterizationSamples = init_info.msaa_samples,
						.sampleShadingEnable = false,
						.minSampleShading = .0f,
						.pSampleMask = nullptr,
						.alphaToCoverageEnable = VK_FALSE,
						.alphaToOneEnable = VK_FALSE
				};

				constexpr VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{
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

				constexpr VkPipelineColorBlendAttachmentState color_blend_attachment_state{
						.blendEnable = VK_TRUE,
						.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
						.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
						.colorBlendOp = VK_BLEND_OP_ADD,
						.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
						.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
						.alphaBlendOp = VK_BLEND_OP_ADD,
						.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
				};
				const VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.logicOpEnable = VK_FALSE,
						.logicOp = VK_LOGIC_OP_CLEAR,
						.attachmentCount = 1,
						.pAttachments = &color_blend_attachment_state,
						.blendConstants = {}
				};

				constexpr VkDynamicState dynamic_state[2]{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
				const VkPipelineDynamicStateCreateInfo dynamic_state_create_info{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
						.pNext = nullptr,
						.flags = 0,
						.dynamicStateCount = 2,
						.pDynamicStates = dynamic_state
				};

				VkGraphicsPipelineCreateInfo pipeline_create_info{
						.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
						.pNext = nullptr,
						.flags = data.pipeline_create_flags,
						.stageCount = 2,
						.pStages = stages,
						.pVertexInputState = &vertex_input_state_create_info,
						.pInputAssemblyState = &assembly_state_create_info,
						.pTessellationState = nullptr,
						.pViewportState = &viewport_state_create_info,
						.pRasterizationState = &rasterization_state_create_info,
						.pMultisampleState = &multi_sample_state_create_info,
						.pDepthStencilState = &depth_stencil_state_create_info,
						.pColorBlendState = &color_blend_state_create_info,
						.pDynamicState = &dynamic_state_create_info,
						.layout = data.pipeline_layout,
						.renderPass = init_info.render_pass,
						.subpass = init_info.sub_pass,
						.basePipelineHandle = VK_NULL_HANDLE,
						.basePipelineIndex = 0
				};
				if (init_info.use_dynamic_rendering)
				{
					assert(
						init_info.pipeline_rendering_create_info.sType == VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR &&
						"pipeline_rendering_create_info sType must be VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR"
					);
					assert(
						init_info.pipeline_rendering_create_info.pNext == nullptr &&
						"pipeline_rendering_create_info.pNext must be NULL"
					);

					pipeline_create_info.pNext = &init_info.pipeline_rendering_create_info;
					pipeline_create_info.renderPass = VK_NULL_HANDLE;
				}

				const auto result = vkCreateGraphicsPipelines(
					init_info.device,
					init_info.pipeline_cache,
					1,
					&pipeline_create_info,
					init_info.allocation_callbacks,
					&data.pipeline
				);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			if (data.font_sampler == VK_NULL_HANDLE)
			{
				VkSamplerCreateInfo info{
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
				const auto result = vkCreateSampler(init_info.device, &info, init_info.allocation_callbacks, &data.font_sampler);
				if (init_info.check_result_callback)
				{
					init_info.check_result_callback(result);
				}
			}

			return true;
		}

		auto destroy_font_texture() -> void
		{
			auto& data = g_data;
			auto& init_info = g_data.init_info;

			if (data.font_descriptor_set != VK_NULL_HANDLE)
			{
				vkFreeDescriptorSets(init_info.device, init_info.descriptor_pool, 1, &data.font_descriptor_set);
				data.font_descriptor_set = VK_NULL_HANDLE;
			}
			if (data.font_view != VK_NULL_HANDLE)
			{
				vkDestroyImageView(init_info.device, data.font_view, init_info.allocation_callbacks);
				data.font_view = VK_NULL_HANDLE;
			}
			if (data.font_image != VK_NULL_HANDLE)
			{
				vkDestroyImage(init_info.device, data.font_image, init_info.allocation_callbacks);
				data.font_image = VK_NULL_HANDLE;
			}
			if (data.font_memory != VK_NULL_HANDLE)
			{
				vkFreeMemory(init_info.device, data.font_memory, init_info.allocation_callbacks);
				data.font_memory = VK_NULL_HANDLE;
			}
		}

		auto destroy_device_objects() -> void
		{
			auto& data = g_data;
			auto& init_info = g_data.init_info;

			// ==============
			// render buffer
			std::ranges::for_each(
				data.window_render_buffer,
				[&init_info](const data_type::frame_render_buffer_type& frame) -> void
				{
					if (frame.vertex_buffer)
					{
						vkDestroyBuffer(init_info.device, frame.vertex_buffer, init_info.allocation_callbacks);
					}
					if (frame.vertex_buffer_memory)
					{
						vkFreeMemory(init_info.device, frame.vertex_buffer_memory, init_info.allocation_callbacks);
					}

					if (frame.index_buffer)
					{
						vkDestroyBuffer(init_info.device, frame.index_buffer, init_info.allocation_callbacks);
					}
					if (frame.index_buffer_memory)
					{
						vkFreeMemory(init_info.device, frame.index_buffer_memory, init_info.allocation_callbacks);
					}
				}
			);
			data.window_render_buffer.clear();

			// ==============
			// font
			destroy_font_texture();

			if (data.font_sampler != VK_NULL_HANDLE)
			{
				vkDestroySampler(init_info.device, data.font_sampler, init_info.allocation_callbacks);
				data.font_sampler = VK_NULL_HANDLE;
			}
			if (data.font_command_buffer != VK_NULL_HANDLE)
			{
				vkFreeCommandBuffers(init_info.device, data.font_command_pool, 1, &data.font_command_buffer);
				data.font_command_buffer = VK_NULL_HANDLE;
			}
			if (data.font_command_pool != VK_NULL_HANDLE)
			{
				vkDestroyCommandPool(init_info.device, data.font_command_pool, init_info.allocation_callbacks);
				data.font_command_pool = VK_NULL_HANDLE;
			}

			// ==============
			// pipeline
			if (data.shader_module_vertex != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(init_info.device, data.shader_module_vertex, init_info.allocation_callbacks);
				data.shader_module_vertex = VK_NULL_HANDLE;
			}
			if (data.shader_module_fragment != VK_NULL_HANDLE)
			{
				vkDestroyShaderModule(init_info.device, data.shader_module_fragment, init_info.allocation_callbacks);
				data.shader_module_fragment = VK_NULL_HANDLE;
			}
			if (data.pipeline_layout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(init_info.device, data.pipeline_layout, init_info.allocation_callbacks);
				data.pipeline_layout = VK_NULL_HANDLE;
			}
			if (data.descriptor_set_layout != VK_NULL_HANDLE)
			{
				vkDestroyDescriptorSetLayout(init_info.device, data.descriptor_set_layout, init_info.allocation_callbacks);
				data.descriptor_set_layout = VK_NULL_HANDLE;
			}
			if (data.pipeline)
			{
				vkDestroyPipeline(init_info.device, data.pipeline, init_info.allocation_callbacks);
				data.pipeline = VK_NULL_HANDLE;
			}
		}

		[[nodiscard]] auto init(const init_info_type& info) -> bool
		{
			g_data.init_info = info;

			return create_device_objects();
		}

		auto shutdown() -> void //
		{
			auto& window = g_window;
			auto& data = g_data;
			auto& init_info = g_data.init_info;

			destroy_device_objects();

			vkDeviceWaitIdle(init_info.device);

			window.destroy_frames(init_info.device, init_info.allocation_callbacks);
			window.destroy_frame_semaphores(init_info.device, init_info.allocation_callbacks);

			vkDestroyPipeline(init_info.device, window.pipeline, init_info.allocation_callbacks);
			vkDestroyRenderPass(init_info.device, window.render_pass, init_info.allocation_callbacks);
			vkDestroySwapchainKHR(init_info.device, window.swap_chain, init_info.allocation_callbacks);
			vkDestroySurfaceKHR(init_info.instance, window.surface, init_info.allocation_callbacks);
		}

		auto new_frame() -> void
		{
			if (g_data.font_descriptor_set == VK_NULL_HANDLE)
			{
				auto& window = g_window;
				auto& data = g_data;
				auto& init_info = data.init_info;

				// Destroy existing texture (if any)
				if (data.font_memory or data.font_image or data.font_view or data.font_descriptor_set)
				{
					vkQueueWaitIdle(init_info.queue);
					destroy_font_texture();
				}

				// Create command pool/buffer
				if (data.font_command_pool == VK_NULL_HANDLE)
				{
					const VkCommandPoolCreateInfo command_pool_create_info{
							.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .pNext = nullptr, .flags = 0,
							.queueFamilyIndex = init_info.queue_family
					};
					check_error(vkCreateCommandPool(
							init_info.device,
							&command_pool_create_info,
							init_info.allocation_callbacks,
							&data.font_command_pool
						)
					);
				}
				if (data.font_command_buffer == VK_NULL_HANDLE)
				{
					const VkCommandBufferAllocateInfo command_buffer_allocate_info{
							.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
							.pNext = nullptr,
							.commandPool = data.font_command_pool,
							.commandBufferCount = 1
					};
					check_error(vkAllocateCommandBuffers(init_info.device, &command_buffer_allocate_info, &data.font_command_buffer));
				}

				// Start command buffer
				{
					check_error(vkResetCommandPool(init_info.device, data.font_command_pool, 0));

					constexpr VkCommandBufferBeginInfo command_buffer_begin_info{
							.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
							.pNext = nullptr,
							.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
							.pInheritanceInfo = nullptr
					};
					check_error(vkBeginCommandBuffer(data.font_command_buffer, &command_buffer_begin_info));
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
					check_error(vkCreateImage(init_info.device, &image_create_info, init_info.allocation_callbacks, &data.font_image));

					VkMemoryRequirements memory_requirements;
					vkGetImageMemoryRequirements(init_info.device, data.font_image, &memory_requirements);

					const VkMemoryAllocateInfo memory_allocate_info{
							.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
							.pNext = nullptr,
							.allocationSize = memory_requirements.size,
							.memoryTypeIndex = memory_type(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_requirements.memoryTypeBits)
					};
					check_error(vkAllocateMemory(init_info.device, &memory_allocate_info, init_info.allocation_callbacks, &data.font_memory));
					check_error(vkBindImageMemory(init_info.device, data.font_image, data.font_memory, 0));
				}

				// Create Image View
				{
					const VkImageViewCreateInfo image_view_create_info{
							.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
							.pNext = nullptr,
							.flags = 0,
							.image = data.font_image,
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
					check_error(vkCreateImageView(init_info.device, &image_view_create_info, init_info.allocation_callbacks, &data.font_view));
				}

				// Create Descriptor Set
				{
					const VkDescriptorSetAllocateInfo descriptor_set_allocate_info{
							.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
							.pNext = nullptr,
							.descriptorPool = init_info.descriptor_pool,
							.descriptorSetCount = 1,
							.pSetLayouts = &data.descriptor_set_layout
					};
					check_error(vkAllocateDescriptorSets(init_info.device, &descriptor_set_allocate_info, &data.font_descriptor_set));

					const VkDescriptorImageInfo descriptor_image_info{
							.sampler = data.font_sampler, .imageView = data.font_view, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					};
					const VkWriteDescriptorSet write_descriptor_set{
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
							.pNext = nullptr,
							.dstSet = data.font_descriptor_set,
							.dstBinding = 0,
							.dstArrayElement = 0,
							.descriptorCount = 1,
							.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
							.pImageInfo = &descriptor_image_info,
							.pBufferInfo = nullptr,
							.pTexelBufferView = nullptr
					};
					vkUpdateDescriptorSets(init_info.device, 1, &write_descriptor_set, 0, nullptr);
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
					check_error(vkCreateBuffer(init_info.device, &buffer_create_info, init_info.allocation_callbacks, &upload_buffer));

					VkMemoryRequirements memory_requirements;
					vkGetBufferMemoryRequirements(init_info.device, upload_buffer, &memory_requirements);
					data.memory_buffer_alignment = std::ranges::max(data.memory_buffer_alignment, memory_requirements.alignment);

					const VkMemoryAllocateInfo memory_allocate_info{
							.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
							.pNext = nullptr,
							.allocationSize = memory_requirements.size,
							.memoryTypeIndex = memory_type(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, memory_requirements.memoryTypeBits)
					};

					VkDeviceMemory upload_buffer_memory;
					check_error(vkAllocateMemory(init_info.device, &memory_allocate_info, init_info.allocation_callbacks, &upload_buffer_memory));
					check_error(vkBindBufferMemory(init_info.device, upload_buffer, upload_buffer_memory, 0));

					// Upload to Buffer
					unsigned char* mapped_memory;
					check_error(vkMapMemory(init_info.device, upload_buffer_memory, 0, upload_size, 0, reinterpret_cast<void**>(&mapped_memory)));
					std::memcpy(mapped_memory, pixels, upload_size);
					const VkMappedMemoryRange mapped_memory_range{
							.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
							.pNext = nullptr,
							.memory = upload_buffer_memory,
							.offset = 0,
							.size = upload_size
					};
					check_error(vkFlushMappedMemoryRanges(init_info.device, 1, &mapped_memory_range));
					vkUnmapMemory(init_info.device, upload_buffer_memory);

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
							.image = data.font_image,
							.subresourceRange =
							{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
							}
					};
					vkCmdPipelineBarrier(
						data.font_command_buffer,
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
						data.font_command_buffer,
						upload_buffer,
						data.font_image,
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
							.image = data.font_image,
							.subresourceRange =
							{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .baseMipLevel = 0, .levelCount = 1, .baseArrayLayer = 0, .layerCount = 1
							}
					};
					vkCmdPipelineBarrier(
						data.font_command_buffer,
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
							.pCommandBuffers = &data.font_command_buffer,
							.signalSemaphoreCount = 0,
							.pSignalSemaphores = nullptr
					};
					check_error(vkEndCommandBuffer(data.font_command_buffer));
					check_error(vkQueueSubmit(init_info.queue, 1, &submit_info, VK_NULL_HANDLE));
					check_error(vkQueueWaitIdle(init_info.queue));

					vkDestroyBuffer(init_info.device, upload_buffer, init_info.allocation_callbacks);
					vkFreeMemory(init_info.device, upload_buffer_memory, init_info.allocation_callbacks);
				}
			}
		}
	} // namespace my_vulkan

	VkInstance g_instance;

	VkPhysicalDevice g_physical_device;
	VkDevice g_device;

	std::uint32_t g_queue_family;
	VkQueue g_queue;

	VkDescriptorPool g_descriptor_pool;

	VkPipelineCache g_pipeline_cache;

	VkAllocationCallbacks* g_allocation_callbacks;

	VkDebugReportCallbackEXT g_debug_report_callback;

	bool g_swap_chain_rebuild_required;

	VKAPI_ATTR auto VKAPI_CALL vk_debug_report(
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
			"Vulkan debug report: \n\t flags({}) \n\t object_type({}) \n\t object({}) \n\t location({}) \n\t message_code({}) \n\t layer_prefix({}) \n\t message({})",
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

	auto setup_vulkan(std::vector<const char*>& extensions) -> void
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
			my_vulkan::check_error(vkEnumerateInstanceExtensionProperties(nullptr, &properties_count, properties.data()));

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
			my_vulkan::check_error(vkCreateInstance(&instance_create_info, g_allocation_callbacks, &g_instance));

			// setup the debug report callback
			const auto create_debug_report_callback =
					reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(g_instance, "vkCreateDebugReportCallbackEXT"));
			assert(create_debug_report_callback);
			VkDebugReportCallbackCreateInfoEXT debug_report_callback_create_info{
					.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
					.pNext = nullptr,
					.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
					.pfnCallback = vk_debug_report,
					.pUserData = nullptr
			};
			my_vulkan::check_error(
				create_debug_report_callback(g_instance, &debug_report_callback_create_info, g_allocation_callbacks, &g_debug_report_callback)
			);
		}

		// Select Physical Device (GPU)
		{
			std::uint32_t gpu_count;
			my_vulkan::check_error(vkEnumeratePhysicalDevices(g_instance, &gpu_count, nullptr));
			assert(gpu_count != 0);
			std::vector<VkPhysicalDevice> gpus;
			gpus.resize(gpu_count);
			my_vulkan::check_error(vkEnumeratePhysicalDevices(g_instance, &gpu_count, gpus.data()));

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

			constexpr float queue_priority[]{1.f,};
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
			my_vulkan::check_error(vkCreateDevice(g_physical_device, &device_create_info, g_allocation_callbacks, &g_device));
			vkGetDeviceQueue(g_device, g_queue_family, 0, &g_queue);
		}

		// Create Descriptor Pool
		{
			// a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)

			constexpr VkDescriptorPoolSize descriptor_pool_size[]
			{
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

			my_vulkan::check_error(vkCreateDescriptorPool(g_device, &descriptor_pool_create_info, g_allocation_callbacks, &g_descriptor_pool));
		}
	}

	auto frame_render() -> void
	{
		// todo
	}

	auto frame_present() -> void
	{
		if (g_swap_chain_rebuild_required)
		{
			return;
		}

		auto& window = my_vulkan::g_window;
		auto& data = my_vulkan::g_data;

		const auto render_complete_semaphore = window.frame_semaphores[window.frame_semaphore_current_index].render_complete_semaphore;
		const VkPresentInfoKHR present_info{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.pNext = nullptr,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &render_complete_semaphore,
				.swapchainCount = 1,
				.pSwapchains = &window.swap_chain,
				.pImageIndices = &window.frame_current_index,
				.pResults = nullptr
		};
		if (
			const auto result = vkQueuePresentKHR(g_queue, &present_info);
			result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR
		)
		{
			g_swap_chain_rebuild_required = true;
			return;
		}
		else
		{
			my_vulkan::check_error(result);
		}

		window.frame_semaphore_current_index = (window.frame_semaphore_current_index + 1) % window.frame_semaphore_total_count;
	}
} // namespace

int main()
{
	glfwSetErrorCallback(my_glfw::error_callback);
	if (not glfwInit())
	{
		std::println(std::cerr, "GLFW: glfwInit failed");
		return -1;
	}

	// Create window with Vulkan context
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	auto* window = glfwCreateWindow(1280, 720, "Vulkan+GLFW GUI Playground", nullptr, nullptr);
	if (not glfwVulkanSupported())
	{
		std::println(std::cerr, "GLFW: Vulkan Not Supported");
		return -1;
	}

	std::uint32_t extensions_count;
	const char** extensions_raw = glfwGetRequiredInstanceExtensions(&extensions_count);
	std::vector<const char*> extensions{extensions_raw, extensions_raw + extensions_count};
	setup_vulkan(extensions);

	// Create Window Surface
	VkSurfaceKHR surface;
	my_vulkan::check_error(glfwCreateWindowSurface(g_instance, window, g_allocation_callbacks, &surface));

	// todo
	constexpr std::uint32_t min_image_count = 2;

	// Create frame buffer
	{
		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);
		my_vulkan::setup_vulkan_window(
			g_instance,
			g_physical_device,
			g_device,
			g_queue_family,
			g_allocation_callbacks,
			surface,
			width,
			height,
			min_image_count
		);
	}

	// Setup Platform/Renderer backends
	if (not my_glfw::init(window))
	{
		std::println(std::cerr, "my_glfw::init: failed");
		return -1;
	}
	{
		const my_vulkan::init_info_type init_info{
				.instance = g_instance,
				.physical_device = g_physical_device,
				.device = g_device,
				.queue_family = g_queue_family,
				.queue = g_queue,
				.descriptor_pool = g_descriptor_pool,
				.render_pass = my_vulkan::g_window.render_pass,
				.msaa_samples = VK_SAMPLE_COUNT_1_BIT,
				.pipeline_cache = g_pipeline_cache,
				.sub_pass = 0,
				.use_dynamic_rendering = false,
				.pipeline_rendering_create_info = {},
				.allocation_callbacks = g_allocation_callbacks,
				.check_result_callback = my_vulkan::check_error,
		};
		if (not my_vulkan::init(init_info))
		{
			std::println(std::cerr, "my_vulkan::init: failed");
			return -1;
		}
	}

	// Main loop
	while (not glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		if (g_swap_chain_rebuild_required)
		{
			int width;
			int height;
			glfwGetFramebufferSize(window, &width, &height);
			if (width > 0 and height > 0)
			{
				my_vulkan::create_or_resize_window(
					g_instance,
					g_physical_device,
					g_device,
					g_queue_family,
					g_allocation_callbacks,
					width,
					height,
					min_image_count
				);
				my_vulkan::g_window.frame_current_index = 0;
				g_swap_chain_rebuild_required = false;
			}
		}

		my_vulkan::new_frame();
		my_glfw::new_frame();

		// todo
		constexpr float clear_color[]{.45f, .55f, .65f, 1.f};

		my_vulkan::g_window.clear_value.color.float32[0] = clear_color[0] * clear_color[3];
		my_vulkan::g_window.clear_value.color.float32[1] = clear_color[1] * clear_color[3];
		my_vulkan::g_window.clear_value.color.float32[2] = clear_color[2] * clear_color[3];
		my_vulkan::g_window.clear_value.color.float32[3] = clear_color[3];

		frame_render();
		frame_present();
	}

	// cleanup
	my_vulkan::check_error(vkDeviceWaitIdle(g_device));
	my_vulkan::shutdown();
	my_glfw::shutdown();

	vkDestroyDescriptorPool(g_device, g_descriptor_pool, g_allocation_callbacks);
	const auto destroy_debug_report_callback =
			reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(vkGetInstanceProcAddr(g_instance, "vkDestroyDebugReportCallbackEXT"));
	destroy_debug_report_callback(g_instance, g_debug_report_callback, g_allocation_callbacks);
	vkDestroyDevice(g_device, g_allocation_callbacks);
	vkDestroyInstance(g_instance, g_allocation_callbacks);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
