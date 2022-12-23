#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <optional>

const std::vector<const char*> requiredLayers = {
	"VK_LAYER_KHRONOS_validation",
};

const uint32_t WIDTH = 640;
const uint32_t HEIGHT = 480;

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();
	return buffer;
}

// デバイスのキューファミリを全て出力
// GPUの「やることリスト」がキュー
// キューはすべての「やること」をサポートするとは限らない
// 同じ能力を持つキューたちをひとまとめにしたのが「キューファミリ」
// キューファミリにはそれぞれ通し番号(index)が与えられる
/*
出力例
queue family count: 3

queue family index: 0
  queue count: 16
  graphic support: True
  compute support: True
  transfer support: True

queue family index: 1
  queue count: 2
  graphic support: False
  compute support: False
  transfer support: True

queue family index: 2
  queue count: 8
  graphic support: False
  compute support: True
  transfer support: True
*/
void showQueueFamilies(const vk::PhysicalDevice& physicalDevice) {
	std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();

	std::cout << "queue family count: " << queueProps.size() << std::endl;
	std::cout << std::endl;
	for (size_t i = 0; i < queueProps.size(); i++)
	{
		std::cout << "queue family index: " << i << std::endl;
		std::cout << "  queue count: " << queueProps[i].queueCount << std::endl;
		std::cout << "  graphic support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics ? "True" : "False") << std::endl;
		std::cout << "  compute support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute ? "True" : "False") << std::endl;
		std::cout << "  transfer support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer ? "True" : "False") << std::endl;
		std::cout << std::endl;
	}
}

// 描画系コマンドをサポートするキューファミリが存在するか？
// あればindexに当該ファミリのインデックスを詰める
std::optional<uint32_t> searchGraphicQueueFamily(const vk::PhysicalDevice& physicalDevice) {
	std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
	std::optional<uint32_t> res;
	uint32_t i = 0;

	for (const auto& queueProp : queueProps) {
		if (queueProp.queueFlags & vk::QueueFlagBits::eGraphics) {
			res = i;
			break;
		}
		++i;
	}

	return res;
}

// 当該GPUボードが、スワップチェーンをサポートしているか？
bool checkSwapchainSupport(const vk::PhysicalDevice& physicalDevice) {
	std::vector<vk::ExtensionProperties> extProps = physicalDevice.enumerateDeviceExtensionProperties();

	for (const auto& extProp : extProps) {
		if (extProp.extensionName == std::string(VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
			return true;
		}
	}

	return false;
}

bool checkSurfaceSupport(
	const vk::PhysicalDevice& physicalDevice,
	const vk::UniqueSurfaceKHR& surface,
	const std::optional<uint32_t> queueFamily) {
	if (!queueFamily.has_value()) {
		return false;
	}

	return (
		!physicalDevice.getSurfaceFormatsKHR(surface.get()).empty() &&
		!physicalDevice.getSurfacePresentModesKHR(surface.get()).empty() &&
		physicalDevice.getSurfaceSupportKHR(queueFamily.value(), surface.get())
	);
}

vk::UniqueInstance createInstance() {
	// 画面に表示する為の拡張機能を取得
	// NOTE: vulkanにおいて、画面表示は拡張機能
	uint32_t requiredExtensionsCount;
	const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

	vk::InstanceCreateInfo instCreateInfo;
	instCreateInfo.enabledExtensionCount = requiredExtensionsCount;
	instCreateInfo.ppEnabledExtensionNames = requiredExtensions;
	// レイヤーの設定
	instCreateInfo.enabledLayerCount = requiredLayers.size();
	instCreateInfo.ppEnabledLayerNames = requiredLayers.data();

	return vk::createInstanceUnique(instCreateInfo);
}

// 適切なデバイスを選択する
// 例: 描画系コマンドをサポートするキューファミリを持つデバイス
// NOTE: なければ例外投げる
vk::PhysicalDevice pickPhysicalDevice(
	const vk::UniqueInstance& instance,
	const vk::UniqueSurfaceKHR& surface)
{
	std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

	for (const auto& physicalDevice : physicalDevices) {
		// 描画系コマンドをサポートしているか？
		std::optional<uint32_t> family = searchGraphicQueueFamily(physicalDevice);
		// swap chainをサポートしているか？
		bool supportSwapchain = checkSwapchainSupport(physicalDevice);
		// surfaceをサポートしているか？
		bool supportSurface = checkSurfaceSupport(physicalDevice, surface, family);

		if (family.has_value() && supportSwapchain && supportSurface) {
			return physicalDevice;
		}
	}

	throw std::runtime_error("graphic not supported");
}

vk::UniqueDevice createLogicalDevice(const vk::PhysicalDevice& physicalDevice) {
	std::vector<const char*> devRequiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	vk::DeviceCreateInfo devCreateInfo;

	// どのキューを使うのか、予め伝える必要がある
	// 今のところ、欲しいQueueは1つ
	vk::DeviceQueueCreateInfo queueCreateInfo[1];
	// キューファミリのインデックス
	queueCreateInfo[0].queueFamilyIndex = searchGraphicQueueFamily(physicalDevice).value();
	// 当該ファミリから、何個のキューが欲しいか
	queueCreateInfo[0].queueCount = 1;
	// キューのタスク実行の優先度
	// NOTE: 優先度はキューごとに決められる
	float queuePriorities[1] = { 1.0f };
	queueCreateInfo[0].pQueuePriorities = queuePriorities;

	// キューの設定
	devCreateInfo.pQueueCreateInfos = queueCreateInfo;
	devCreateInfo.queueCreateInfoCount = 1;
	// 拡張機能(swap chain)の設定
	devCreateInfo.ppEnabledExtensionNames = devRequiredExtensions.data();
	devCreateInfo.enabledExtensionCount = devRequiredExtensions.size();

	return physicalDevice.createDeviceUnique(devCreateInfo);
}

vk::UniqueImage createImage(const vk::UniqueDevice& device) {
	vk::ImageCreateInfo imgCreateInfo;
	imgCreateInfo.imageType = vk::ImageType::e2D;
	imgCreateInfo.extent = vk::Extent3D(WIDTH, HEIGHT, 1);
	imgCreateInfo.mipLevels = 1;
	imgCreateInfo.arrayLayers = 1;
	// RGBAをそれぞれ8bitで持つ
	imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
	imgCreateInfo.tiling = vk::ImageTiling::eLinear;
	imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
	imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
	imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
	imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

	return device->createImageUnique(imgCreateInfo);
}

vk::UniqueDeviceMemory allocateImageMemory(
	const vk::PhysicalDevice& physicalDevice,
	const vk::UniqueDevice& device,
	const vk::UniqueImage& image)
{
	// GPU上にどんなデバイスメモリが乗っているか
	vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

	// イメージに必要なGPU上のデバイスメモリ情報(サイズ、種類)を取得
	vk::MemoryRequirements imgMemReq = device->getImageMemoryRequirements(image.get());

	vk::MemoryAllocateInfo imgMemAllocInfo;
	imgMemAllocInfo.allocationSize = imgMemReq.size;

	// 必要なメモリと、利用可能なメモリの照合
	for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
		if (imgMemReq.memoryTypeBits & (1 << i)) {
			imgMemAllocInfo.memoryTypeIndex = i;
			return device->allocateMemoryUnique(imgMemAllocInfo);
		}
	}

	throw std::runtime_error("Suitable device memory not found");
}

vk::UniqueShaderModule createShaderModule(
	const vk::UniqueDevice& device,
	const std::string& filename)
{
	std::vector<char> shaderCode = readFile(filename);

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	return device->createShaderModuleUnique(createInfo);
}

vk::UniquePipeline createGraphicsPipeline(
	const vk::UniqueDevice& device,
	const vk::UniqueRenderPass& renderPass)
{
	vk::Viewport viewports[1];
	viewports[0].x = 0.0;
	viewports[0].y = 0.0;
	viewports[0].minDepth = 0.0;
	viewports[0].maxDepth = 1.0;
	viewports[0].width = WIDTH;
	viewports[0].height = HEIGHT;

	vk::Rect2D scissors[1];
	scissors[0].offset.setX(0);
	scissors[0].offset.setY(0);
	scissors[0].extent.setWidth(WIDTH);
	scissors[0].extent.setHeight(HEIGHT);

	vk::PipelineViewportStateCreateInfo viewportState;
	viewportState.viewportCount = 1;
	viewportState.pViewports = viewports;
	viewportState.scissorCount = 1;
	viewportState.pScissors = scissors;

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
	inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
	inputAssembly.primitiveRestartEnable = false;

	vk::PipelineRasterizationStateCreateInfo rasterizer;
	rasterizer.depthClampEnable = false;
	rasterizer.rasterizerDiscardEnable = false;
	rasterizer.polygonMode = vk::PolygonMode::eFill;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = vk::CullModeFlagBits::eBack;
	rasterizer.frontFace = vk::FrontFace::eClockwise;
	rasterizer.depthBiasEnable = false;

	vk::PipelineMultisampleStateCreateInfo multisample;
	multisample.sampleShadingEnable = false;
	multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

	vk::PipelineColorBlendAttachmentState blendattachment[1];
	blendattachment[0].colorWriteMask =
		vk::ColorComponentFlagBits::eA |
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB;
	blendattachment[0].blendEnable = false;

	vk::PipelineColorBlendStateCreateInfo blend;
	blend.logicOpEnable = false;
	blend.attachmentCount = 1;
	blend.pAttachments = blendattachment;

	vk::PipelineLayoutCreateInfo layoutCreateInfo;
	layoutCreateInfo.setLayoutCount = 0;
	layoutCreateInfo.pSetLayouts = nullptr;

	vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(layoutCreateInfo);

	// shader moduleの作成
	vk::UniqueShaderModule vertShader = createShaderModule(device, "shaders/vert.spv");
	vk::UniqueShaderModule fragShader = createShaderModule(device, "shaders/frag.spv");
	vk::PipelineShaderStageCreateInfo shaderStage[2];
	shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
	shaderStage[0].module = vertShader.get();
	shaderStage[0].pName = "main";
	shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
	shaderStage[1].module = fragShader.get();
	shaderStage[1].pName = "main";

	vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pRasterizationState = &rasterizer;
	pipelineCreateInfo.pMultisampleState = &multisample;
	pipelineCreateInfo.pColorBlendState = &blend;
	pipelineCreateInfo.layout = pipelineLayout.get();
	pipelineCreateInfo.renderPass = renderPass.get();
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStage;

	return device->createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;
}

// パイプライン処理の中からイメージを扱うためのオブジェクトを作成
// パイプライン内の各アタッチメントに結びつける
vk::UniqueImageView createImageView(
	const vk::UniqueDevice& device,
	const vk::UniqueImage& image)
{
	vk::ImageViewCreateInfo imgViewCreateInfo;
	imgViewCreateInfo.image = image.get();
	imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
	imgViewCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
	imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
	imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
	imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
	imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
	imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imgViewCreateInfo.subresourceRange.levelCount = 1;
	imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imgViewCreateInfo.subresourceRange.layerCount = 1;

	return device->createImageViewUnique(imgViewCreateInfo);
}

// レンダーパスが取り扱うデータ(attachment)の具体的な内容を決める為のオブジェクトを作成
// NOTE: 実際にrenderPassとimageViewが結びつく訳ではない
//       フレームバッファが結びつき得るレンダーパスを指定するのみ
vk::UniqueFramebuffer createFrameBuffer(
	const vk::UniqueDevice& device,
	const vk::UniqueImageView& imgView,
	const vk::UniqueRenderPass& renderPass)
{
	vk::ImageView frameBufAttachments[1];
	frameBufAttachments[0] = imgView.get();

	vk::FramebufferCreateInfo frameBufCreateInfo;
	frameBufCreateInfo.width = WIDTH;
	frameBufCreateInfo.height = HEIGHT;
	frameBufCreateInfo.layers = 1;
	frameBufCreateInfo.renderPass = renderPass.get();
	frameBufCreateInfo.attachmentCount = 1;
	frameBufCreateInfo.pAttachments = frameBufAttachments;

	return device->createFramebufferUnique(frameBufCreateInfo);
}

void recordCommands(
	std::vector<vk::UniqueCommandBuffer>& cmdBufs,
	const vk::UniqueRenderPass& renderPass,
	const vk::UniqueFramebuffer& frameBuf,
	const vk::UniquePipeline& pipeline)
{
	vk::CommandBufferBeginInfo cmdBeginInfo;
	cmdBufs[0]->begin(cmdBeginInfo);
	// ここからコマンド記録開始

	vk::RenderPassBeginInfo renderPassBeginInfo;
	renderPassBeginInfo.renderPass = renderPass.get();
	renderPassBeginInfo.framebuffer = frameBuf.get();
	renderPassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, { WIDTH, HEIGHT });
	renderPassBeginInfo.clearValueCount = 0;
	renderPassBeginInfo.pClearValues = nullptr;

	cmdBufs[0]->beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	// ここからサブパス0番の処理
	// 例えば以下のようにすれば、サブパス1番へ以降する
	// cmdBufs[0]->nextSubpass(vk::SubpassContents::eInline);

	cmdBufs[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
	cmdBufs[0]->draw(
		3, /* 頂点の数 */
		1, /* インスタンスの数(?) */
		0, /* 頂点のオフセット */
		0  /* インスタンスのオフセット */
	);/* オフセットを用いて、例えば2000個の頂点について、前1000個は物体Aのモデルデータ、後は物体Bという使い方ができる */

	// ここまでサブパス0番の処理
	cmdBufs[0]->endRenderPass();

	// ここまでコマンド記録
	cmdBufs[0]->end();
}

GLFWwindow* createWindow() {
	if (!glfwInit()) {
		throw std::runtime_error("failed to init window");
	}

	// GLFWは、デフォルトではOpenGLを期待するので、それをOFF
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);
	if (!window) {
		throw std::runtime_error("failed to create error!");
	}

	return window;
}
vk::UniqueSurfaceKHR createWindowSurface(
	const vk::UniqueInstance& instance,
	GLFWwindow* window)
{
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(instance.get(), window, nullptr, &surface);
	if (result != VK_SUCCESS) {
		const char* err;
		glfwGetError(&err);
		std::cerr << err << std::endl;
		abort();
		throw std::runtime_error("failed to create window surface");
	}
	
	return vk::UniqueSurfaceKHR{ surface, instance.get() };
}

vk::UniqueSwapchainKHR createSwapchain(
	const vk::PhysicalDevice& physicalDevice,
	const vk::UniqueDevice& device,
	const vk::UniqueSurfaceKHR& surface,
	vk::SurfaceFormatKHR& swapchainFormat,
	vk::PresentModeKHR& swapchainPresentMode)
{
	auto surfaceCapabilities        = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
	std::vector surfaceFormats      = physicalDevice.getSurfaceFormatsKHR(surface.get());
	std::vector surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());

	swapchainFormat = surfaceFormats[0];
	swapchainPresentMode = surfacePresentModes[0];

	vk::SwapchainCreateInfoKHR swapchainCreateInfo;
	swapchainCreateInfo.surface				= surface.get();
	swapchainCreateInfo.minImageCount		= surfaceCapabilities.minImageCount + 1;
	swapchainCreateInfo.imageFormat			= swapchainFormat.format;
	swapchainCreateInfo.imageColorSpace		= swapchainFormat.colorSpace;
	swapchainCreateInfo.imageExtent			= surfaceCapabilities.currentExtent;
	swapchainCreateInfo.imageArrayLayers	= 1;
	swapchainCreateInfo.imageUsage			= vk::ImageUsageFlagBits::eColorAttachment;
	swapchainCreateInfo.imageSharingMode	= vk::SharingMode::eExclusive;
	swapchainCreateInfo.preTransform		= surfaceCapabilities.currentTransform;
	swapchainCreateInfo.presentMode			= swapchainPresentMode;
	swapchainCreateInfo.clipped				= VK_TRUE;

	return device->createSwapchainKHRUnique(swapchainCreateInfo);
}

std::vector<vk::UniqueImageView> createSwapchainImageViews(
	const vk::UniqueDevice& device,
	const std::vector<vk::Image>& images,
	const vk::SurfaceFormatKHR& swapchainFormat)
{
	std::vector<vk::UniqueImageView> views(images.size());

	for (size_t i = 0; i < views.size(); ++i) {
		vk::ImageViewCreateInfo imgViewCreateInfo;
		imgViewCreateInfo.image = images[i];
		imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
		imgViewCreateInfo.format = swapchainFormat.format; // 元のcreateImageViewsとはここだけ違う
		imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
		imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
		imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
		imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
		imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imgViewCreateInfo.subresourceRange.levelCount = 1;
		imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imgViewCreateInfo.subresourceRange.layerCount = 1;

		views[i] = device->createImageViewUnique(imgViewCreateInfo);
	}

	return views;
}

vk::UniqueRenderPass createSwapchainRenderpass(
	const vk::UniqueDevice& device,
	const vk::SurfaceFormatKHR& swapchainFormat)
{
	vk::AttachmentDescription attachments[1];
	// swapchainのフォーマットに合わせる
	attachments[0].format = swapchainFormat.format;
	attachments[0].samples = vk::SampleCountFlagBits::e1;
	attachments[0].loadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
	attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachments[0].initialLayout = vk::ImageLayout::eUndefined;
	// imageの取り扱い方
	// 例: VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: カラーアタッチメントとして使用
	//     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: メモリのコピー先として使用
	// 今回はswap chainによる表示を目的として、VK_IMAGE_LAYOUT_PRESENT_SRC_KHRを指定
	// NOTE: eGeneralでも動くらしい
	attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

	vk::AttachmentReference subpass0_attachmentRefs[1];
	subpass0_attachmentRefs[0].attachment = 0; // n番目のアタッチメント
	subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

	vk::SubpassDescription subpasses[1];
	subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpasses[0].colorAttachmentCount = 1;
	subpasses[0].pColorAttachments = subpass0_attachmentRefs;

	vk::RenderPassCreateInfo renderPassCreateInfo;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = subpasses;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	return device->createRenderPassUnique(renderPassCreateInfo);
}

uint32_t acquireNextImageIndex(
	const vk::UniqueDevice& device,
	const vk::UniqueSwapchainKHR& swapchain)
{
	vk::FenceCreateInfo fenceCreateInfo;
	fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
	vk::UniqueFence fence = device->createFenceUnique(fenceCreateInfo);

	// NOTE: 非同期。関数がreturnしたからといって、swapchainが準備完了したわけではない
	vk::ResultValue result = device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, {}, { fence.get() });
	if (result.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to acquire next image");
	}

	// 実際はここでindex取得できるらしいが、一応待機
	device->waitForFences({ fence.get() }, VK_TRUE, UINT32_MAX);
	
	return result.value;
}

void submitCommands(
	const std::vector<vk::CommandBuffer>& cmdBufs,
	const vk::Queue& graphicsQueue)
{
	vk::SubmitInfo submitInfo;
	submitInfo.commandBufferCount = cmdBufs.size();
	submitInfo.pCommandBuffers = cmdBufs.data();

	// 第二引数にはfenceを指定し、CPU側で待機できる
	graphicsQueue.submit({ submitInfo }, nullptr);
}

// NOTE: presentationはGPU側の処理なので、コマンドを送信する
//		 尚、コマンドの送信はsubmitではなく専用APIで行う
void presentRenderResult(
	const vk::UniqueSwapchainKHR& swapchain,
	uint32_t imgIndex,
	const vk::Queue& graphicsQueue)
{
	
	vk::PresentInfoKHR presentInfo;

	std::vector<vk::SwapchainKHR> presentSwapchains = { swapchain.get() };
	std::vector<uint32_t> imgIndices = { imgIndex };

	presentInfo.swapchainCount = presentSwapchains.size();
	presentInfo.pSwapchains = presentSwapchains.data();
	presentInfo.pImageIndices = imgIndices.data();

	// presentationのコマンドをGPUに送信する
	graphicsQueue.presentKHR(presentInfo);
}

int main() {
	// ウィンドウ作成
	// NOTE: glfwのinitがあるので、createInstanceより前に呼ばれることを期待
	GLFWwindow * window = createWindow();

	// インスタンス作成
	vk::UniqueInstance instance = createInstance();

	// ウィンドウサーフェスを作成
	// プラットフォームに依らず、ここに書き込めば画面に表示できる
	// NOTE: したがって、この作成処理はプラットフォーム依存
	vk::UniqueSurfaceKHR surface = createWindowSurface(instance, window);

	// 物理デバイス作成
	vk::PhysicalDevice physicalDevice = pickPhysicalDevice(instance, surface);

	// 論理デバイス作成
	vk::UniqueDevice device = createLogicalDevice(physicalDevice);

	// キューの取得
	// NOTE: 第二引数は、ファミリ内で何番目のキューを取得するか
	uint32_t queueFamilyIndex = searchGraphicQueueFamily(physicalDevice).value();
	vk::Queue graphicsQueue = device->getQueue(queueFamilyIndex, 0);

	// コマンドプールの作成
	// このプールから、GPUの「やること」であるコマンドを並べたCommand bufferを生成する
	vk::CommandPoolCreateInfo cmdPoolCreateInfo;
	cmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
	vk::UniqueCommandPool cmdPool = device->createCommandPoolUnique(cmdPoolCreateInfo);

	// コマンドバッファの作成
	// 作成したコマンドプールからallocateされる
	vk::CommandBufferAllocateInfo cmdBufAllocInfo;
	cmdBufAllocInfo.commandPool = cmdPool.get();
	cmdBufAllocInfo.commandBufferCount = 1;
	cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
	std::vector<vk::UniqueCommandBuffer> cmdBufs
		= device->allocateCommandBuffersUnique(cmdBufAllocInfo);

	// swap chainの作成, 当該format, presentModeの取得
	vk::SurfaceFormatKHR swapchainFormat;
	vk::PresentModeKHR swapchainPresentMode;
	vk::UniqueSwapchainKHR swapchain = createSwapchain(
		physicalDevice,
		device,
		surface,
		swapchainFormat,
		swapchainPresentMode
	);

	// swap chain用imageの作成
	std::vector<vk::Image> swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

	// swap chain用のimage view作成
	std::vector<vk::UniqueImageView> swapchainImageViews = createSwapchainImageViews(
		device,
		swapchainImages,
		swapchainFormat
	);

	// レンダーパスの作成
	// swapchain用のレンダーパス
	vk::UniqueRenderPass renderPass = createSwapchainRenderpass(
		device,
		swapchainFormat
	);

	// フレームバッファの作成
	// swapchain用のframebuffers
	std::vector<vk::UniqueFramebuffer> swapchainFramebuffers(swapchainImageViews.size());
	for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
		swapchainFramebuffers[i] = createFrameBuffer(
			device,
			swapchainImageViews[i],
			renderPass
		);
	}

	// グラフィックスパイプラインの作成
	vk::UniquePipeline pipeline = createGraphicsPipeline(device, renderPass);

	// 描画ループ
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		// swapchainが描画すべきimageのインデックス(表示されているimageの次)を取得
		// NOTE: ここでacquireの完了待機も行う
		uint32_t imgIndex = acquireNextImageIndex(device, swapchain);

		// コマンドを記録
		recordCommands(
			cmdBufs,
			renderPass,
			swapchainFramebuffers[imgIndex],
			pipeline
		);

		// 記録したコマンドバッファをキューに送信する
		std::vector submitCmdBufs = { cmdBufs[0].get() };
		submitCommands(
			submitCmdBufs,
			graphicsQueue
		);

		// キューが空になるまで待つ
		graphicsQueue.waitIdle();

		// presentation処理
		presentRenderResult(swapchain, imgIndex, graphicsQueue);
	}

	glfwTerminate();
	return 0;
}