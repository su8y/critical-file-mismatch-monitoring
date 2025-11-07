#ifndef MISMATCHMONITOR_APPCONFIG_H
#define MISMATCHMONITOR_APPCONFIG_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct MonitorConfig {
	std::string type;
	std::string target;
	std::string target_type;
	std::string target_reference;
};

struct AppConfig {
	std::vector<MonitorConfig> monitors;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MonitorConfig, type, target, target_type, target_reference)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AppConfig, monitors)

#endif // MISMATCHMONITOR_APPCONFIG_H
