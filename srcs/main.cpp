#include "MapParser.hpp"
#include "Renderer.hpp"

const std::vector<fdf::Vertex> testVertices = {
	fdf::Vertex{0.5f, -0.5f, 5.0f, (uint32_t)0xffffffff},
	fdf::Vertex{0.5f, 0.5f, 1.0f, (uint32_t)0xffffffff},
	fdf::Vertex{-0.5f, 0.5f, 0.0f, (uint32_t)0xffffffff},
};

int main() {
	fdf::MapParser parser;
	std::vector<fdf::Vertex> vertices;
	fdf::Renderer renderer;

	try {
		vertices = parser.parse("maps/42.fdf");
		renderer.init();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	for (const auto& vert : vertices) {
		vert.print();
	}

	renderer.importVertex(vertices);
	renderer.loop();

	return 0;
}
