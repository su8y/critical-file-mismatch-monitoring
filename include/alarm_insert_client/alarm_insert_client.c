#include "alarm_insert_client.h"



static bool storeFailedQuery(const char* failedCommand) {
    if (failedCommand == NULL) {
        log_error("Failed command is NULL");
        return false;
    }
    
    int fd = open(FAILED_QUERY_FILE, O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (fd == -1) 
    {
        log_error("Failed open file descriptor: %s", strerror(errno));
        return false;
    }

    if (flock(fd, LOCK_EX) == -1) {
        log_error("Failed to lock file: %s", strerror(errno));
        close(fd);
        return false;
    }

    FILE* file = fdopen(fd, "a");

    if (file == NULL) {

        close(fd);
        log_error("Failed to open failed query file: %s", strerror(errno));
        return false;
    }
    
    fprintf(file, "%s\n", failedCommand);
    
    log_info("Failed query stored: %s", failedCommand);

    fclose(file);  // 내부적으로 fd를 닫기때문에 락 해제도 진행함.
    return true;
}

static bool buildAlarmCommand(char* command, size_t commandSize,
                           const char* hostname, const char* type,
                           const char* severity, const char* code,
                           const char* message, const char* ip,
                           const char* index, const char* prevIndex,
                           const char*  registerDateTime
                        ) {
    
    int result = snprintf(command, commandSize,
        "%s %s %s %s %s \"%s\" %s %s %s \"%s\"",
        ALARM_INSERT_SCRIPT_PATH, hostname, type, severity, code, message, ip, index, prevIndex, registerDateTime);
    
    if (result < 0 || result >= commandSize) {
        log_error("Command buffer overflow or formatting error");
        return false;
    }
    
    return true;
}

static bool executeSystemCommand(const char* command) {
    if (command == NULL || strlen(command) <= 0)  {
        log_error("Command is NULL");
        return false;
    }
    
    int status = system(command);
    
    if (status == -1) {
        log_error("Failed to execute command: %s", strerror(errno));
        return false;
    }
    
    // WIFEXITED와 WEXITSTATUS를 사용하여 정확한 종료 상태 확인
    if (WIFEXITED(status)) {
        int exitCode = WEXITSTATUS(status);
        if (exitCode == 0) {
            log_info("Command executed successfully: %s", command);
            return true;
        } else {
            log_error("Command failed with exit code %d: %s", exitCode, command);
            return false;
        }
    } else {
        log_error("Command terminated abnormally: %s", command);
        return false;
    }
}

static bool validateInputParameters(const char* hostname, const char* type,
                                const char* code, const char* severity,
                                const char* ip, const char* message,
                                const char* index, const char* prevIndex,
                                const char* registerDateTime) {
    
    if (hostname == NULL || strlen(hostname) == 0) {
        log_error("Hostname is NULL or empty");
        return false;
    }
    
    if (type == NULL || strlen(type) == 0) {
        log_error("Type is NULL or empty");
        return false;
    }

    if (severity == NULL || strlen(severity) == 0) {
        log_error("Severity is NULL or empty");
        return false;
    }
    
    if (code == NULL || strlen(code) == 0) {
        log_error("Code is NULL or empty");
        return false;
    }
    
    if (ip == NULL || strlen(ip) == 0) {
        log_error("IP is NULL or empty");
        return false;
    }
    
    if (message == NULL) {
        log_error("Message is NULL");
        return false;
    }
    
    if (index == NULL || strlen(index) == 0) {
        log_error("Index is NULL or empty");
        return false;
    }
    
    if (registerDateTime == NULL || strlen(registerDateTime) == 0) {
        log_error("RegisterDateTime is NULL or empty");
        return false;
    }
    
    return true;
}

bool insertIntoPackAlarmInfo(const char* index, const char* code, 
                           const char* type, const char* severity,
                           const char* hostname, const char* ip,
                           const char* message, const char* prevIndex,
                           const char* registerDatetime) {
    
    // 입력 매개변수 유효성 검사
    if (!validateInputParameters(hostname, type, code, severity, 
                              ip, message, index, prevIndex, registerDatetime)) {
        return false;
    }
    
    // 명령어 버퍼 초기화
    char command[MAX_COMMAND_LENGTH];
    memset(command, 0, sizeof(command));
    
    // 알람 명령어 생성
    if (!buildAlarmCommand(command, sizeof(command), hostname, type, severity, code, message, ip, index, prevIndex, registerDatetime))
    {
        return false;
    }

    // 시스템 명령어 실행, 실패시 실패한 쿼리 저장
    if (!executeSystemCommand(command)) {
        storeFailedQuery(command);
        return false;
    }
    
    return true;
}

bool insertIntoPackAlarmInfoWithoutHostInfo(const char* index, const char* code,
                           			const char* type, const char* severity,
                           			const char* message, const char* prevIndex) {

    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) == -1)
    {
        log_error("ERROR\tgethostname() fail...");
        exit(1);
    }

    // GET IDDR

    char ip[128];
    char *ip_addr;
    struct ifaddrs *addrs, *tmp_addrs;
    struct sockaddr_in *sa;

    getifaddrs(&addrs);
    tmp_addrs = addrs;

    while (tmp_addrs) {
        if (tmp_addrs->ifa_addr && tmp_addrs->ifa_addr->sa_family == AF_INET) {
            sa = (struct sockaddr_in *)tmp_addrs->ifa_addr;
            ip_addr = inet_ntoa(sa->sin_addr);

            if (!strcmp(tmp_addrs->ifa_name, "enp0s3")) {
                strcpy(ip, ip_addr);
                break;
            }
            else if (!strcmp(tmp_addrs->ifa_name, "ens192")) {
                strcpy(ip, ip_addr);
                break;
            }
            else if (!strcmp(tmp_addrs->ifa_name, "eth0")) {
                strcpy(ip, ip_addr);
                break;
            }
        }
        tmp_addrs = tmp_addrs->ifa_next;
    }
    freeifaddrs(addrs);

    //registerDatetime
    time_t timer;
    char registerDateTime[128];
    timer = time(NULL);
    struct tm *timeinfo;
    timeinfo = localtime(&timer);
    strftime(registerDateTime, sizeof(registerDateTime), "%Y-%m-%d %H:%M:%S", timeinfo);

    // 입력 매개변수 유효성 검사
    if (!validateInputParameters(hostname, type, code, severity,
                              ip, message, index, prevIndex, registerDateTime)) {
        return false;
     }

    // 명령어 버퍼 초기화
    char command[MAX_COMMAND_LENGTH];
    memset(command, 0, sizeof(command));

    // 알람 명령어 생성
    if (!buildAlarmCommand(command, sizeof(command), hostname, type, severity, code, message, ip, index, prevIndex, registerDateTime))
    {
        return false;
    }

    // 시스템 명령어 실행, 실패시 실패한 쿼리 저장
    if (!executeSystemCommand(command)) {
        storeFailedQuery(command);
        return false;
    }

    return true;
}

/**
 * @brief 실패한 쿼리들을 재시도합니다.
 * @return 성공시 0, 실패시 -1
 */
bool retryFailedQueries() {
    int fd = open(FAILED_QUERY_FILE, O_RDWR | O_CREAT, 0644);
    if (fd == -1)
    {
        log_error("Failed open file descriptor: %s", strerror(errno));
        return false;
    }
    flock(fd, LOCK_EX);     

    FILE* file = fdopen(fd, "r+");
    if (file == NULL) {
        close(fd);
        log_info("No failed queries file found or file cannot be opened");
        return true;
    }


    
    bool result = true;
    char line[MAX_COMMAND_LENGTH];
    int successCount = 0;
    int totalCount = 0;
    
    // 실패한 쿼리들을 버퍼에 저장하기 위한 준비
    char **failedQueryBuffer;
    int bufferCapacity = 10;
    int failedCountInBuffer = 0;

    failedQueryBuffer = (char**)malloc(bufferCapacity * sizeof(char*));

    
    
    // 각 실패한 쿼리를 재시도
    while (fgets(line, sizeof(line), file) != NULL) {
        // 개행 문자 제거
        line[strcspn(line, "\n")] = 0;
        
        if (strlen(line) == 0) {
            continue;
        }
        
        totalCount++;
        
        if (executeSystemCommand(line) == true) {
            successCount += 1;
            continue;
        }
        // Buffer에 쓰기
        if (failedCountInBuffer >= bufferCapacity) {
            bufferCapacity *= 2;
            char** tempRealloc = (char**) realloc(failedQueryBuffer, bufferCapacity * sizeof(char*));

            if (tempRealloc == NULL) {
                result = false;
                goto cleanup;
            }

            failedQueryBuffer =  tempRealloc;
        }

        failedQueryBuffer[failedCountInBuffer] = strdup(line); // 문자열 복사 

        if (failedQueryBuffer[failedCountInBuffer] == NULL) {
            log_error("Failed to duplicate query string.");
            // 메모리 할당 실패 시 자원 해제 후 종료
            result = false;
            goto cleanup;
        }

        failedCountInBuffer++;

    }
    

    
    rewind(file); // seek start
    if (ftruncate(fd, 0 ) == -1) 
    {
        log_error("Failed to truncate file: %s", strerror(errno));
        result = false;
        goto cleanup;
    }

    for(int i=0; i<failedCountInBuffer; i++)
    {
        fprintf(file, "%s\n", failedQueryBuffer[i]);
    }

    if(fflush(file) !=0) 
    {
        log_error("Failed to flush file stream: %s", strerror(errno));
    }

    if (totalCount > 0)
    {
        log_info("Retry completed: %d/%d queries succeeded", successCount, totalCount);
    }

    result = true;

cleanup: 
    if (file) 
    {
        fclose(file);
    } 
    else if (fd != -1) 
    {
        flock(fd, LOCK_UN);
        close(fd);
    }

    for (int i = 0; i < failedCountInBuffer; ++i) 
        if (failedQueryBuffer[i]) free(failedQueryBuffer[i]);
    if(failedQueryBuffer) free(failedQueryBuffer);

    return result;
}
