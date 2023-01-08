#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <optional>
#include "DataStructure.hpp"
#include "GraphicsPipelineBuilder.hpp"

namespace fdf
{
	// NOTE: surfaceがUniqueでない。Uniqueにすると破棄時にExceptionが出る。原因不明
	class Renderer
	{
	public:
		Renderer() : _currentImgIndex(0) {
			_ubo.model = glm::mat4(1.0f);
			_ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			_ubo.proj = glm::perspective(glm::radians(30.0f), (float)kWinWidth / kWinHeight, 0.0f, 1.0f);
			_ubo.proj[1][1] *= -1;
		}

		~Renderer()
		{
			glfwDestroyWindow(_window);
			glfwTerminate();
			for (const auto& p : _uniformBuffersMemory) {
				_device->unmapMemory(p.get());
			}
		}

		void init();
		void loop();
		void importVertex(
			const std::vector<fdf::Vertex>& vertices,
			size_t row, size_t col
		);
		void setUniformBuffer(const fdf::UniformBufferObject& ubo);

		static const uint32_t kWinWidth, kWinHeight;
		static const std::string kAppName, kWindowName;

	private:
		GLFWwindow*							_window;
		vk::SurfaceKHR						_surface;
		vk::SurfaceCapabilitiesKHR			_surfaceCapabilities;
		vk::SurfaceFormatKHR				_surfaceFormat;
		vk::PresentModeKHR					_surfacePresentMode;
		vk::UniqueInstance					_instance;
		vk::PhysicalDevice					_physicalDevice;
		uint32_t							_queueFamilyIndex;
		vk::UniqueDevice					_device;
		vk::UniqueCommandPool				_cmdPool;
		vk::Queue							_queue;
		vk::UniqueSwapchainKHR				_swapchain;
		uint32_t							_currentImgIndex;
		std::vector<vk::Image>				_imgs;
		std::vector<vk::UniqueImageView>	_imgViews;
		vk::UniqueRenderPass				_renderPass;
		std::vector<vk::UniqueFramebuffer>	_framebuffers;
		vk::UniquePipeline					_pipeline;
		vk::UniquePipelineLayout			_pipelineLayout;
		std::vector<vk::UniqueCommandBuffer> _cmdBuffer;
		vk::UniqueSemaphore					_renderSem;
		vk::UniqueSemaphore					_presentSem;
		std::vector<vk::UniqueFence>		_cmdFences;

		vk::UniqueDescriptorSetLayout		_descSetLayout;
		vk::UniqueDescriptorPool			_descPool;
		std::vector<vk::UniqueDescriptorSet> _descSets;

		std::vector<fdf::Vertex>			_vertices;
		vk::UniqueBuffer					_vertexBuffer;
		vk::UniqueDeviceMemory				_vertexBufferMemory;
		std::vector<uint32_t>				_vertexIndices;
		vk::UniqueBuffer					_indexBuffer;
		vk::UniqueDeviceMemory				_indexBufferMemory;
		std::vector<vk::UniqueBuffer>		_uniformBuffers;
		std::vector<vk::UniqueDeviceMemory>	_uniformBuffersMemory;
		std::vector<void*>					_uniformBuffersMapped;

		static const std::vector<const char*> kRequiredLayers;
		static const std::vector<const char*> kRequiredExtensions;

		/* Initialize helpers */
		void createWindow();
		void createSurface();
		void createInstance();
		void pickPhysicalDevice();
		void createDevice();
		void createCmdPool();
		void getQueue();
		void createSwapchain();
		void createImages();
		void createImageViews();
		void createRenderPass();
		void createFramebuffers();
		void createCmdBuffer();
		void createUniformBuffer();
		void createDescriptorSetLayout();
		void createDescriptorPool();
		void allocateDescriptorSet();
		void updateDescriptorSet();
		void createPipeline();
		void createSyncObjects();

		/* Input callbacks */
		void kayCallback(GLFWwindow* win, int key, int scancode, int action, int mods);

		/* Uniform Buffers */
		fdf::UniformBufferObject _ubo;
		void updateUniformBuffer();

		/* Drawing loop */
		void acquireNextImgIndex();
		void recordCmd();
		void submitCmd();
		void drawFrame();

		/* Import vertices data */
		void createBuffer(
			vk::UniqueBuffer& dstBuf,
			vk::UniqueDeviceMemory& dstMem,
			vk::BufferUsageFlagBits usage,
			vk::MemoryPropertyFlagBits memProps,
			vk::DeviceSize size
		);
		void copyBuffer(
			vk::UniqueDeviceMemory& dst,
			const void* src,
			size_t size
		);

		/* General helpers */
		static std::optional<uint32_t> searchQueueFamily(
			const vk::PhysicalDevice& physicalDevice,
			vk::QueueFlagBits queueFamily
		);
		static std::optional<uint32_t> getMemoryTypeIndex(
			const vk::PhysicalDevice& physicalDevice,
			uint32_t requestBits,
			vk::MemoryPropertyFlagBits requestProps
		);
		static std::vector<uint32_t> createVertexIndicesStrip(
			size_t row, size_t col
		);
	};
}
