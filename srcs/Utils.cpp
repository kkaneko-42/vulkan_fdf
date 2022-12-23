#include "Utils.hpp"

std::vector<char> fdf::readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file");
	}

	std::size_t fileSize = file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}
