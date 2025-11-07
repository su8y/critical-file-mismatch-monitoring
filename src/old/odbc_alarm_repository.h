#ifndef ODBC_ALARM_REPOSITORY_H
#define ODBC_ALARM_REPOSITORY_H

#include "alarm_repository.h"

class OdbcAlarmRepository : public AlarmRepository {
public:
    OdbcAlarmRepository(const std::string& hostname, const std::string& hostIp);
    std::string sendMajorAlarm(const std::string& reason) override;
    void sendClearAlarm(const std::string& reason, const std::string& alarmId) override;
private:
    std::string hostname_;
    std::string hostIp_;
};

#endif // ODBC_ALARM_REPOSITORY_H
