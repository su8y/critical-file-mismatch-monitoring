#include "../golden_parameter_monitor.h"
#include <spdlog/spdlog.h>
#include <thread>
#include "../string_json_file_diff.hpp"

GoldenParameterMonitor::GoldenParameterMonitor(AlarmRepository& alarmRepository,
                                             ConfigRepository& configRepository,
                                             std::shared_ptr<MismatchStatus> mismatchStatus)
    : alarm_repository_(alarmRepository),
      config_repository_(configRepository),
      mismatch_status_(mismatchStatus) {}

void GoldenParameterMonitor::check() const {
    const std::string goldenParameterFile = config_repository_.getGoldenParameterFile();
    const std::string referenceFile = config_repository_.getReferenceFile();

    if (mismatch_status_->isGoldenParameterMismatch()) {
        if (StringJsonFileDiff::compareJsonBoth(goldenParameterFile, referenceFile).empty()) {
            spdlog::info("Golden parameter mismatch cleared");
            std::string alarmId = mismatch_status_->getAlarmId("GoldenParameterMismatch");
            alarm_repository_.sendClearAlarm("GoldenParameterMismatch", alarmId);
            mismatch_status_->setGoldenParameterMismatch(false);
            mismatch_status_->setAlarmId("GoldenParameterMismatch", "");
        }
    } else {
        if (!StringJsonFileDiff::compareJsonBoth(goldenParameterFile, referenceFile).empty()) {
            spdlog::warn("Golden parameter mismatch detected");
            std::string alarmId = alarm_repository_.sendMajorAlarm("GoldenParameterMismatch");
            mismatch_status_->setGoldenParameterMismatch(true);
            mismatch_status_->setAlarmId("GoldenParameterMismatch", alarmId);
        }
    }
}

void GoldenParameterMonitor::schedule(const int intervalSeconds) const{
    while (true) {
        check();
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
    }
}
