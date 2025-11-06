#ifndef MISMATCHMONITOR_ALARM_REPOSITORY_H
#define MISMATCHMONITOR_ALARM_REPOSITORY_H

#include <string>

class AlarmRepository
{
public:
	explicit AlarmRepository(const std::string& hostname, const std::string& hostip);

	static AlarmRepository& getInstance()
	{
		static AlarmRepository instance;
		return instance;
	}

	void init(const std::string& hostname, const std::string& hostIp)
	{
		this->hostname = hostname;
		this->hostIp = hostIp;
	}

	/**
	 * 알람 내용 변경
	 * @param alarmId 변경할 알람 아이디
	 * @param alarmCode  변경할 알람 타입
	 * @param alarmLevel  변경할 알람 레벨
	 * @param alarmContent  변경할 알람 내용
	 * @return 변경된 알람 UUID
	 */
	std::string update_alarm(const std::string& alarmId,
	                         const std::string& alarmCode,
	                         const std::string& alarmLevel,
	                         const std::string& alarmContent) const
	{
		auto _ = clear_alarm(alarmId, alarmCode);
		std::string savedAlarmId = insert_alarm(alarmCode,
		                                        alarmLevel,
		                                        alarmContent);
		return savedAlarmId;
	};

	/**
	 * 알람 발생
	 * @param alarmCode 발생 시킬 알람 코드
	 * @param alarmLevel  발생 시킬 알람의 레벨
	 * @param alarmContent  발생 시킬 알람의 내용
	 * @return 생성된 알람 UUID
	 */
	std::string insert_alarm(const std::string& alarmCode,
	                         const std::string& alarmLevel,
	                         const std::string& alarmContent) const;

	/**
	 *
	 * 발생된 알람 클리어
	 * @param alarmId 클리어할 알람 아이디
	 * @param alarmCode  클리어하는 알람 타입
	 * @return  생성된 알람 UUID
	 */
	std::string clear_alarm(const std::string& alarmId,
	                        const std::string& alarmCode) const;

private:
	AlarmRepository() = default;
	std::string hostname;
	std::string hostIp;
};


#endif //MISMATCHMONITOR_ALARM_REPOSITORY_H
