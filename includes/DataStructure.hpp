#ifndef FDF_DATA_STRUCTURE_HPP
#define FDF_DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace fdf
{
	struct Vertex
	{
		glm::vec3 pos;
		glm::vec4 rgba;

		void print() const {
			std::cout << "(";
			std::cout << pos.x << ", ";
			std::cout << pos.y << ", ";
			std::cout << pos.z;
			std::cout << "), ";

			std::cout << "(";
			std::cout << rgba.x << ", ";
			std::cout << rgba.y << ", ";
			std::cout << rgba.z << ", ";
			std::cout << rgba.w;
			std::cout << ")" << std::endl;
		}

		// 0xff0000ff => (1.0f, 0.0f, 0.0f, 1.0f)
		void setRGBA(uint32_t color) {
			rgba.x = (color & 0xff000000);
			rgba.y = (color & 0x00ff0000);
			rgba.z = (color & 0x0000ff00);
			rgba.w = (color & 0x000000ff);
			// rgba /= 255;
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
}

#endif
