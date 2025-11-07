#ifndef MISMATCHMONITOR_MD5SUM_DIFF_HPP
#define MISMATCHMONITOR_MD5SUM_DIFF_HPP
#include <array>
#include <memory>
#include <string>


inline std::string get_md5sum(const std::string& file_path)
{
	std::array<char, 128> buffer{};
	std::string result;

	// 터미널에서 md5sum 실행
	std::string command = "md5sum \"" + file_path + "\" 2>/dev/null";
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

	if (!pipe)
		throw std::runtime_error("popen() failed to run md5sum");

	// 결과 읽기
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
	{
		result += buffer.data();
	}

	// 결과 형식: "<hash>  <filename>"
	auto pos = result.find(' ');
	if (pos != std::string::npos)
		result = result.substr(0, pos);

	return result;
}
std::string compare_md5sum(const std::string& ref_file, const std::string& target_file)
{
	try
	{
		std::string ref_hash = get_md5sum(ref_file);
		std::string target_hash = get_md5sum(target_file);

		if (ref_hash != target_hash)
		{
			return ref_hash + " -> " + target_hash; // 변경된 해시 문자열 반환
		}
		else
		{
			return ""; // 동일한 경우
		}
	}
	catch (const std::exception& e)
	{
		return std::string("Error: ") + e.what();
	}
}

#endif // MISMATCHMONITOR_MD5SUM_DIFF_HPP
