#ifndef MISMATCHMONITOR_FILEUTIL_H
#define MISMATCHMONITOR_FILEUTIL_H

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <sstream>
#include <fstream>

namespace file_util
{
	inline std::string read_file_as_string(const std::string& file_path)
	{
		std::ifstream file_stream(file_path);
		if (!file_stream.is_open()) {
			throw new std::runtime_error("Could not open file: " + file_path + "");
		}
		std::stringstream buffer;
		buffer << file_stream.rdbuf();
		return buffer.str();
	}
}

#endif // MISMATCHMONITOR_FILEUTIL_H
