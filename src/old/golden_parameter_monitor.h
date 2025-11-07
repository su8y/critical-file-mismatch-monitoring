#ifndef GOLDEN_PARAMETER_MONITOR_H
#define GOLDEN_PARAMETER_MONITOR_H

#include <memory>
#include "../mismatch_status.h"
#include "alarm_repository.h"
#include "config_repository.h"

class GoldenParameterMonitor
{
public:
	GoldenParameterMonitor(AlarmRepository& alarmRepository, ConfigRepository& configRepository,
						   std::shared_ptr<MismatchStatus> mismatchStatus);

	void check() const;
	[[noreturn]] void schedule(int intervalSeconds) const;

private:
	AlarmRepository& alarm_repository_;
	ConfigRepository& config_repository_;
	std::shared_ptr<MismatchStatus> mismatch_status_;
};

#endif // GOLDEN_PARAMETER_MONITOR_H
