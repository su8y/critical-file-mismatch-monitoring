#ifndef CONFIG_REPOSITORY_H
#define CONFIG_REPOSITORY_H

#include <string>

class ConfigRepository {
public:
    virtual ~ConfigRepository() = default;
    virtual std::string getGoldenParameterFile() = 0;
    virtual std::string getReferenceFile() = 0;
    virtual std::string getChecksumFile() = 0;
};

#endif // CONFIG_REPOSITORY_H
