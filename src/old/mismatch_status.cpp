#include "../mismatch_status.h"
#include <fstream>

MismatchStatus::MismatchStatus(const std::string& filePath) : file_path_(filePath) {}

bool MismatchStatus::load() {
    std::ifstream file(file_path_);
    if (!file.is_open()) {
        return false;
    }
    file >> status_;
    return true;
}

bool MismatchStatus::save() {
    std::ofstream file(file_path_);
    if (!file.is_open()) {
        return false;
    }
    file << status_.dump(4);
    return true;
}

bool MismatchStatus::isGoldenParameterMismatch() const {
    return status_.value("goldenParameterMismatch", false);
}

void MismatchStatus::setGoldenParameterMismatch(bool value) {
    status_["goldenParameterMismatch"] = value;
    save();
}

bool MismatchStatus::isChecksumMismatch() const {
    return status_.value("checksumMismatch", false);
}

void MismatchStatus::setChecksumMismatch(bool value) {
    status_["checksumMismatch"] = value;
    save();
}

std::string MismatchStatus::getChecksum(const std::string& filePath) const {
    return status_["checksums"].value(filePath, "");
}

void MismatchStatus::setChecksum(const std::string& filePath, const std::string& checksum) {
    status_["checksums"][filePath] = checksum;
    save();
}

std::string MismatchStatus::getAlarmId(const std::string& reason) const {
    return status_["alarms"].value(reason, "");
}

void MismatchStatus::setAlarmId(const std::string& reason, const std::string& alarmId) {
    status_["alarms"][reason] = alarmId;
    save();
}
