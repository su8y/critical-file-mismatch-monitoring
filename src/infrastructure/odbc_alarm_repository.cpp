#include "odbc_alarm_repository.h"
#include <alarm_insert_client/alarm_insert_client.h>

#include "../support/utils.hpp"

std::string generate_alarm_id(const std::string& time, const std::string& uuid)
{
	return "ALARM" + time + "_" + uuid;
}

std::string AlarmRepository::insert_alarm(const std::string& alarmCode,
                                          const std::string& alarmLevel,
                                          const std::string& alarmContent) const
{
	const std::string time = Utils::get_current_timestamp_str().c_str();
	const std::string uuid = Utils::generate_uuid_v4();

	const std::string newAlarmId = generate_alarm_id(time, uuid);

	insertIntoPackAlarmInfo(newAlarmId.c_str(),
	                        alarmCode.c_str(),
	                        "F",
	                        alarmLevel.c_str(),
	                        this->hostname.c_str(),
	                        this->hostIp.c_str(),
	                        alarmContent.c_str(),
	                        "0",
	                        time.c_str()
	);
	return newAlarmId;
}

std::string AlarmRepository::clear_alarm(const std::string& alarmId,
                                         const std::string& alarmCode) const
{
	const std::string time = Utils::get_current_timestamp_str().c_str();
	const std::string uuid = Utils::generate_uuid_v4();

	const std::string newAlarmId = generate_alarm_id(time, uuid);

	insertIntoPackAlarmInfo(newAlarmId.c_str(),
	                        alarmCode.c_str(),
	                        "F",
	                        "CLEARED",
	                        this->hostname.c_str(),
	                        this->hostIp.c_str(),
	                        "",
	                        alarmId.c_str(),
	                        time.c_str());
	return newAlarmId;
}
