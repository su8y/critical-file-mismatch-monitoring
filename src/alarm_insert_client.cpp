#include "alarm_insert_client.h"

static bool storeFailedQuery(const char *failedCommand) {
	if (failedCommand == NULL) {
		spdlog::error("Failed command is NULL");
		return false;
	}

	int fd = open(FAILED_QUERY_FILE, O_APPEND | O_CREAT | O_WRONLY, 0644);
	if (fd == -1) {
		spdlog::error("[ALARM::storeFailedQuery]Failed open file descriptor: {}", strerror(errno));
		return false;
	}

	if (flock(fd, LOCK_EX) == -1) {
		spdlog::error("Failed to lock file: {}", strerror(errno));
		close(fd);
		return false;
	}

	FILE *file = fdopen(fd, "a");

	if (file == NULL) {

		close(fd);
		spdlog::error("Failed to open failed query file: {}", strerror(errno));
		return false;
	}

	fprintf(file, "%s\n", failedCommand);

	spdlog::info("Failed query stored: {}", failedCommand);

	fclose(file); // 내부적으로 fd를 닫기때문에 락 해제도 진행함.
	return true;
}

static bool buildAlarmCommand(char *command, size_t commandSize,
							  const char *hostname, const char *type,
							  const char *severity, const char *code,
							  const char *message, const char *ip,
							  const char *index, const char *prevIndex,
							  const char *registerDateTime) {

	int result = snprintf(
			command, commandSize, "%s %s \"%s\" %s %s \"%s\" %s %s %s \"%s\"",
			ALARM_INSERT_SCRIPT_PATH, hostname, type, severity, code, message,
			ip, index, prevIndex, registerDateTime);

	if (result < 0 || result >= commandSize) {
		spdlog::error("Command buffer overflow or formatting error");
		return false;
	}

	return true;
}

static bool executeSystemCommand(const char *command) {
	if (command == NULL || strlen(command) <= 0) {
		spdlog::error("Command is NULL");
		return false;
	}

	int status = system(command);

	if (status == -1) {
		spdlog::error("Failed to execute command: {}", strerror(errno));
		return false;
	}

	// WIFEXITED와 WEXITSTATUS를 사용하여 정확한 종료 상태 확인
	if (WIFEXITED(status)) {
		int exitCode = WEXITSTATUS(status);
		if (exitCode == 0) {
			spdlog::info("Command executed successfully: {}", command);
			return true;
		} else {
			spdlog::error("Command failed with exit code {}: {}", exitCode,
						  command);
			return false;
		}
	} else {
		spdlog::error("Command terminated abnormally: {}", command);
		return false;
	}
}

static bool validateInputParameters(const char *hostname, const char *type,
									const char *code, const char *severity,
									const char *ip, const char *message,
									const char *index, const char *prevIndex,
									const char *registerDateTime) {

	if (hostname == NULL || strlen(hostname) == 0) {
		spdlog::error("Hostname is NULL or empty");
		return false;
	}

	if (type == NULL || strlen(type) == 0) {
		spdlog::error("Type is NULL or empty");
		return false;
	}

	if (severity == NULL || strlen(severity) == 0) {
		spdlog::error("Severity is NULL or empty");
		return false;
	}

	if (code == NULL || strlen(code) == 0) {
		spdlog::error("Code is NULL or empty");
		return false;
	}

	if (ip == NULL || strlen(ip) == 0) {
		spdlog::error("IP is NULL or empty");
		return false;
	}

	if (message == NULL) {
		spdlog::error("Message is NULL");
		return false;
	}

	if (index == NULL || strlen(index) == 0) {
		spdlog::error("Index is NULL or empty");
		return false;
	}

	if (registerDateTime == NULL || strlen(registerDateTime) == 0) {
		spdlog::error("RegisterDateTime is NULL or empty");
		return false;
	}

	return true;
}

bool insertIntoPackAlarmInfo(const char *index, const char *code,
							 const char *type, const char *severity,
							 const char *hostname, const char *ip,
							 const char *message, const char *prevIndex,
							 const char *registerDatetime) {

	// 입력 매개변수 유효성 검사
	if (!validateInputParameters(hostname, type, code, severity, ip, message,
								 index, prevIndex, registerDatetime)) {
		printf("vaild\n");
		return false;
	}

	// 명령어 버퍼 초기화
	char command[MAX_COMMAND_LENGTH];
	memset(command, 0, sizeof(command));

	// 알람 명령어 생성
	if (!buildAlarmCommand(command, sizeof(command), hostname, type, severity,
						   code, message, ip, index, prevIndex,
						   registerDatetime)) {
		printf("const\n");
		return false;
	}

	// 시스템 명령어 실행, 실패시 실패한 쿼리 저장
	if (!executeSystemCommand(command)) {
		printf("failed\n");
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
	if (fd == -1) {
		spdlog::error("Failed open file descriptor: {}", strerror(errno));
		return false;
	}
	flock(fd, LOCK_EX);

	FILE *file = fdopen(fd, "r+");
	if (file == NULL) {
		close(fd);
		spdlog::info("No failed queries file found or file cannot be opened");
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

	failedQueryBuffer = (char **)malloc(bufferCapacity * sizeof(char *));

	// 각 실패한 쿼리를 재시도
	while (fgets(line, sizeof(line), file) != NULL) {
		// 개행 문자 제거
		line[strcspn(line, "\n")] = 0;

		if (strlen(line) == 0)
			continue;

		totalCount++;

		if (executeSystemCommand(line) == true) {
			successCount += 1;
			continue;
		}
		// Buffer에 쓰기
		if (failedCountInBuffer >= bufferCapacity) {
			bufferCapacity *= 2;
			char **tempRealloc = (char **)realloc(
					failedQueryBuffer, bufferCapacity * sizeof(char *));

			if (tempRealloc == NULL) {
				result = false;
				goto cleanup;
			}

			failedQueryBuffer = tempRealloc;
		}

		failedQueryBuffer[failedCountInBuffer] = strdup(line); // 문자열 복사

		if (failedQueryBuffer[failedCountInBuffer] == NULL) {
			spdlog::error("Failed to duplicate query string.");
			// 메모리 할당 실패 시 자원 해제 후 종료
			result = false;
			goto cleanup;
		}

		failedCountInBuffer++;
	}

	rewind(file); // seek start
	if (ftruncate(fd, 0) == -1) {
		spdlog::error("Failed to truncate file: {}", strerror(errno));
		result = false;
		goto cleanup;
	}

	for (int i = 0; i < failedCountInBuffer; i++)
		fprintf(file, "%s\n", failedQueryBuffer[i]);

	if (fflush(file) != 0)
		spdlog::error("Failed to flush file stream: {}", strerror(errno));

	if (totalCount > 0) {
		spdlog::info("Retry completed: {}/{} queries succeeded", successCount,
					 totalCount);
	}

	result = true;

cleanup:
	if (file) {
		fclose(file);
	} else if (fd != -1) {
		flock(fd, LOCK_UN);
		close(fd);
	}

	for (int i = 0; i < failedCountInBuffer; ++i)
		if (failedQueryBuffer[i])
			free(failedQueryBuffer[i]);
	if (failedQueryBuffer)
		free(failedQueryBuffer);

	return result;
}
