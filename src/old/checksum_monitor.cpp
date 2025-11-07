#include "../checksum_monitor.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <thread>

#include "../config_repository.h"
#include "../mismatch_status.h"
#include "../utils.hpp"
#include "alarm_repository.h"

ChecksumMonitor::ChecksumMonitor(AlarmRepository& alarmRepository, ConfigRepository& configRepository,
								 std::shared_ptr<MismatchStatus> mismatchStatus) :
	alarm_repository_(alarmRepository), config_repository_(configRepository), mismatch_status_(mismatchStatus)
{
}

void ChecksumMonitor::check()
{
	std::string checksumFile = config_repository_.getChecksumFile();
	std::ifstream file(checksumFile);
	if (!file.is_open())
	{
		spdlog::error("Failed to open checksum file: {}", checksumFile);
		return;
	}

	bool mismatch_found = false;
	std::string line;
	while (std::getline(file, line))
	{
		std::string currentChecksum = Utils::calculate_xor_checksum(line);
		if (mismatch_status_->getChecksum(line) != currentChecksum)
		{
			mismatch_found = true;
			if (mismatch_status_->getChecksum(line) != currentChecksum)
			{
				spdlog::warn("Checksum mismatch for file: {}", line);
				mismatch_status_->setChecksum(line, currentChecksum);
			}
		}
	}

	if (mismatch_found)
	{
		if (!mismatch_status_->isChecksumMismatch())
		{
			spdlog::warn("Checksum mismatch detected");
			std::string alarmId = alarm_repository_.sendMajorAlarm("ChecksumMismatch");
			mismatch_status_->setChecksumMismatch(true);
			mismatch_status_->setAlarmId("ChecksumMismatch", alarmId);
		}
	}
	else
	{
		if (mismatch_status_->isChecksumMismatch())
		{
			spdlog::info("Checksum mismatch cleared");
			std::string alarmId = mismatch_status_->getAlarmId("ChecksumMismatch");
			alarm_repository_.sendClearAlarm("ChecksumMismatch", alarmId);
			mismatch_status_->setChecksumMismatch(false);
			mismatch_status_->setAlarmId("ChecksumMismatch", "");
		}
	}
}

void ChecksumMonitor::schedule(const int intervalSeconds)
{
	while (true)
	{
		check();
		std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds));
	}
}
