
#ifndef JSON_CONFIG_REPOSITORY_H
#define JSON_CONFIG_REPOSITORY_H

#include <nlohmann/json.hpp>
#include "config_repository.h"

using json = nlohmann::json;

class JsonConfigRepository : public ConfigRepository
{
public:
	explicit JsonConfigRepository(const std::string& filePath);

	std::string getGoldenParameterFile() override;
	std::string getReferenceFile() override;
	std::string getChecksumFile() override;

private:
	std::string file_path_;
	json config_;
};

#endif // JSON_CONFIG_REPOSITORY_H
