#include "Renderer.hpp"
#include <chrono>

const uint32_t fdf::Renderer::kWinWidth = 640;
const uint32_t fdf::Renderer::kWinHeight = 480;
const std::string fdf::Renderer::kAppName = "fdf";
const std::string fdf::Renderer::kWindowName = "fdf";
const std::vector<const char*> fdf::Renderer::kRequiredLayers = {
	"VK_LAYER_KHRONOS_validation",
};
const std::vector<const char*> fdf::Renderer::kRequiredExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

fdf::UniformBufferObject fdf::Renderer::_ubo = {
	// glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::mat4(1.0f),
	glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
	glm::perspective(glm::radians(30.0f), (float)kWinWidth / kWinHeight, 0.0f, 1.0f)
};

void fdf::Renderer::loop() {
	_ubo.proj[1][1] *= -1;

	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();
		acquireNextImgIndex();
		updateUniformBuffer();
		recordCmd();
		submitCmd();
		drawFrame();
	}
	_queue.waitIdle();
}

void fdf::Renderer::init() {
	if (!glfwInit()) {
		throw std::runtime_error("failed to init window");
	}

	createWindow();
	createInstance();
	pickPhysicalDevice();
	createDevice();
	getQueue();
	createSurface();
	createSwapchain();
	createImages();
	createImageViews();
	createRenderPass();
	createFramebuffers();
	createCmdPool();
	createCmdBuffer();
	
	createUniformBuffer();
	createDescriptorSetLayout();
	createDescriptorPool();
	allocateDescriptorSet();
	updateDescriptorSet();
	
	createPipeline();
	createSyncObjects();

	glfwSetKeyCallback(_window, kayCallback);
	glfwSetScrollCallback(_window, scrollCallback);
}

void fdf::Renderer::createWindow() {
	// GLFWは、デフォルトではOpenGLを期待するので、それをOFF
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_window = glfwCreateWindow(
		kWinWidth,
		kWinHeight,
		kWindowName.c_str(),
		nullptr,
		nullptr
	);

	if (!_window) {
		throw std::runtime_error("failed to create window");
	}
}

void fdf::Renderer::createInstance() {
	uint32_t requiredExtensionsCount = 0;
	const char** requiredExtensions =
		glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

	vk::ApplicationInfo appInfo;
	appInfo.pApplicationName = kAppName.c_str();

	vk::InstanceCreateInfo createInfo;
	createInfo.enabledLayerCount = kRequiredLayers.size();
	createInfo.ppEnabledLayerNames = kRequiredLayers.data();
	createInfo.enabledExtensionCount = requiredExtensionsCount;
	createInfo.ppEnabledExtensionNames = requiredExtensions;
	createInfo.pApplicationInfo = &appInfo;

	_instance = vk::createInstanceUnique(createInfo);
}

void fdf::Renderer::pickPhysicalDevice() {
	std::vector<vk::PhysicalDevice> physDevs =
		_instance->enumeratePhysicalDevices();

	for (const auto& dev : physDevs) {
		std::optional<uint32_t> graphicsFamily =
			searchQueueFamily(dev, vk::QueueFlagBits::eGraphics);

		// TODO: swapchain, surfaceサポートの確認
		if (graphicsFamily.has_value()) {
			_queueFamilyIndex = graphicsFamily.value();
			_physicalDevice = dev;
			return;
		}
	}

	throw std::runtime_error("failed to pick physical device");
}

void fdf::Renderer::createDevice() {
	vk::DeviceQueueCreateInfo qCreateInfo;
	const float priorities[1] = { 1.0f };
	qCreateInfo.queueFamilyIndex = _queueFamilyIndex;
	qCreateInfo.queueCount = 1;
	qCreateInfo.pQueuePriorities = priorities;

	vk::DeviceCreateInfo createInfo;
	createInfo.enabledExtensionCount = kRequiredExtensions.size();
	createInfo.ppEnabledExtensionNames = kRequiredExtensions.data();
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &qCreateInfo;

	_device = _physicalDevice.createDeviceUnique(createInfo);
}

void fdf::Renderer::createCmdPool() {
	vk::CommandPoolCreateInfo createInfo;
	createInfo.queueFamilyIndex = _queueFamilyIndex;
	createInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	
	_cmdPool = _device->createCommandPoolUnique(createInfo);
}

void fdf::Renderer::getQueue() {
	_queue = _device->getQueue(_queueFamilyIndex, 0);
}

void fdf::Renderer::createSurface() {
	// TODO: error handle
	VkSurfaceKHR surface;
	glfwCreateWindowSurface(
		_instance.get(),
		_window,
		nullptr,
		&surface
	);

	//_surface = vk::UniqueSurfaceKHR{ surface, _instance.get() };
	_surface = vk::SurfaceKHR{ surface };

	_surfaceCapabilities = _physicalDevice.getSurfaceCapabilitiesKHR(_surface);
	_surfaceFormat = _physicalDevice.getSurfaceFormatsKHR(_surface)[0];
	_surfacePresentMode = _physicalDevice.getSurfacePresentModesKHR(_surface)[0];
}

void fdf::Renderer::createSwapchain() {
	vk::SwapchainCreateInfoKHR createInfo;
	createInfo.surface			= _surface;
	createInfo.minImageCount	= _surfaceCapabilities.minImageCount + 1;
	createInfo.imageFormat		= _surfaceFormat.format;
	createInfo.imageColorSpace	= _surfaceFormat.colorSpace;
	createInfo.imageExtent		= _surfaceCapabilities.currentExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage		= vk::ImageUsageFlagBits::eColorAttachment;
	createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	createInfo.preTransform		= _surfaceCapabilities.currentTransform;
	createInfo.presentMode		= _surfacePresentMode;
	createInfo.clipped			= VK_TRUE;

	_swapchain = _device->createSwapchainKHRUnique(createInfo);
}

void fdf::Renderer::createImages() {
	_imgs = _device->getSwapchainImagesKHR(_swapchain.get());
}

void fdf::Renderer::createImageViews() {
	_imgViews.resize(_imgs.size());
	
	for (size_t i = 0; i < _imgs.size(); ++i) {
		vk::ImageViewCreateInfo createInfo;
		createInfo.image = _imgs[i];
		createInfo.viewType = vk::ImageViewType::e2D;
		createInfo.format = _surfaceFormat.format;
		createInfo.components.r = vk::ComponentSwizzle::eIdentity;
		createInfo.components.g = vk::ComponentSwizzle::eIdentity;
		createInfo.components.b = vk::ComponentSwizzle::eIdentity;
		createInfo.components.a = vk::ComponentSwizzle::eIdentity;
		createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		_imgViews[i] = _device->createImageViewUnique(createInfo);
	}
}

void fdf::Renderer::createRenderPass() {
	vk::AttachmentDescription attachmentDesc;
	attachmentDesc.format = _surfaceFormat.format;
	attachmentDesc.samples = vk::SampleCountFlagBits::e1;
	attachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
	attachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
	attachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
	attachmentDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference attachmentRef;
	attachmentRef.attachment = 0;
	attachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpassDesc;
	subpassDesc.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &attachmentRef;

	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = 1;
	createInfo.pAttachments = &attachmentDesc;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpassDesc;
	createInfo.dependencyCount = 0;
	createInfo.pDependencies = nullptr;

	_renderPass = _device->createRenderPassUnique(createInfo);
}

void fdf::Renderer::createFramebuffers() {
	_framebuffers.resize(_imgViews.size());

	for (size_t i = 0; i < _imgViews.size(); ++i) {
		vk::FramebufferCreateInfo createInfo;
		createInfo.renderPass = _renderPass.get();
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &(_imgViews[i].get());
		createInfo.width = kWinWidth;
		createInfo.height = kWinHeight;
		createInfo.layers = 1;

		_framebuffers[i] = _device->createFramebufferUnique(createInfo);
	}
}

void fdf::Renderer::createCmdBuffer() {
	vk::CommandBufferAllocateInfo allocInfo;
	allocInfo.commandPool = _cmdPool.get();
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = _imgs.size();

	_cmdBuffer = _device->allocateCommandBuffersUnique(allocInfo);
}

void fdf::Renderer::createUniformBuffer() {
	const vk::DeviceSize size = sizeof(fdf::UniformBufferObject);

	_uniformBuffers.resize(_imgs.size());
	_uniformBuffersMemory.resize(_imgs.size());
	_uniformBuffersMapped.resize(_imgs.size());
	vk::MemoryPropertyFlags prop = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
	for (size_t i = 0; i < _imgs.size(); ++i) {
		createBuffer(_uniformBuffers[i], _uniformBuffersMemory[i],
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits(unsigned int(prop)),
			size
		);
		_uniformBuffersMapped[i] = _device->mapMemory(_uniformBuffersMemory[i].get(), 0, size);
	}
}

void fdf::Renderer::createDescriptorSetLayout() {
	vk::DescriptorSetLayoutBinding uboBinding;
	uboBinding.binding = 0;
	uboBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
	uboBinding.descriptorCount = 1;
	uboBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;

	vk::DescriptorSetLayoutCreateInfo createInfo;
	createInfo.bindingCount = 1;
	createInfo.pBindings = &uboBinding;

	_descSetLayout = _device->createDescriptorSetLayoutUnique(createInfo);
}

void fdf::Renderer::createDescriptorPool() {
	vk::DescriptorPoolSize poolSize;
	poolSize.type = vk::DescriptorType::eUniformBuffer;
	poolSize.descriptorCount = _imgs.size();

	vk::DescriptorPoolCreateInfo createInfo;
	createInfo.maxSets = _imgs.size();
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &poolSize;
	createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

	_descPool = _device->createDescriptorPoolUnique(createInfo);
}

void fdf::Renderer::allocateDescriptorSet() {
	std::vector<vk::DescriptorSetLayout> layouts;
	for (size_t i = 0; i < _imgs.size(); ++i)
	{
		layouts.push_back(_descSetLayout.get());
	}

	vk::DescriptorSetAllocateInfo allocInfo;
	allocInfo.descriptorPool = _descPool.get();
	allocInfo.descriptorSetCount = _imgs.size();
	allocInfo.pSetLayouts = layouts.data();

	_descSets = _device->allocateDescriptorSetsUnique(allocInfo);
}

void fdf::Renderer::updateDescriptorSet() {
	for (size_t i = 0; i < _imgs.size(); ++i) {
		vk::DescriptorBufferInfo bufInfo;
		bufInfo.buffer = _uniformBuffers[i].get();
		bufInfo.offset = 0;
		bufInfo.range = VK_WHOLE_SIZE;

		vk::WriteDescriptorSet writeDesc;
		writeDesc.dstSet = _descSets[i].get();
		writeDesc.dstBinding = 0;
		writeDesc.descriptorType = vk::DescriptorType::eUniformBuffer;
		writeDesc.descriptorCount = 1;
		writeDesc.pBufferInfo = &bufInfo;
		writeDesc.pImageInfo = nullptr;
		writeDesc.pTexelBufferView = nullptr;

		_device->updateDescriptorSets({ writeDesc }, nullptr);
	}
}

void fdf::Renderer::createPipeline() {
	fdf::GraphicsPipelineBuilder builder(_device.get());

	builder.addShaderStage(
		vk::ShaderStageFlagBits::eVertex,
		"shaders/vert.spv",
		"main"
	);
	builder.addShaderStage(
		vk::ShaderStageFlagBits::eFragment,
		"shaders/frag.spv",
		"main"
	);
	builder.setPrimitiveTopology(vk::PrimitiveTopology::eTriangleStrip);
	builder.setVertexBinding(sizeof(fdf::Vertex));
	builder.setVertexPosAttribute(vk::Format::eR32G32B32Sfloat, offsetof(fdf::Vertex, pos));
	builder.setVertexColorAttribute(vk::Format::eR32G32B32A32Sfloat, offsetof(fdf::Vertex, rgba));
	builder.setPolygonMode(vk::PolygonMode::eLine);
	builder.setWH(kWinWidth, kWinHeight);
	builder.setRenderPass(_renderPass.get());
	builder.setSubpass(0);
	
	builder.setDescriptorSetLayouts(&_descSetLayout.get(), 1);

	auto result = builder.build();
	_pipeline = std::move(result.pipeline);
	_pipelineLayout = std::move(result.layout);
}

void fdf::Renderer::createSyncObjects() {
	_cmdFences.resize(_imgs.size());
	vk::FenceCreateInfo fenceInfo;
	fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

	for (size_t i = 0; i < _imgs.size(); ++i)
	{
		_cmdFences[i] = _device->createFenceUnique(fenceInfo);
	}

	vk::SemaphoreCreateInfo renderSemInfo, presentSemInfo;

	_renderSem = _device->createSemaphoreUnique(renderSemInfo);
	_presentSem = _device->createSemaphoreUnique(presentSemInfo);
}

void fdf::Renderer::acquireNextImgIndex() {
	vk::ResultValue result = _device->acquireNextImageKHR(
		_swapchain.get(),
		UINT64_MAX,
		{ _presentSem.get() }
	);

	// TODO: error validation
	_currentImgIndex = result.value;
}

void fdf::Renderer::recordCmd() {
	// 前フレームのコマンド実行完了を待機
	_device->waitForFences({ _cmdFences[_currentImgIndex].get()}, VK_TRUE, UINT32_MAX);

	vk::CommandBufferBeginInfo cmdBeginInfo;
	_cmdBuffer[_currentImgIndex]->begin(cmdBeginInfo);

	vk::RenderPassBeginInfo rpBeginInfo;
	rpBeginInfo.renderPass = _renderPass.get();
	rpBeginInfo.framebuffer = _framebuffers[_currentImgIndex].get();
	rpBeginInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
	rpBeginInfo.renderArea.extent = vk::Extent2D{ kWinWidth, kWinHeight };
	rpBeginInfo.clearValueCount = 1;
	vk::ClearValue clearValue = { {0.0f, 0.0f, 0.0f, 0.0f} };
	rpBeginInfo.pClearValues = &clearValue;

	_cmdBuffer[_currentImgIndex]->beginRenderPass(
		rpBeginInfo,
		vk::SubpassContents::eInline
	);
	_cmdBuffer[_currentImgIndex]->bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.get());
	_cmdBuffer[_currentImgIndex]->bindVertexBuffers(0, { _vertexBuffer.get()}, { 0 });
	_cmdBuffer[_currentImgIndex]->bindIndexBuffer(_indexBuffer.get(), 0, vk::IndexType::eUint32);
	
	_cmdBuffer[_currentImgIndex]->bindDescriptorSets(
		vk::PipelineBindPoint::eGraphics,
		_pipelineLayout.get(),
		0,
		{ _descSets[_currentImgIndex].get() },
		nullptr
	);
	
	
	// _cmdBuffer->draw(_vertices.size(), 1, 0, 0);
	_cmdBuffer[_currentImgIndex]->drawIndexed(_vertexIndices.size(), 1, 0, 0, 0);

	_cmdBuffer[_currentImgIndex]->endRenderPass();
	_cmdBuffer[_currentImgIndex]->end();
}

void fdf::Renderer::submitCmd() {
	vk::SubmitInfo info;
	info.commandBufferCount = 1;
	info.pCommandBuffers = &_cmdBuffer[_currentImgIndex].get();
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &_presentSem.get();
	info.signalSemaphoreCount = 1;
	info.pSignalSemaphores = &_renderSem.get();

	_device->resetFences({ _cmdFences[_currentImgIndex].get() });
	_queue.submit({ info }, _cmdFences[_currentImgIndex].get());
}

void fdf::Renderer::drawFrame() {
	vk::PresentInfoKHR info;
	info.swapchainCount = 1;
	info.pSwapchains = &_swapchain.get();
	info.pImageIndices = &_currentImgIndex;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &_renderSem.get();

	// TODO: error handle
	_queue.presentKHR(&info);
}

void fdf::Renderer::importVertex(
	const std::vector<fdf::Vertex>& vertices,
	size_t row, size_t col)
{
	const size_t dataSize = sizeof(fdf::Vertex) * vertices.size();
	_vertexIndices = createVertexIndicesStrip(row, col);
	_vertices = vertices;

	createBuffer(_vertexBuffer, _vertexBufferMemory,
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible,
		dataSize
	);
	copyBuffer(_vertexBufferMemory, vertices.data(), dataSize);

	createBuffer(_indexBuffer, _indexBufferMemory,
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible,
		dataSize
	);
	copyBuffer(_indexBufferMemory, _vertexIndices.data(), sizeof(uint32_t) * _vertexIndices.size());
}

void fdf::Renderer::setUniformBuffer(const fdf::UniformBufferObject& ubo) {
	for (size_t i = 0; i < _uniformBuffersMapped.size(); ++i) {
		std::memcpy(&_uniformBuffersMapped[i], &ubo, sizeof(fdf::UniformBufferObject));
	}
}

void fdf::Renderer::createBuffer(
	vk::UniqueBuffer& dstBuf,
	vk::UniqueDeviceMemory& dstMem,
	vk::BufferUsageFlagBits usage,
	vk::MemoryPropertyFlagBits memProps,
	vk::DeviceSize size)
{
	/* Create buffer */
	vk::BufferCreateInfo createInfo;
	createInfo.usage = usage;
	createInfo.size = size;

	dstBuf = _device->createBufferUnique(createInfo);

	/* Allocate Buffer */
	vk::MemoryRequirements req = _device->getBufferMemoryRequirements(dstBuf.get());

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = req.size;

	std::optional<uint32_t> memTypeIdx = getMemoryTypeIndex(
		_physicalDevice,
		req.memoryTypeBits,
		memProps
	);
	if (!memTypeIdx.has_value()) {
		throw std::runtime_error("Suitable device memory not found");
	}

	allocInfo.memoryTypeIndex = memTypeIdx.value();
	dstMem = _device->allocateMemoryUnique(allocInfo);

	/* Bind memory */
	_device->bindBufferMemory(dstBuf.get(), dstMem.get(), 0);
}

void fdf::Renderer::copyBuffer(vk::UniqueDeviceMemory& dst, const void* src, size_t size) {
	void* mappedDst = _device->mapMemory(dst.get(), 0, size);
	std::memcpy(mappedDst, src, size);
	_device->unmapMemory(dst.get());
}

// 縮退三角形によるstrip最適化
// 参考: https://nakamura001.hatenablog.com/entry/20100111/1263219309
std::vector<uint32_t> fdf::Renderer::createVertexIndicesStrip(
	size_t row, size_t col)
{
	std::vector<uint32_t> indices;

	// 各列にそってstrip indexを生成する
	// 一番左の列から順に、縦に長い長方形が完成していくイメージ
	// 縮退三角形を生成するため、長方形の左上・右下のindexをそれぞれ2回連続で記録する
	size_t stripStartCol = 0;
	while (stripStartCol != col - 1) {
		uint32_t vertIndex = stripStartCol;

		// 縮退三角形の生成
		// 長方形の左上は2回記録
		if (stripStartCol != 0) {
			indices.push_back(vertIndex);
		}

		for (size_t i = 0; i < row; ++i) {
			indices.push_back(vertIndex);
			indices.push_back(vertIndex + 1);
			vertIndex += col;
		}

		// 縮退三角形の生成
		// 長方形の右下は2回記録
		indices.push_back(col * (row - 1) + stripStartCol + 1);
		++stripStartCol;
	}

	return indices;
}

std::optional<uint32_t> fdf::Renderer::searchQueueFamily(
	const vk::PhysicalDevice& physicalDevice,
	vk::QueueFlagBits queueFamily)
{
	std::vector<vk::QueueFamilyProperties> props =
		physicalDevice.getQueueFamilyProperties();
	std::optional<uint32_t> res;
	
	for (uint32_t i = 0; i < props.size(); ++i) {
		if (props[i].queueFlags & queueFamily) {
			res = i;
			break;
		}
	}

	return res;
}

std::optional<uint32_t> fdf::Renderer::getMemoryTypeIndex(
	const vk::PhysicalDevice& physicalDevice,
	uint32_t requestBits,
	vk::MemoryPropertyFlagBits requestProps)
{
	auto memProps = physicalDevice.getMemoryProperties();
	std::optional<uint32_t> res;

	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
		bool matchProps = (bool)(memProps.memoryTypes[i].propertyFlags & requestProps);

		if (matchProps && (requestBits & (1 << i))) {
			res = i;
			break;
		}
	}

	return res;
}

void fdf::Renderer::updateUniformBuffer()
{
	/*
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	_ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	_ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	_ubo.proj = glm::perspective(glm::radians(30.0f), (float)kWinWidth / kWinHeight, 0.0f, 1.0f);
	_ubo.proj[1][1] *= -1;
	*/

	std::memcpy(_uniformBuffersMapped[_currentImgIndex], &_ubo, sizeof(_ubo));
}

void fdf::Renderer::kayCallback(
	GLFWwindow* win,
	int key,
	int scancode,
	int action,
	int mods)
{
	const float kRotationRate = 3.0f;
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_RIGHT:
			_ubo.model = glm::rotate(_ubo.model, glm::radians(-kRotationRate), glm::vec3(0.0f, 0.0f, 1.0f));
			break;

		case GLFW_KEY_LEFT:
			_ubo.model = glm::rotate(_ubo.model, glm::radians(kRotationRate), glm::vec3(0.0f, 0.0f, 1.0f));
			break;

		case GLFW_KEY_UP:
			_ubo.model = glm::rotate(_ubo.model, glm::radians(-kRotationRate), glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		case GLFW_KEY_DOWN:
			_ubo.model = glm::rotate(_ubo.model, glm::radians(kRotationRate), glm::vec3(1.0f, 0.0f, 0.0f));
			break;

		default:
			break;
		}
	}
}

void fdf::Renderer::scrollCallback(GLFWwindow* win, double xoffset, double yoffset)
{
	float scaleRate = 1.2f;
	if (yoffset < 0) {
		scaleRate = 1 / scaleRate;
	}

	_ubo.model = glm::scale(_ubo.model, glm::vec3(scaleRate));
}

/*
int main() {
	fdf::Renderer renderer;

	try {
		renderer.init();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	renderer.loop();

	return 0;
}
*/
