
#include "../json_config_repository.h"
#include <fstream>

JsonConfigRepository::JsonConfigRepository(const std::string& filePath) : file_path_(filePath) {
    std::ifstream file(file_path_);
    if (file.is_open()) {
        file >> config_;
    }
}

std::string JsonConfigRepository::getGoldenParameterFile() {
    return config_.value("goldenParameterFile", "");
}

std::string JsonConfigRepository::getReferenceFile() {
    return config_.value("referenceFile", "");
}

std::string JsonConfigRepository::getChecksumFile() {
    return config_.value("checksumFile", "");
}
