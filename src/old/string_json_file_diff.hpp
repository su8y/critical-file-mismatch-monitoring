#ifndef MISMATCHMONITOR_STRING_FILE_DIFF_HPP
#define MISMATCHMONITOR_STRING_FILE_DIFF_HPP
#include <cstring>
#include <stdexcept>
#include <string>
#include <sstream>

const std::string& DIFF_TOOLS = "/home/gis/MM/bin/flatten_json_diff";

class StringJsonFileDiff
{
public:
	/**
	 *
	 * @param actual 원본 Json File
	 * @param expect 비교 대상 Json File
	 * @return 변경 내용에 대한 Summary String
	 */
	static std::string compareJsonBoth(std::string actual, std::string expect)
	{
		const std::string command = std::string(DIFF_TOOLS) + " \"" + actual + "\" \"" + expect + "\"";

		FILE* fp = popen(command.c_str(), "r");
		if (fp == nullptr)
		{
			throw std::runtime_error("Failed popen() " + std::string(strerror(errno)));
		}

		std::stringstream diffOutputStream;
		char lineBuffer[256];
		while (fgets(lineBuffer, sizeof(lineBuffer), fp) != nullptr)
		{
			diffOutputStream << lineBuffer;
		}


		if (pclose(fp) == -1)
		{
			throw std::runtime_error("Failed pclose() " + std::string(strerror(errno)));
		}

		return diffOutputStream.str();
	}
};


#endif
