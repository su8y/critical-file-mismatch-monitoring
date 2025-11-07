#include "../odbc_alarm_repository.h"
#include <alarm_insert_client/alarm_insert_client.h>
#include "../utils.hpp"

std::string generate_alarm_id(const std::string& time, const std::string& uuid) { return "ALARM" + time + "_" + uuid; }

OdbcAlarmRepository::OdbcAlarmRepository(const std::string& hostname, const std::string& hostIp) :
	hostname_(hostname), hostIp_(hostIp)
{
}

std::string OdbcAlarmRepository::sendMajorAlarm(const std::string& reason)
{
	const std::string time = Utils::get_current_timestamp_str().c_str();
	const std::string uuid = Utils::generate_uuid_v4();
	const std::string newAlarmId = generate_alarm_id(time, uuid);

	insertIntoPackAlarmInfo(newAlarmId.c_str(), reason.c_str(), "F", "MAJOR", hostname_.c_str(), hostIp_.c_str(),
							reason.c_str(), "0", time.c_str());
	return newAlarmId;
}

void OdbcAlarmRepository::sendClearAlarm(const std::string& reason, const std::string& alarmId)
{
	const std::string time = Utils::get_current_timestamp_str().c_str();
	const std::string uuid = Utils::generate_uuid_v4();
	const std::string newAlarmId = generate_alarm_id(time, uuid);

	insertIntoPackAlarmInfo(newAlarmId.c_str(), reason.c_str(), "F", "CLEARED", hostname_.c_str(), hostIp_.c_str(), "",
							alarmId.c_str(), time.c_str());
}
