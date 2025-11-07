
#ifndef MISMATCHMONITOR_TARGET_H
#define MISMATCHMONITOR_TARGET_H
#include <string>

struct Target
{
	std::string path;
	std::string golden_reference;
};

#endif // MISMATCHMONITOR_TARGET_H
