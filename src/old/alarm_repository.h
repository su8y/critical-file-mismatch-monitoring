#ifndef ALARM_REPOSITORY_H
#define ALARM_REPOSITORY_H

#include <string>

class AlarmRepository {
public:
    virtual ~AlarmRepository() = default;
    virtual std::string sendMajorAlarm(const std::string& reason) = 0;
    virtual void sendClearAlarm(const std::string& reason, const std::string& alarmId) = 0;
};

#endif // ALARM_REPOSITORY_H
