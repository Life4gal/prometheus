#if GAL_PROMETHEUS_USE_MODULE
import gal.prometheus;
#else
#include <prometheus.ixx>
#endif

#include <vector>
#include <cassert>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

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

		[[nodiscard]] auto init(GLFWwindow* window) -> bool
		{
			g_data.window = window;
			return true;
		}

		[[nodiscard]] auto shutdown() -> bool //
		{
			return true;
		}

		auto new_frame() -> void //
		{
			glfwGetWindowSize(g_data.window, &g_data.width, &g_data.height);
		}
	} // namespace my_glfw

	namespace my_vulkan
	{
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

			// >= 2
			std::uint32_t min_image_count{0};
			// >= min_image_count
			std::uint32_t image_count{0};

			// 0 defaults to VK_SAMPLE_COUNT_1_BIT
			VkSampleCountFlagBits msaa_samples{static_cast<VkSampleCountFlagBits>(0)};

			VkPipelineCache pipeline_cache{VK_NULL_HANDLE};
			std::uint32_t sub_pass{0};

			bool use_dynamic_rendering{true};
			VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info{};

			const VkAllocationCallbacks* allocation_callbacks{nullptr};
			void (*check_result_callback)(VkResult){nullptr};
			VkDeviceSize min_allocation_size{0};
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

		data_type g_data;

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
						.codeSize = std::ranges::size(shader_data),
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
						.codeSize = std::ranges::size(shader_data),
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
						.rasterizationSamples = init_info.msaa_samples == 0 ? VK_SAMPLE_COUNT_1_BIT : init_info.msaa_samples,
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
	} // namespace my_vulkan
} // namespace

int main()
{
	std::cout << "hello world\n";
}
