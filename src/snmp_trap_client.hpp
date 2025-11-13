#include <fstream>
#include <spdlog/spdlog.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <sstream>

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
		std::ofstream outfile(FAILED_QUERY_FILE, std::ios::out | std::ios::app);

		if (!outfile.is_open()) {
			spdlog::error("[SNMP::storeFailedQuery] Failed to open failed query file: {}",
					FAILED_QUERY_FILE);
			return false;
		}
		outfile << command << std::endl;
		return true;
	}


	bool executeCommand(const std::string &command) {
		int result = system(command.c_str());
		if(result == -1) {
			spdlog::error("[SNMP::executeCommand] Failed to execute command");
			return false;
		} else if (WIFEXITED(result) && WEXITSTATUS(result) != 0) {
			spdlog::error("[SNMP::executeCommand] Command exited with non-zero status: {}", WEXITSTATUS(result));
			return false;
		}
		return true;

	}

	/* 실패한 쿼리 재시도 */
	bool retryFailedQueries() {
		if (!std::filesystem::exists(FAILED_QUERY_FILE)) {
			std::ofstream create_file(FAILED_QUERY_FILE);
			if (!create_file.is_open()) {
				spdlog::error("[storeFailedQuery] Failed to create failed "
							  "query file: {}",
							  FAILED_QUERY_FILE);
				return false;
			}
			create_file.close(); // 생성 후 닫기
		}
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
		std::string message= "None";
		if(!info.message.empty()) {
			message = info.message;
		}
		// 실패한 quries 재시도
		retryFailedQueries();
		auto quote = [](const std::string &s) {
			std::string escaped = s;
			// 내부 "를 \"로 변환
			size_t pos = 0;
			while ((pos = escaped.find('"', pos)) != std::string::npos) {
				escaped.replace(pos, 1, "\\\"");
				pos += 2; // 치환 후 이동
			}

			pos = 0;
			while ((pos = escaped.find(' ', pos)) != std::string::npos) {
				escaped.replace(pos, 1, "_");
				pos += 1; 
			}
			return "\"" + escaped + "\"";
		};

		// command 생성
		std::ostringstream oss;
		oss << quote(SNMP_TRAP_SCRIPT) << ' ' 
			<< quote(info.oss_addr) << ' '
			<< quote(info.hostname) << ' ' 
			<< quote(info.code) << ' '
			<< quote(info.type) << ' ' 
			<< quote(std::to_string(info.severity)) << ' ' 
			<< quote(std::to_string(info.status)) << ' '
			<< quote(std::to_string(info.value)) << ' ' 
			<< quote(message) << ' '
			<< quote(info.registerDatetime); 

		if (!executeCommand(oss.str())) {
			spdlog::error("Failed to execute SNMP TRAP command, storing to failed queue. command: {}", oss.str());
			storeFailedQuery(oss.str());
		} else {
			spdlog::info("Command executed successfully: {}", oss.str());
			
		}
	}

} // namespace FailedQueue
