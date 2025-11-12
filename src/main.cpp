#include "md5sum_diff.hpp"
#include "utils/CommonUtil.hpp"
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

#include "alarm_insert_client.h"
#include "flatten_json_diff.hpp"
#include "snmp_trap_client.hpp"
/* alarm_insert_client */
const char *ALARM_INSERT_SCRIPT_PATH = "/home/gis/alarm_insert/bin/alarmInsert";
const char *FAILED_QUERY_FILE = "/home/gis/MM/failed_queries.log";

const std::string DEFAULT_CONFIG_PATH = "/var/MM/setup.json";

/* Server Properties (require) */
std::string server_hostname;
std::string server_logfile = "logs/app.out";
std::map<std::string, nlohmann::json> checksum_status; // file-name

/* Alarm Constant Properties */
#define ALARM_MAJOR_INT_VALUE 2
#define ALARM_NORMAL_INT_VALUE 5
#define ALARM_OCCURRED_INT_VALUE 1
#define ALARM_RELEASE 0

#define META_FILE "/home/gis/config/meta_data.json"
#define GOLDEN_FILE "/home/gis/config/Current_GoldenParameter.json"

#define ALARM_STATUS_FILE "/home/gis/MM/resources/backup_alarm.json"
typedef struct alarm_status {
	std::string alarmId;
	std::string alarmLevel;
	std::string alarmContent;
} alarm_status;

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(alarm_status, alarmId, alarmLevel,
								   alarmContent)

const std::string alarm_type = "F";
const std::string alarm_severity_major = "MAJOR";
const std::string alarm_severity_clear = "CLEARED";
std::string alarm_host_ip;
std::string snmp_oss_address_1;
std::string snmp_oss_address_2;

std::map<std::string, alarm_status> alarm_status_map;

/* Sentinel Type*/
typedef struct sentinel {
	std::string checkfile;
	std::string reffile;
	std::string file_type;	  // listfile or file
	std::string compare_type; // checksum or json
	std::string compare_trigger_msg;
	std::string compare_time; // crontab
} sentinel;

std::vector<sentinel> jobs;

void print_usage() {
	std::cout
			<< R"( Usage: sentinel [--config <config_file_path>] [off <target_file>]
  Options: 
    --config <config_file_path>  Specify the path to the configuration file. (default: /var/MM/setup.json)
    off <target_file>            Disable alarm for the specified reference file by updating its target file.
    -h, --help                   Show this help message.
  )" << std::endl;
}

void save_alarm_status_map(std::string backupfile) {
	json j = json::object(); // 기본적으로 빈 JSON 객체 생성

	for (const auto &[key, value] : alarm_status_map)
		j[key] = value; // NLOHMANN 매크로 덕분에 자동 직렬화
	std::ofstream file(backupfile);
	file << j.dump(4);
	file.close();
}

void load_alarm_status_map(const std::string &backupfile) {
	std::ifstream file(backupfile);
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
			spdlog::debug("Load alarm[{}] -> alarmLevel: {}, alarmId: {}, "
						  "alarmContent: {}",
						  key, status.alarmLevel, status.alarmId,
						  status.alarmContent);
		} catch (const std::exception &e) {
			spdlog::error("Failed to parse alarm status file: {}", e.what());
		}
	}
}

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
			0, 0,			// 0시 0분마다 회전
			false,			// 파일 압축하지 않음
			7				// 최대 7일치 보관
	);
	file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
	file_sink->set_level(spdlog::level::info);

	// === 여러 sink 결합 ===
	std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
	auto logger = std::make_shared<spdlog::logger>(application, sinks.begin(),
												   sinks.end());
	spdlog::register_logger(logger);

	// === 기본 설정 ===
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug); // trace, debug, info, warn, err,
											 // critical, off
	spdlog::flush_on(spdlog::level::info);
}

/* 모니터링 (checksum or json)*/
std::string check_file(const sentinel &s) {
	std::string output;
	if (s.compare_type == "json")
		output = json_diff_process(s.reffile, s.checkfile);
	else if (s.compare_type == "checksum")
		output = compare_md5sum(s.reffile, s.checkfile);
	return output;
}

std::string create_alarm_id() {
	std::string time = CommonUtil::get_current_timestamp_str();
	std::string uuid = CommonUtil::generate_uuid_v4();
	return uuid + "_" + time;
}

void check_new_alarm(const sentinel &s) {
	if (alarm_status_map.count(s.checkfile) == 0) {
		alarm_status_map[s.checkfile] =
				alarm_status{.alarmId = "",
							 .alarmLevel = alarm_severity_clear,
							 .alarmContent = ""};
	}
}

void trigger_alarm(const sentinel &s, const alarm_status &status,
				   const std::string &prevAlarmId) {

	// s.compare_trigger_msg "_"로 스플릿해서 그 뒤에 코드 추출
	auto it = s.compare_trigger_msg.find_last_of('_');

	if (it == std::string::npos) {
		spdlog::error("Invalid compare_trigger_msg format: {}",
					  s.compare_trigger_msg);
		return;
	}
	std::string alarm_name = s.compare_trigger_msg.substr(0, it);
	std::string alarm_code = s.compare_trigger_msg.substr(it + 1);
	std::string insertAlarmMessage =
			"ALARM " + alarm_code + " " + status.alarmLevel + " " + alarm_name +
			" " + "ALARM(" + status.alarmLevel + "): " + status.alarmContent;

	spdlog::debug("alarm tirggered: alarm_insert {} {} {} {} {} {} {} {} {}",
				  status.alarmId, alarm_code, alarm_name, status.alarmLevel,
				  server_hostname, alarm_host_ip, insertAlarmMessage,
				  prevAlarmId, CommonUtil::get_current_timestamp_str());
	insertIntoPackAlarmInfo(status.alarmId.c_str(), alarm_code.c_str(),
							alarm_name.c_str(), status.alarmLevel.c_str(),
							server_hostname.c_str(), alarm_host_ip.c_str(),
							insertAlarmMessage.c_str(), prevAlarmId.c_str(),
							CommonUtil::get_current_timestamp_str().c_str());

	int alarm_status = status.alarmLevel == alarm_severity_major
							   ? ALARM_OCCURRED_INT_VALUE
							   : ALARM_RELEASE;
	int alarmLevel_int_val = status.alarmLevel == alarm_severity_major
									 ? ALARM_MAJOR_INT_VALUE
									 : ALARM_NORMAL_INT_VALUE;
	FailedQueue::trapSend({snmp_oss_address_1, server_hostname, alarm_code,
						   alarm_name, alarmLevel_int_val, alarm_status,
						   alarmLevel_int_val, status.alarmContent,
						   CommonUtil::get_current_timestamp_str().c_str()});
}

void clear_alarm(const sentinel &s) {
	check_new_alarm(s);
	std::string old_alarm_id = alarm_status_map[s.checkfile].alarmId;

	std::string new_alarm_id = create_alarm_id();
	spdlog::info("Alarm cleared for file: {}, prev: {}, current: {}",
				 s.checkfile, alarm_status_map[s.checkfile].alarmId,
				 new_alarm_id);

	alarm_status_map[s.checkfile] = alarm_status{
			.alarmId = new_alarm_id,
			.alarmLevel = alarm_severity_clear,
			.alarmContent = "",
	};
	trigger_alarm(s, alarm_status_map[s.checkfile], old_alarm_id);
	save_alarm_status_map(ALARM_STATUS_FILE);
}

void update_alarm(const sentinel &s, const std::string &diffResult) {
	check_new_alarm(s);

	if (alarm_status_map[s.checkfile].alarmLevel == alarm_severity_clear) {
		std::string new_alarm_id = create_alarm_id();

		alarm_status_map[s.checkfile] = alarm_status{
				.alarmId = new_alarm_id,
				.alarmLevel = alarm_severity_major,
				.alarmContent = diffResult,
		};
		trigger_alarm(s, alarm_status_map[s.checkfile], "0");

	} else if (alarm_status_map[s.checkfile].alarmLevel ==
					   alarm_severity_major &&
			   alarm_status_map[s.checkfile].alarmContent != diffResult) {
		// 기존 알람이 있지만 내용이 다른 경우
		clear_alarm(s);
		std::string new_alarm_id = create_alarm_id();

		// 알람 발생
		alarm_status_map[s.checkfile] = alarm_status{
				.alarmId = new_alarm_id,
				.alarmLevel = alarm_severity_major,
				.alarmContent = diffResult,
		};

		trigger_alarm(s, alarm_status_map[s.checkfile], "0");
	}
	save_alarm_status_map(ALARM_STATUS_FILE);
}

void sentinel_process(const sentinel &s) {
	spdlog::trace("Sentinel Process: {} {} {} {} {} {}", s.checkfile, s.reffile,
				  s.file_type, s.compare_type, s.compare_trigger_msg,
				  s.compare_time);

	if (s.file_type == "listfile") {
		// FIXME: listfile off 지원 필요
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
				output << line << ":" << prev_md5sum << "->" << md5sum
					   << std::endl;
			}
		}

		// 기 저장된 데이터가 없는 경우 새로 추가
		if (alarm_status_map.count(s.checkfile) == 0) {
			alarm_status_map[s.checkfile] =
					alarm_status{.alarmId = "",
								 .alarmLevel = alarm_severity_clear,
								 .alarmContent = ""};
		}

		if (output.str().empty() &&
			alarm_status_map[s.checkfile].alarmLevel == alarm_severity_major) {
			clear_alarm(s);
		} else if (!output.str().empty()) {
			update_alarm(s, output.str());
		}
	} else if (s.file_type == "file") {
		std::string diffResult = check_file(s);
		/* 알람 발생 */
		if (diffResult.empty() &&
			alarm_status_map[s.checkfile].alarmLevel == alarm_severity_major) {
			clear_alarm(s);
		} else if (!diffResult.empty()) {
			update_alarm(s, diffResult);
		}
	}
}

int main(int argc, char *argv[]) {
	try {
		std::string config_path = DEFAULT_CONFIG_PATH;
		if (argc <= 1 || strcmp(argv[1], "-h") == 0 ||
			strcmp(argv[1], "--help") == 0) {
			print_usage();
			return 0;
		}

		if (argc > 2) {
			if (strcmp(argv[1], "--config") == 0)
				config_path = argv[2];
		}

		initLogger("sentinel");
		initServer(config_path);
		initAlarm(META_FILE, GOLDEN_FILE);
		load_alarm_status_map(ALARM_STATUS_FILE);
		printInfo();

		if (argc > 4) {
			if (strcmp(argv[3], "off") == 0) {
				for (int i = 0; i < jobs.size(); i++) {
					if (jobs[i].checkfile == argv[4]) {
						std::ifstream src(jobs[i].reffile);
						std::ofstream dst(jobs[i].checkfile);
						dst << src.rdbuf();
						src.close();
						dst.close();
						exit(0);
					}
				}
			}
		}

		// TODO: Cron 표현식에 따른 스케줄링 필요
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
