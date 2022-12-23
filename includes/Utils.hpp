#ifndef FDF_UTILS_HPP
#define FDF_UTILS_HPP

#include <vector>
#include <string>
#include <fstream>

namespace fdf
{
	std::vector<char> readFile(const std::string& filename);
}

#endif
