#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <optional>
#include "DataStructure.hpp"
#include "GraphicsPipelineBuilder.hpp"

namespace fdf
{
	class Renderer
	{
	public:
		void init();
		void loop();
		void importVertex(
			const std::vector<fdf::Vertex>& vertices,
			size_t row, size_t col
		);

		static const uint32_t kWinWidth, kWinHeight;
		static const std::string kAppName, kWindowName;

	private:
		GLFWwindow*							_window;
		vk::UniqueSurfaceKHR				_surface;
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
		vk::UniqueCommandBuffer				_cmdBuffer;
		vk::UniqueSemaphore					_renderSem;
		vk::UniqueSemaphore					_presentSem;
		vk::UniqueFence						_cmdFence;

		std::vector<fdf::Vertex>			_vertices;
		vk::UniqueBuffer					_vertexBuffer;
		vk::UniqueDeviceMemory				_vertexBufferMemory;
		std::vector<uint32_t>				_vertexIndices;
		vk::UniqueBuffer					_indexBuffer;
		vk::UniqueDeviceMemory				_indexBufferMemory;

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
		void createPipeline();
		void createSyncObjects();

		/* Drawing loop */
		void acquireNextImgIndex();
		void recordCmd();
		void submitCmd();
		void drawFrame();

		/* Import vertices data */
		void createVertexBuffer();
		void allocateVertexBuffer();
		void createIndexBuffer();
		void allocateIndexBuffer();

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
