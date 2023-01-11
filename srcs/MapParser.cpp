#include "MapParser.hpp"
#include "Utils.hpp"

fdf::MapParser::ParseResult fdf::MapParser::parse(
	const std::string& filename)
{
	_fileName = filename;
	_fileContent = fdf::readFile(_fileName);
	_currentRow = 0;
	_currentCol = 0;

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
			throw std::runtime_error(generateErrorMsg("not complete tetragon"));
		}
	}

	return ParseResult{ _vertices, _currentRow, _currentCol };
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

	vert.pos.x = _currentCol;
	vert.pos.y = _currentRow;
	vert.pos.z = static_cast<float>(digit());
	vert.rgba = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

	if (getCurrentChar() == ',') {
		cursorNext();
		vert.setRGBA((hexDigit() << 8) + 0x000000ff);
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
	throw std::runtime_error(generateErrorMsg("invalid newline"));
}

int fdf::MapParser::digit() {
	int n = 0;
	int sgn = 1;

	if (getCurrentChar() == '-') {
		sgn = -1;
		cursorNext();
	}

	if (!std::isdigit(getCurrentChar())) {
		throw std::runtime_error(generateErrorMsg("invalid digit found"));
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
			// parse error not found(correct map)
			
			uint32_t n = 0;
			
			while (true) {
				cursorNext();
				char c = getCurrentChar();

				if (std::isdigit(c)) {
					n = (n * 16) + c - '0';
				} else if ('a' <= c && c <= 'f') {
					n = (n * 16) + c - 'a' + 10;
				} else if ('A' <= c && c <= 'F') {
					n = (n * 16) + c - 'A' + 10;
				} else {
					break;
				}
			}

			return n;
		}
	}

	// invalid hex such as "0x", "ffff00"
	throw std::runtime_error(generateErrorMsg("invalid hex"));
}

std::string fdf::MapParser::generateErrorMsg(const std::string& msg) const
{
	std::string rowStr = std::to_string(_currentRow + 1);
	return _fileName + ":" + rowStr + ": error: " + msg;
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