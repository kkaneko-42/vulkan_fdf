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
		uint32_t rgba;

		// Vertex() : x(0), y(0), z(0), rgba(0x000000ff) {}

		void print() const {
			std::cout << "(";
			std::cout << pos.x << ", ";
			std::cout << pos.y << ", ";
			std::cout << pos.z << ", ";
			std::cout << std::hex << rgba;
			std::cout << ")" << std::endl;
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
}

#endif
