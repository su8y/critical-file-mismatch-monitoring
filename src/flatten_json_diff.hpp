#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include <set>

using json = nlohmann::json;

// JSON의 기본 타입들을 std::string으로 변환해주는 헬퍼 함수
std::string to_string(const json &j) {
  if (j.is_string()) {
    return j.get<std::string>();
  } else if (j.is_number_integer()) {
    return std::to_string(j.get<long long>());
  } else if (j.is_number_unsigned()) {
    return std::to_string(j.get<unsigned long long>());
  } else if (j.is_number_float()) {
    return std::to_string(j.get<double>());
  } else if (j.is_boolean()) {
    return j.get<bool>() ? "true" : "false";
  } else if (j.is_null()) {
    return "null";
  }
  return j.dump();
}

// JSON을 flatten하게 만드는 재귀 함수
void flatten_json(const json &j, const std::string &prefix, std::map<std::string, json> &result) {
  if (j.is_object()) {
    for (auto it = j.begin(); it != j.end(); ++it) {
      std::string new_prefix = prefix.empty() ? it.key() : prefix + "." + it.key();
      flatten_json(it.value(), new_prefix, result);
    }
  } else if (j.is_array()) {
    for (size_t i = 0; i < j.size(); ++i) {
      std::string key = std::to_string(i);
      std::string new_prefix = prefix.empty() ? key : prefix + "[" + key + "]";
      flatten_json(j.at(i), new_prefix, result);
    }
  } else {
    result[prefix] = j;
  }
}

std::string json_diff_process(std::string file, std::string target) {
  try {
    json j1, j2;
    std::ifstream file1_stream(file);
    if (!file1_stream.is_open()) {
    	throw std::runtime_error("Failed to open file " + file);
    }
    j1 = json::parse(file1_stream);
    file1_stream.close();

    // 2. 두 번째 파일에서 JSON 읽기
    std::ifstream file2_stream(target);
    if (!file2_stream.is_open()) {
    	throw std::runtime_error("Failed to open file " + target);
    }
    j2 = json::parse(file2_stream);
    file2_stream.close();

    std::map<std::string, json> flattened_map1;
    flatten_json(j1, "", flattened_map1);

    std::map<std::string, json> flattened_map2;
    flatten_json(j2, "", flattened_map2);

    // 2. 변경사항을 찾기 위한 두 맵 비교
    std::stringstream output;

    // map1을 기준으로 변경/삭제된 항목 찾기
    for (const auto &pair1: flattened_map1) {
      const auto &key = pair1.first;
      const auto &value1 = pair1.second;

      auto it2 = flattened_map2.find(key);
      if (it2 != flattened_map2.end()) {
        // 키가 map2에도 존재하는 경우: 값 비교
        const auto &value2 = it2->second;
        if (value1 != value2) {
          output << "[CHANGED]_" << key << ":_PREV_VALUE=" << to_string(value1) << "_&_CURRENT_VALUE=" << to_string(value2) << std::endl;
        }
      } else {
        // 키가 map2에 없는 경우: 삭제됨
        output << "[REMOVED]_" << key << ":_" << to_string(value1) << std::endl;
      }
    }

    // map2를 기준으로 추가된 항목 찾기
    for (const auto &pair2: flattened_map2) {
      const auto &key = pair2.first;
      auto it1 = flattened_map1.find(key);
      if (it1 == flattened_map1.end()) {
        // 키가 map1에 없는 경우: 추가됨
        output << "[ADDED]___" << key << ":_" << to_string(pair2.second) << std::endl;
      }
    }

	return output.str();
  } catch (const json::parse_error &e) {
	throw e;
  }
}
