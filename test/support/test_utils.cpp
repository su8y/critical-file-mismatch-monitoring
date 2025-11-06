#include "gtest/gtest.h"
#include "support/utils.hpp" // CMake의 include_directories 덕분에 바로 include 가능

#include <string>
#include <regex>       // 형식 검증을 위해 regex 포함
#include <algorithm>   // std::all_of
#include <cctype>      // ::isdigit

// Utils 클래스의 테스트 스위트
TEST(UtilsTest, GenerateUUIDv4) {
    // 두 개의 UUID 생성
    std::string uuid1 = Utils::generate_uuid_v4();
    std::string uuid2 = Utils::generate_uuid_v4();

    // 1. 고유성 테스트: 두 UUID는 달라야 함
    ASSERT_NE(uuid1, uuid2);

    // 2. 형식(Format) 테스트: 정규식을 사용하여 UUID v4 형식(하이픈 없음)인지 확인
    // 형식: 32개의 16진수 문자
    // 버전 (13번째 문자): '4'
    // 변형 (17번째 문자): '8', '9', 'a', 'b' 중 하나
    const std::regex uuid_v4_regex(
        "^[0-9a-f]{12}4[0-9a-f]{3}[89ab][0-9a-f]{15}$"
    );

    // 2-1. 길이 및 형식 검증 (uuid1)
    ASSERT_EQ(uuid1.length(), 32);
    ASSERT_TRUE(std::regex_match(uuid1, uuid_v4_regex))
        << "UUID1 does not match v4 format: " << uuid1;

    // 2-2. 길이 및 형식 검증 (uuid2)
    ASSERT_EQ(uuid2.length(), 32);
    ASSERT_TRUE(std::regex_match(uuid2, uuid_v4_regex))
        << "UUID2 does not match v4 format: " << uuid2;
}

TEST(UtilsTest, GetCurrentTimestampStr) {
    std::string timestamp = Utils::get_current_timestamp_str();

    // 1. 길이 테스트: "YYYYMMDDHHMMSS"는 14자
    ASSERT_EQ(timestamp.length(), 14);

    // 2. 형식 테스트: 모든 문자가 숫자인지 확인
    bool all_digits = std::all_of(
        timestamp.begin(),
        timestamp.end(),
        [](char c){ return ::isdigit(c); }
    );
    ASSERT_TRUE(all_digits)
        << "Timestamp contains non-digit characters: " << timestamp;

    // (선택 사항) 더 엄격한 정규식 테스트
    // 예: 연도가 20xx로 시작하는지 등
    const std::regex ts_regex("^20\\d{12}$"); // 20으로 시작하는 14자리 숫자
    ASSERT_TRUE(std::regex_match(timestamp, ts_regex))
        << "Timestamp format is not 20xx...: " << timestamp;
}

TEST(UtilsTest, GetFormattedTimestamp) {
    // 테스트할 커스텀 포맷
    const char* format = "%Y-%m-%d";
    std::string date_str = Utils::get_formatted_timestamp(format);

    // 1. 길이 테스트: "YYYY-MM-DD"는 10자
    ASSERT_EQ(date_str.length(), 10);

    // 2. 형식 테스트: 정규식으로 "XXXX-XX-XX" 형식인지 확인
    const std::regex date_regex("^\\d{4}-\\d{2}-\\d{2}$");
    ASSERT_TRUE(std::regex_match(date_str, date_regex))
        << "Formatted date string is not YYYY-MM-DD: " << date_str;
}