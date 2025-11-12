#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include <vector>

extern const char *FAILED_QUERY_FILE; // 실패 쿼리를 저장하는 Queue
const std::string SNMP_TRAP_SCRIPT = "/home/gis/FM/shell/trap_send.sh";

struct snmp_trap_info {
	std::string oss_addr;
	std::string hostname;
	std::string code; // A2700, A2800
	std::string type; // MISMATCH, CRITICALFILE MISMATCH
	int severity;	  // MAJOR, CLEARED
	int status;		  // (RELEASE, CLEARED) 발생 알람인지 해제알람인지
	int value;		  // MAJOR, CLEARED
	std::string message;
	std::string registerDatetime;
};

namespace FailedQueue {

	/* 실패한 쿼리 적재 */
	bool storeFailedQuery(const std::string &command) {
		std::ofstream outfile(FAILED_QUERY_FILE, std::ios::app);

		if (!outfile.is_open()) {
			spdlog::error("Failed to open failed query file: {}",
					FAILED_QUERY_FILE);
			return false;
		}
		outfile << command << std::endl;
	}


	bool executeCommand(const std::string &command) {
		int result = system(command.c_str());
		if(result == -1) {
			spdlog::error("Failed to execute command");
			return false;
		}
		return true;

	}

	/* 실패한 쿼리 재시도 */
	bool retryFailedQueries() {
		std::ifstream inputfile(FAILED_QUERY_FILE);

		if (!inputfile.is_open()) {
			spdlog::error("Failed to open failed query file: {}",
					FAILED_QUERY_FILE);
			return false;
		}

		/* get failed command list */
		std::string output; // outputfile result
		std::string line;
		while (std::getline(inputfile, line))
			if (!executeCommand(line))
				output += line;

		inputfile.close();
		std::ofstream outputfile(FAILED_QUERY_FILE);
		outputfile << output;
		outputfile.flush();
		return true;
	}

	/* command 실행 */
	void trapSend(const snmp_trap_info &info) {
		// 실패한 quries 재시도
		retryFailedQueries();

		// command 생성
		std::string command = SNMP_TRAP_SCRIPT + " " + info.oss_addr + " " +
			info.hostname + " " + info.code + " \"" + info.type + "\" " +
			std::to_string(info.severity) + " " +
			std::to_string(info.status) + " " +
			std::to_string(info.value) + " \"" + info.message + "\" " +
			 info.registerDatetime;

		// command 실행 실패시 failed Query Queue에 적재
		if (!executeCommand(command)) {
			storeFailedQuery(command);
		}
	}

} // namespace FailedQueue
