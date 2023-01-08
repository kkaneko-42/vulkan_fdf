#include "MapParser.hpp"
#include "Renderer.hpp"

const std::vector<fdf::Vertex> testVertices = {
	fdf::Vertex{ { -0.5f, -0.5f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.0f, -0.5f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.5f, -0.5f, 0.0f }, (uint32_t)0xffffffff },
	fdf::Vertex{ { -0.5f, 0.0f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.0f, 0.0f, 0.3f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.5f, 0.0f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { -0.5f, 0.5f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.0f, 0.5f, 0.0f }, (uint32_t)0xffffffff},
	fdf::Vertex{ { 0.5f, 0.5f, 0.0f }, (uint32_t)0xffffffff},
};
/*
const fdf::UniformBufferObject ubo = {
	glm::mat4(1.0f),
	glm::lookAt(
		glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, -1, 0)
	),
	glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.0f, 1.0f)
};
*/

const fdf::UniformBufferObject ubo = {
	glm::mat4(1.0f),
	glm::mat4(1.0f),
	glm::mat4(1.0f)
};

int main() {
	fdf::MapParser parser;
	std::vector<fdf::Vertex> vertices;
	fdf::Renderer renderer;

	try {
		// vertices = parser.parse("maps/42.fdf");
		renderer.init();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	for (const auto& vert : vertices) {
		vert.print();
	}

	renderer.importVertex(testVertices, 3, 3);
	renderer.loop();

	return 0;
}
