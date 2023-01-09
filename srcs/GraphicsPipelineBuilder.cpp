#include "GraphicsPipelineBuilder.hpp"

const uint32_t fdf::GraphicsPipelineBuilder::kVertexPosLocation = 0;
const uint32_t fdf::GraphicsPipelineBuilder::kVertexColorLocation = 1;
const uint32_t fdf::GraphicsPipelineBuilder::kVertexBinding = 0;

static std::vector<char> readFile(const std::string& filename) {
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

#include <iostream>

fdf::GraphicsPipelineBuilder::BuildResult fdf::GraphicsPipelineBuilder::build()
{
	vk::ResultValue result = _device.createGraphicsPipelineUnique(
		nullptr, getCreateInfo()
	);

	if (result.result != vk::Result::eSuccess) {
		// error
	}

	return BuildResult{ std::move(result.value), std::move(_layout) };
}

void fdf::GraphicsPipelineBuilder::addShaderStage(
	vk::ShaderStageFlagBits stage,
	const std::string& filename,
	const char* entrypoint)
{
	_shaderModules.push_back(createShaderModule(filename));

	vk::PipelineShaderStageCreateInfo createInfo;
	createInfo.stage = stage;
	createInfo.module = _shaderModules.back().get();
	createInfo.pName = entrypoint;

	_shaderStages.push_back(createInfo);
}

void fdf::GraphicsPipelineBuilder::setVertexPosAttribute(
	vk::Format format, uint32_t offset)
{
	_vertexAttributes[kVertexPosLocation].format = format;
	_vertexAttributes[kVertexPosLocation].offset = offset;
}

void fdf::GraphicsPipelineBuilder::setVertexColorAttribute(
	vk::Format format, uint32_t offset)
{
	_vertexAttributes[kVertexColorLocation].format = format;
	_vertexAttributes[kVertexColorLocation].offset = offset;
}

void fdf::GraphicsPipelineBuilder::setDescriptorSetLayouts(
	const vk::DescriptorSetLayout* layouts, uint32_t size)
{
	_layoutInfo.setLayoutCount = size;
	_layoutInfo.pSetLayouts = layouts;
}

void fdf::GraphicsPipelineBuilder::setDefault() {
	_viewport.x = 0.0;
	_viewport.y = 0.0;
	_viewport.minDepth = 0.0;
	_viewport.maxDepth = 1.0;
	_scissor.offset = vk::Offset2D{ 0, 0 };
	_viewportState.viewportCount = 1;
	_viewportState.pViewports = &_viewport;
	_viewportState.scissorCount = 1;
	_viewportState.pScissors = &_scissor;

	_vertexBinding.binding = kVertexBinding;
	_vertexBinding.inputRate = vk::VertexInputRate::eVertex;
	_vertexAttributes[kVertexPosLocation].location = kVertexPosLocation;
	_vertexAttributes[kVertexColorLocation].location = kVertexColorLocation;

	_vertexInput.vertexBindingDescriptionCount = 1;
	_vertexInput.pVertexBindingDescriptions = &_vertexBinding;
	_vertexInput.vertexAttributeDescriptionCount = 1;
	_vertexInput.pVertexAttributeDescriptions = _vertexAttributes;

	/*
	_vertexInput.vertexBindingDescriptionCount = 0;
	_vertexInput.pVertexBindingDescriptions = nullptr;
	_vertexInput.vertexAttributeDescriptionCount = 0;
	_vertexInput.pVertexAttributeDescriptions = nullptr;
	*/

	_inputAsm.topology = vk::PrimitiveTopology::eTriangleList;
	_inputAsm.primitiveRestartEnable = false;

	_rasterization.depthClampEnable = false;
	_rasterization.rasterizerDiscardEnable = false;
	_rasterization.polygonMode = vk::PolygonMode::eFill;
	_rasterization.lineWidth = 1.0f;
	_rasterization.cullMode = vk::CullModeFlagBits::eNone;
	_rasterization.frontFace = vk::FrontFace::eCounterClockwise;
	_rasterization.depthBiasEnable = false;

	_multisample.sampleShadingEnable = false;
	_multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

	_blendAttachment.colorWriteMask =
		vk::ColorComponentFlagBits::eA |
		vk::ColorComponentFlagBits::eR |
		vk::ColorComponentFlagBits::eG |
		vk::ColorComponentFlagBits::eB;
	_blendAttachment.blendEnable = false;

	_colorBlend.logicOpEnable = false;
	_colorBlend.attachmentCount = 1;
	_colorBlend.pAttachments = &_blendAttachment;

	_layoutInfo.setLayoutCount = 0;
	_layoutInfo.pSetLayouts = nullptr;

	_subpass = 0;
}

vk::GraphicsPipelineCreateInfo fdf::GraphicsPipelineBuilder::getCreateInfo()
{
	_layout = _device.createPipelineLayoutUnique(_layoutInfo);
	vk::GraphicsPipelineCreateInfo createInfo;

	createInfo.stageCount = _shaderStages.size();
	createInfo.pStages = _shaderStages.data();
	createInfo.pVertexInputState = &_vertexInput;
	createInfo.pInputAssemblyState = &_inputAsm;
	createInfo.pTessellationState = &_tesselation;
	createInfo.pViewportState = &_viewportState;
	createInfo.pRasterizationState = &_rasterization;
	createInfo.pMultisampleState = &_multisample;
	createInfo.pDepthStencilState = &_depthStencil;
	createInfo.pColorBlendState = &_colorBlend;
	createInfo.layout = _layout.get();
	createInfo.renderPass = _renderPass;
	createInfo.subpass = _subpass;

	return createInfo;
}

vk::UniqueShaderModule fdf::GraphicsPipelineBuilder::createShaderModule(
	const std::string& filename) const
{
	std::vector<char> shaderCode = readFile(filename);

	vk::ShaderModuleCreateInfo createInfo;
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

	return _device.createShaderModuleUnique(createInfo);
}
