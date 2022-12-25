#ifndef FDF_DATA_STRUCTURE_HPP
#define FDF_DATA_STRUCTURE_HPP

#include <cstdint>
#include <iostream>

namespace fdf
{
	struct Vertex
	{
		float x, y, z;
		uint32_t rgba;

		// Vertex() : x(0), y(0), z(0), rgba(0x000000ff) {}

		void print() const {
			std::cout << "(";
			std::cout << x << ", ";
			std::cout << y << ", ";
			std::cout << z << ", ";
			std::cout << std::hex << rgba;
			std::cout << ")" << std::endl;
		}
	};
}

#endif
