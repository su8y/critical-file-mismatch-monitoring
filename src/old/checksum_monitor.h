#ifndef CHECKSUM_MONITOR_H
#define CHECKSUM_MONITOR_H

#include <memory>
#include "../config_repository.h"
#include "../mismatch_status.h"
#include "alarm_repository.h"

class ChecksumMonitor {
public:
    ChecksumMonitor(AlarmRepository& alarmRepository,
                    ConfigRepository& configRepository,
                    std::shared_ptr<MismatchStatus> mismatchStatus);

    void check();
    [[noreturn]] void schedule(int intervalSeconds);

private:
    AlarmRepository& alarm_repository_;
    ConfigRepository& config_repository_;
    std::shared_ptr<MismatchStatus> mismatch_status_;
};

#endif // CHECKSUM_MONITOR_H
