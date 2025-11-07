#ifndef MISMATCHMONITOR_UTILS_H
#define MISMATCHMONITOR_UTILS_H

#include <chrono>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>

class Utils {
public:
	/**
	 * @brief C++ 스타일로 UUID v4 문자열을 생성합니다. (static)
	 */
	static std::string generate_uuid_v4() {
		// 1. 고품질 난수 생성기 (스레드별로 독립적)
		thread_local std::random_device rd;
		thread_local std::mt19937 gen(rd()); // Mersenne Twister 엔진

		std::uniform_int_distribution<int> dist(0, 15);
		std::uniform_int_distribution<int> dist_variant(8, 11);

		static constexpr char hex_chars[] = "0123456789abcdef";

		std::string uuid_str(32, ' ');
		for (int i = 0; i < 32; ++i) {
			uuid_str[i] = hex_chars[dist(gen)];
		}

		uuid_str[12] = '4'; // Version 4
		uuid_str[16] = hex_chars[dist_variant(gen)]; // Variant 10xx

		return uuid_str;
	}

	/**
	 * @brief 현재 시간을 "YYYYMMDDHHMMSS" 형식의 문자열로 반환합니다. (static)
	 */
	static std::string get_current_timestamp_str() {
		return get_formatted_timestamp("%Y%m%d%H%M%S");
	}

	/**
	 * @brief (추가됨) 원하는 포맷으로 현재 시간 문자열을 반환합니다. (static)
	 */
	static std::string get_formatted_timestamp(const char* format) {
		const auto now = std::chrono::system_clock::now();
		const auto now_c = std::chrono::system_clock::to_time_t(now);

		// C++ 스트림 사용 (localtime은 스레드 안전하지 않음에 유의)
		std::stringstream ss;
		ss << std::put_time(std::localtime(&now_c), format);
		return ss.str();
	}


	/*
	 * checksum
	 */
	static std::string calculate_xor_checksum(const std::string& filePath) {
		std::ifstream file(filePath, std::ios::binary);
		if (!file) {
			return "";
		}

		char buffer[1024];
		unsigned long checksum = 0;
		while (file.read(buffer, sizeof(buffer))) {
			for (int i = 0; i < file.gcount(); ++i) {
				checksum ^= buffer[i];
			}
		}
		for (int i = 0; i < file.gcount(); ++i) {
			checksum ^= buffer[i];
		}

		return std::to_string(checksum);
	}
};

#endif //MISMATCHMONITOR_UTILS_H