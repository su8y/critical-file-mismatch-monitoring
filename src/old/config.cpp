#include "../config.h"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include "mis_mon.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <unistd.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <unistd.h> 

typedef int SOCKET;
const int INVALID_SOCKET = -1;
const int SOCKET_ERROR = -1;
#define closesocket(s) close(s) // closesocket()을 close()로 매핑
#endif

void initLogger(std::string application)
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

	auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(server.logfile, // 기본 파일 이름
																		 0, 0, // 0시 0분마다 회전
																		 false, // 파일 압축하지 않음
																		 7 // 최대 7일치 보관
	);
	file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

	// === 여러 sink 결합 ===
	std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
	auto logger = std::make_shared<spdlog::logger>(application, sinks.begin(), sinks.end());
	spdlog::register_logger(logger);

	// === 기본 설정 ===
	spdlog::set_default_logger(logger);
	spdlog::set_level(spdlog::level::debug); // trace, debug, info, warn, err, critical, off
	spdlog::flush_on(spdlog::level::info);
}

void initDefaultServerConfig()
{
	char tempHostname[256];
	if (gethostname(tempHostname, sizeof(tempHostname)) == -1)
	{
		throw std::runtime_error("Failed to get hostname");
	}
	server.hostname = tempHostname;

#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}
	struct addrinfo* result = NULL;
	struct addrinfo* ptr = NULL;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	if (getaddrinfo(server.hostname.c_str(), NULL, &hints, &result) != 0)
	{
		WSACleanup();
		throw std::runtime_error("getaddrinfo failed");
	}

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		if (ptr->ai_family == AF_INET)
		{
			struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)ptr->ai_addr;
			char ip_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, ip_str, INET_ADDRSTRLEN);
			server.hostIp = ip_str;
			break;
		}
	}
	freeaddrinfo(result);
	WSACleanup();
#else
	struct ifaddrs *addrs, *tmp_addrs;
	bool ipFound = false;
	const std::vector<std::string> targetInterfaces = {"enp0s3", "ens192", "eth0"};

	getifaddrs(&addrs);
	tmp_addrs = addrs;

	while (tmp_addrs)
	{
		if (tmp_addrs->ifa_addr && tmp_addrs->ifa_addr->sa_family == AF_INET)
		{
			std::string ifName = tmp_addrs->ifa_name;
			for (const auto& targetName : targetInterfaces)
			{
				if (ifName == targetName)
				{
					struct sockaddr_in* sa = (struct sockaddr_in*)tmp_addrs->ifa_addr;
					server.hostIp = inet_ntoa(sa->sin_addr);
					ipFound = true;
					break;
				}
			}
		}
		if (ipFound)
			break;
		tmp_addrs = tmp_addrs->ifa_next;
	}

	freeifaddrs(addrs);

	if (!ipFound)
	{
		throw std::runtime_error("Failed to find IP address for target interfaces");
	}
#endif
}

void initMainConfig(void)
{
	std::ifstream file(server.configfile);
	if (!file.is_open())
	{
		spdlog::error("Failed to open server config file: {}", server.configfile);
		throw std::runtime_error("Failed to open server config file");
	}
	try
	{
		nlohmann::json mainJson = nlohmann::json::parse(file);
		auto& systemParams = mainJson.at("GIS").at("SystemParameters");

		std::string oss_addr1 = systemParams.at("OSS_ADDR").get<std::string>();
		if (!oss_addr1.empty())
		{
			server.ossAddresses.push_back(oss_addr1);
		}

		std::string oss_addr2 = systemParams.at("OSS_ADDR2").get<std::string>();
		if (!oss_addr2.empty() && isdigit(oss_addr2[0]))
		{
			server.ossAddresses.push_back(oss_addr2);
		}
	}
	catch (const nlohmann::json::exception& e)
	{
		throw std::runtime_error(fmt::format("Failed to parse main config: {}", e.what()));
	}
}

void initMisMonServer(void)
{
	initLogger("mis_mon");
	initDefaultServerConfig();
	initMainConfig();
}
