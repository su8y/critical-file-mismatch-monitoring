#ifndef MISMATCH_STATUS_H
#define MISMATCH_STATUS_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class MismatchStatus {
public:
    explicit MismatchStatus(const std::string& filePath);

    bool load();
    bool save();

    bool isGoldenParameterMismatch() const;
    void setGoldenParameterMismatch(bool value);

    bool isChecksumMismatch() const;
    void setChecksumMismatch(bool value);

    std::string getChecksum(const std::string& filePath) const;
    void setChecksum(const std::string& filePath, const std::string& checksum);

    std::string getAlarmId(const std::string& reason) const;
    void setAlarmId(const std::string& reason, const std::string& alarmId);

private:
    std::string file_path_;
    json status_;
};

#endif // MISMATCH_STATUS_H
