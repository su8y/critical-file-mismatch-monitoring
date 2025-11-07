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
#include <unistd.h> // gethostname
#endif

#include <nlohmann/json.hpp>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

const std::string DEFAULT_CONFIG_PATH = "/var/MM/setup.json";

/* Server Properties (require) */
std::string server_hostname;
std::string server_logfile = "logs/app.out";

/* Alarm Constant Properties */
#define META_FILE "/home/gis/config/meta_data.json"
#define GOLDEN_FILE "/home/gis/config/Current_GoldenParameter.json"
const std::string alarm_type = "F";
const std::string alarm_severity_major = "MAJOR";
const std::string alarm_severity_clear = "CLEAR";
std::string alarm_host_ip;
std::string snmp_oss_address_1;
std::string snmp_oss_address_2;

/* Sentinel Type*/
typedef struct sentinel
{
	std::string checkfile;
	std::string reffile;
	std::string file_type; // listfile or file
	std::string compare_type; // checksum or json
	std::string compare_trigger_msg;
	std::string compare_time; // crontab
} sentinel;

std::vector<sentinel> jobs;


void initAlarm(const std::string metafile, const std::string goldenfile)
{

	/* SNMP Address Setting*/
	nlohmann::json gf_json;
	std::ifstream gf(goldenfile);
	if (!gf.is_open())
	{
		throw std::runtime_error("Failed to open file {}" + goldenfile);
	}
	gf >> gf_json;
	gf_json.at("GIS").at("SystemParameters").at("OSS_ADDR").get_to(snmp_oss_address_1);
	gf_json.at("GIS").at("SystemParameters").at("OSS_ADDR2").get_to(snmp_oss_address_2);
	gf.close();

	/* hostip */
	if (server_hostname.empty())
	{
		throw std::runtime_error("Server hostname is empty. Please setting before this line.");
	}

	nlohmann::json meta_json;
	std::ifstream meta(metafile);
	if (!meta.is_open())
	{
		throw std::runtime_error("Failed to open file {}" + metafile);
	}
	meta >> meta_json;
	meta_json.at(server_hostname).at("VMHost").get_to(alarm_host_ip);
	meta.close();
}

void initServer(const std::string configfile)
{
	/* Hostname setting */
	char hostname[256];
	std::memset(hostname, 0, sizeof(hostname));

	if (gethostname(hostname, sizeof(hostname)) != 0)
	{
		throw std::runtime_error("Failed to get hostname");
	}
	server_hostname = hostname;

	/* Setup config*/
	std::ifstream file(configfile);
	if (!file.is_open())
	{
		// spdlog::error("Failed to open server config file: {}", configfile);
		throw std::runtime_error("Failed to open server config file");
	}
	nlohmann::json j;
	file >> j;

	if (j.contains("jobs") && j["jobs"].is_array())
	{
		for (const auto& item : j["jobs"])
		{
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

void printInfo()
{
	for (int i = 0; i < jobs.size(); i++)
	{
		spdlog::info("jobs({}): {} {} {} {} {}", i, jobs[i].checkfile, jobs[i].reffile, jobs[i].file_type,
					 jobs[i].compare_type, jobs[i].compare_time);
	}
	spdlog::info("Alarm: {} {} {} {}", alarm_type, alarm_severity_major, alarm_severity_clear, alarm_host_ip);
	spdlog::info("Server: {}", server_hostname);
}
void initLogger(std::string application)
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
	console_sink->set_level(spdlog::level::debug);

	auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(server_logfile, // 기본 파일 이름
																		 0, 0, // 0시 0분마다 회전
																		 false, // 파일 압축하지 않음
																		 7 // 최대 7일치 보관
	);
	file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");
	file_sink->set_level(spdlog::level::info);

	// === 여러 sink 결합 ===
	std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
	auto logger = std::make_shared<spdlog::logger>(application, sinks.begin(), sinks.end());
	spdlog::register_logger(logger);

	// === 기본 설정 ===
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug); // trace, debug, info, warn, err, critical, off
	spdlog::flush_on(spdlog::level::info);
}

int main(int argc, char* argv[])
{
	try
	{
		std::string config_path = DEFAULT_CONFIG_PATH;
		if (argc > 2)
		{
			if (strcmp(argv[1], "--config") == 0)
				config_path = argv[2];
		}
		initLogger("sentinel");
		initServer(config_path);
		initAlarm(META_FILE, GOLDEN_FILE);

		printInfo();
	}
	catch (const std::exception& e)
	{
		std::cerr << "Critical Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
