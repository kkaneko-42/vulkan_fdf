#ifndef FDF_MAP_PARSER_HPP
#define FDF_MAP_PARSER_HPP

#include "DataStructure.hpp"
#include <vector>
#include <string>
#include <stdexcept>
#include <cctype>

namespace fdf
{
	class MapParser
	{
	public:
		typedef std::vector< std::vector<fdf::Vertex> > ParseResult;
		
		ParseResult parse(
			const std::string& filename
		);

		static const std::string kValidMapExtension;

	private:
		std::vector<char> _fileContent;
		std::vector<fdf::Vertex> _vertices;
		size_t _cursorIndex;
		size_t _currentRow;
		size_t _currentCol;

		char getCurrentChar();
		void cursorNext();
		std::vector<fdf::Vertex> parseRow();
		void vertex();
		void delim();
		void newline();
		int digit();
		uint32_t hexDigit();

		static bool isHexDigit(char c);
	};
}

#endif
