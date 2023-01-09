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

static void translateVertices(
	std::vector<fdf::Vertex>& vertices,
	float translateX, float translateY)
{
	for (size_t i = 0; i < vertices.size(); ++i) {
		vertices[i].pos.x += translateX;
		vertices[i].pos.y += translateY;
	}
}

static void rescaleZ(
	std::vector<fdf::Vertex>& vertices,
	float rescaleRate)
{
	for (size_t i = 0; i < vertices.size(); ++i) {
		vertices[i].pos.z *= rescaleRate;
	}
}

int main() {
	fdf::MapParser parser;
	std::vector<fdf::Vertex> vertices;
	size_t row, col;
	fdf::Renderer renderer;

	try {
		auto parseResult = parser.parse("maps/42.fdf");
		vertices = parseResult.vertices;
		row = parseResult.row;
		col = parseResult.col;
		renderer.init();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	translateVertices(vertices, -(float)col / 2, -(float)row / 2);
	rescaleZ(vertices, -0.2);
	renderer.importVertex(vertices, row, col);
	renderer.loop();

	return 0;
}
