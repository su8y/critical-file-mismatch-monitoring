#ifndef ALARM_INSERT_CLIENT_H
#define ALARM_INSERT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>

#include <spdlog/spdlog.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_QUERY_LENGTH 512

extern const char* FAILED_QUERY_FILE; // 실패 쿼리를 저장하는 Queue
extern const char* ALARM_INSERT_SCRIPT_PATH;


bool insertIntoPackAlarmInfo(const char* index, const char* code,
                            const char* type, const char* severity,
                            const char* hostname, const char* ip,
                            const char* message, const char* prevIndex,
                            const char* registerDatetime);

bool retryFailedQueries();

#ifdef __cplusplus
}
#endif
#endif
