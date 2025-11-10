#include "md5sum_diff.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <nlohmann/json.hpp>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "flatten_json_diff.hpp"

const std::string DEFAULT_CONFIG_PATH = "/var/MM/setup.json";

/* Server Properties (require) */
std::string server_hostname;
std::string server_logfile = "logs/app.out";
std::map<std::string, nlohmann::json> checksum_status; // file-name

/* Alarm Constant Properties */
#define META_FILE "/home/gis/config/meta_data.json"
#define GOLDEN_FILE "/home/gis/config/Current_GoldenParameter.json"

#define ALARM_STATUS_FILE "/home/gis/MM/resources/backup_alarm.json"
typedef struct alarm_status {
  std::string alarmId;
  std::string alarmLevel;
  std::string alarmContent;
} alarm_status;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(alarm_status, alarmId, alarmLevel, alarmContent)

const std::string alarm_type = "F";
const std::string alarm_severity_major = "MAJOR";
const std::string alarm_severity_clear = "CLEAR";
std::string alarm_host_ip;
std::string snmp_oss_address_1;
std::string snmp_oss_address_2;

std::map<std::string, alarm_status> alarm_status_map;

/* Sentinel Type*/
typedef struct sentinel {
  std::string checkfile;
  std::string reffile;
  std::string file_type;    // listfile or file
  std::string compare_type; // checksum or json
  std::string compare_trigger_msg;
  std::string compare_time; // crontab
} sentinel;

std::vector<sentinel> jobs;

void initAlarm(const std::string metafile, const std::string goldenfile) {

  /* SNMP Address Setting*/
  nlohmann::json gf_json;
  std::ifstream gf(goldenfile);
  if (!gf.is_open())
    throw std::runtime_error("Failed to open file {}" + goldenfile);
  gf >> gf_json;
  gf_json.at("GIS")
      .at("SystemParameters")
      .at("OSS_ADDR")
      .get_to(snmp_oss_address_1);
  gf_json.at("GIS")
      .at("SystemParameters")
      .at("OSS_ADDR2")
      .get_to(snmp_oss_address_2);
  gf.close();

  /* hostip */
  if (server_hostname.empty()) {
    throw std::runtime_error(
        "Server hostname is empty. Please setting before this line.");
  }

  nlohmann::json meta_json;
  std::ifstream meta(metafile);
  if (!meta.is_open())
    throw std::runtime_error("Failed to open file {}" + metafile);
  meta >> meta_json;
  meta_json.at(server_hostname).at("VMHost").get_to(alarm_host_ip);
  meta.close();
}

void initServer(const std::string configfile) {
  /* Hostname setting */
  char hostname[256];
  std::memset(hostname, 0, sizeof(hostname));

  if (gethostname(hostname, sizeof(hostname)) != 0)
    throw std::runtime_error("Failed to get hostname");
  server_hostname = hostname;

  /* Setup config*/
  std::ifstream file(configfile);
  if (!file.is_open()) {
    // spdlog::error("Failed to open server config file: {}", configfile);
    throw std::runtime_error("Failed to open server config file");
  }
  nlohmann::json j;
  file >> j;

  if (j.contains("jobs") && j["jobs"].is_array()) {
    for (const auto &item : j["jobs"]) {
      sentinel s;

      /* 필수 값 */
      s.checkfile = item["target"].get<std::string>();
      s.reffile = item["target_reference"].get<std::string>();
      s.file_type = item["target_type"].get<std::string>();
      s.compare_type = item["type"].get<std::string>();
      s.compare_time = item["cron"].get<std::string>();
      if (item.contains("msg"))
        s.compare_trigger_msg = item["msg"].get<std::string>();
      else
        s.compare_trigger_msg = "";

      jobs.push_back(s);
    }
  }
}

void printInfo() {
  for (int i = 0; i < jobs.size(); i++) {
    spdlog::info("jobs({}): {} {} {} {} {}", i, jobs[i].checkfile,
                 jobs[i].reffile, jobs[i].file_type, jobs[i].compare_type,
                 jobs[i].compare_time);
  }
  spdlog::info("Alarm: {} {} {} {}", alarm_type, alarm_severity_major,
               alarm_severity_clear, alarm_host_ip);
  spdlog::info("Server: {}", server_hostname);
}

void initLogger(std::string application) {
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
  console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
  console_sink->set_level(spdlog::level::debug);

  auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
      server_logfile, // 기본 파일 이름
      0, 0,           // 0시 0분마다 회전
      false,          // 파일 압축하지 않음
      7               // 최대 7일치 보관
  );
  file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
  file_sink->set_level(spdlog::level::info);

  // === 여러 sink 결합 ===
  std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
  auto logger =
      std::make_shared<spdlog::logger>(application, sinks.begin(), sinks.end());
  spdlog::register_logger(logger);

  // === 기본 설정 ===
  spdlog::set_default_logger(logger);
  spdlog::set_level(
      spdlog::level::debug); // trace, debug, info, warn, err, critical, off
  spdlog::flush_on(spdlog::level::info);
}
/* 모니터링 */
std::string check_file(const sentinel &s) {
  std::string output;
  if (s.compare_type == "json")
    output = json_diff_process(s.reffile, s.checkfile);
  else if (s.compare_type == "checksum")
    output = compare_md5sum(s.reffile, s.checkfile);
  return output;
}

void sentinel_process(const sentinel &s) {
  spdlog::debug("Sentinel Process: {} {} {} {} {} {}", s.checkfile, s.reffile,
                s.file_type, s.compare_type, s.compare_trigger_msg,
                s.compare_time);

  if (s.file_type == "listfile") {
    if (s.compare_type == "json")
      throw std::runtime_error("json type is not supported for listfile");
    if (!std::filesystem::exists(s.reffile)) {
      nlohmann::json j;
      /* 기존 파일 읽어서 새로운 참조 파일 만들기 */
      std::ifstream checkfile(s.checkfile);
      std::string line;
      while (std::getline(checkfile, line))
        j[line] = get_md5sum(line);
      checkfile.close();
      std::ofstream ref_file_new(s.reffile);
      ref_file_new << j.dump(4);
      ref_file_new.close();
      checksum_status[s.checkfile] = j;
    }
    if (checksum_status.count(s.checkfile) == 0) {
      std::ifstream checkfile(s.reffile);
      nlohmann::json j;
      checkfile >> j;
      checksum_status[s.checkfile] = j;
    }
    std::ifstream listfile(s.checkfile);

    if (!listfile.is_open())
      spdlog::error("Failed to open file {}", s.checkfile);
    std::string line;
    std::stringstream output;
    while (std::getline(listfile, line)) {
      std::string md5sum = get_md5sum(line);
      std::string prev_md5sum = checksum_status[s.checkfile][line];
      if (md5sum != prev_md5sum) {
        /* append diff result */
        output << line << ":" << prev_md5sum << "->" << md5sum << std::endl;
      }
    }
    if (!output.str().empty())
      spdlog::warn("list file result: {}", output.str());
  } else if (s.file_type == "file") {
    std::string diffResult = check_file(s);
    /* status 업데이트 한번 올렸으면 다시 올리지 않는다. */
    /* 알람 발생 */
    if (!diffResult.empty())
      spdlog::warn("Sentinel Diff Result: {}", diffResult);
  }
}

void save_alarm_status_map(std::string backupfile) {
  json j = json::object(); // 기본적으로 빈 JSON 객체 생성

  for (const auto &[key, value] : alarm_status_map)
    j[key] = value; // NLOHMANN 매크로 덕분에 자동 직렬화
  std::ofstream file(backupfile);
  file << j.dump(4);
  file.close();
}
void load_alarm_status_map() {
  std::ifstream file(ALARM_STATUS_FILE);
  if (!file.is_open()) {
    spdlog::error("Failed to open file {}", ALARM_STATUS_FILE);
    return;
  }
  nlohmann::json j;
  file >> j;
  if (!j.is_object())
    throw std::runtime_error("Invalid json format");

  for (auto &[key, value] : j.items()) {
    try {
      alarm_status status = value.get<alarm_status>();
      alarm_status_map[key] = status;
      spdlog::debug("Load alarm[{}] -> alarmLevel: {}, alarmId: {}, alarmContent: {}", key,
                    status.alarmLevel, status.alarmId, status.alarmContent);
    } catch (const std::exception &e) {
      spdlog::error("Failed to parse alarm status file: {}", e.what());
    }
  }
}

int main(int argc, char *argv[]) {
  try {
    std::string config_path = DEFAULT_CONFIG_PATH;
    if (argc > 2) {
      if (strcmp(argv[1], "--config") == 0)
        config_path = argv[2];
    }
    initLogger("sentinel");
    initServer(config_path);
    initAlarm(META_FILE, GOLDEN_FILE);
    printInfo();

    while (true) {
      for (int i = 0; i < jobs.size(); i++)
        sentinel_process(jobs[i]);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  } catch (const std::exception &e) {
    std::cerr << "Critical Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
