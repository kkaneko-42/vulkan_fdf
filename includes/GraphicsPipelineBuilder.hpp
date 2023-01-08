#pragma once

#include <vulkan/vulkan.hpp>
#include <fstream>

namespace fdf
{
	class GraphicsPipelineBuilder
	{
	public:
		struct BuildResult
		{
			vk::UniquePipeline pipeline;
			vk::UniquePipelineLayout layout;
		};

		static const uint32_t kVertexBinding;
		static const uint32_t kVertexPosLocation;
		static const uint32_t kVertexColorLocation;

		GraphicsPipelineBuilder(const vk::Device& device)
			: _device(device) {
			setDefault();
		}

		BuildResult build();
		void addShaderStage(
			vk::ShaderStageFlagBits stage,
			const std::string& filename,
			const char* entrypoint
		);

		void setPrimitiveTopology(vk::PrimitiveTopology topology) {
			_inputAsm.topology = topology;
		}

		void setVertexBinding(uint32_t stride) {
			_vertexBinding.stride = stride;
		}
		void setVertexPosAttribute(vk::Format format, uint32_t offset);
		void setVertexColorAttribute(vk::Format format, uint32_t offset);

		void setDescriptorSetLayouts(const vk::DescriptorSetLayout* layouts, uint32_t size);

		void setPolygonMode(vk::PolygonMode mode) {
			_rasterization.polygonMode = mode;
		}

		void setWH(uint32_t width, uint32_t height) {
			_viewport.width = width;
			_viewport.height = height;
			_scissor.extent = vk::Extent2D{ width, height };
		}

		void setRenderPass(const vk::RenderPass& renderPass) {
			_renderPass = renderPass;
		}

		void setSubpass(uint32_t subpass) {
			_subpass = subpass;
		}

	private:
		const vk::Device _device;
		
		std::vector<vk::UniqueShaderModule> _shaderModules;
		std::vector<vk::PipelineShaderStageCreateInfo> _shaderStages;
		
		vk::PipelineVertexInputStateCreateInfo _vertexInput;
		vk::VertexInputBindingDescription _vertexBinding;
		vk::VertexInputAttributeDescription _vertexAttributes[2];
		
		vk::PipelineInputAssemblyStateCreateInfo _inputAsm;

		vk::PipelineTessellationStateCreateInfo _tesselation;

		vk::Viewport _viewport;
		vk::Rect2D _scissor;
		vk::PipelineViewportStateCreateInfo _viewportState;

		vk::PipelineRasterizationStateCreateInfo _rasterization;

		vk::PipelineMultisampleStateCreateInfo _multisample;

		vk::PipelineDepthStencilStateCreateInfo _depthStencil;

		vk::PipelineColorBlendAttachmentState _blendAttachment;
		vk::PipelineColorBlendStateCreateInfo _colorBlend;

		vk::PipelineDynamicStateCreateInfo _dynamic;

		vk::PipelineLayoutCreateInfo _layoutInfo;
		vk::UniquePipelineLayout	 _layout;
		
		vk::RenderPass _renderPass;
		
		uint32_t _subpass;

		void setDefault();
		vk::GraphicsPipelineCreateInfo getCreateInfo();
		vk::UniqueShaderModule createShaderModule(const std::string& filename) const;
	};
}