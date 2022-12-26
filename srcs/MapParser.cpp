#include "MapParser.hpp"
#include "Utils.hpp"

std::vector<fdf::Vertex> fdf::MapParser::parse(
	const std::string& filename)
{
	_fileContent = fdf::readFile(filename);

	// 列数取得の為、第一行のパースはwhile外で
	_vertices.clear();
	parseRow();
	size_t rowLength = _vertices.size();

	while (getCurrentChar() != '\0') {
		_currentCol = 0;
		++_currentRow;

		parseRow();
		if (_vertices.size() % rowLength != 0) {
			// not complete tetragon
			throw std::runtime_error("not complete tetragon");
		}
	}

	return _vertices;
}

char fdf::MapParser::getCurrentChar() {
	if (_cursorIndex >= _fileContent.size()) {
		return '\0';
	}

	return _fileContent[_cursorIndex];
}

void fdf::MapParser::cursorNext() {
	++_cursorIndex;
}

void fdf::MapParser::parseRow() {
	delim();
	while (true) {
		if (getCurrentChar() == '\0') {
			break;
		} else if (getCurrentChar() == '\n') {
			newline();
			break;
		}

		vertex();
		delim();
	}
}

void fdf::MapParser::vertex() {
	fdf::Vertex vert;

	vert.x = _currentCol;
	vert.y = _currentRow;
	vert.z = static_cast<float>(digit());

	if (getCurrentChar() == ',') {
		cursorNext();
		vert.rgba = (hexDigit() << 8);
	}

	_vertices.push_back(vert);
	++_currentCol;
}

void fdf::MapParser::delim() {
	while (getCurrentChar() == ' ' || getCurrentChar() == '\t') {
		cursorNext();
	}
}

void fdf::MapParser::newline() {
	if (getCurrentChar() == '\n') {
		cursorNext();
		return;
	}

	// invalid newline code
	throw std::runtime_error("invalid newline");
}

int fdf::MapParser::digit() {
	int n = 0;
	int sgn = 1;

	if (getCurrentChar() == '-') {
		sgn = -1;
		cursorNext();
	}

	if (!std::isdigit(getCurrentChar())) {
		throw std::runtime_error("invalid digit");
	}

	while (std::isdigit(getCurrentChar())) {
		n = (n * 10) + getCurrentChar() - '0';
		cursorNext();
	}

	return sgn * n;
}

uint32_t fdf::MapParser::hexDigit() {
	if (getCurrentChar() == '0') {
		cursorNext();
		if (getCurrentChar() == 'x') {
			// parse error not found
			cursorNext();
			
			uint32_t n = 0;
			while (isHexDigit(getCurrentChar())) {
				if (std::isdigit(getCurrentChar())) {
					n = (n * 16) + getCurrentChar() - '0';
				}
				else {
					n = (n * 16) + getCurrentChar() - 'A' + 10;
				}
				
				cursorNext();
			}

			return n;
		}
	}

	// invalid hex such as "0x", "ffff00"
	throw std::runtime_error("invalid hex");
}

bool fdf::MapParser::isHexDigit(char c) {
	return (
		std::isdigit(c) ||
		('A' <= c && c <= 'F')
	);
}
/*
int main() {
	fdf::MapParser parser;
	std::vector<fdf::Vertex> result;

	try {
		result = parser.parse("maps/elem-col.fdf");
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	for (const auto& vertex : result) {
		vertex.print();
	}

	return 0;
}
*/