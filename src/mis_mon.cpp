#include "mis_mon.h"

#include "config.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fcntl.h>

// #include "application/golden_parameter_monitor.h"
// #include "infrastructure/odbc_alarm_repository.h"
// #include "domain/MismatchStatus.h"

using json = nlohmann::json;

std::atomic<bool> keepRunning(true);

misMonServer server;


void signal_handler(int signum) {
  keepRunning = false;
  spdlog::info("Signal {} received", signum);
  exit(0);
}
void initSignalHandler(void) {
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
}

void info() {
  spdlog::info("mis_mon version {}", MIS_MON_VERSION);
  spdlog::info("mis_mon initial_variables\n"
               "\rconfigfile:{}\n"
               "\rbackupfile:{}\n"
               "\rlogfile:{}\n"
               "\rdamonize:{}\n"
               "\rpid:{}\n"
               "\rhostname:{}\n"
               "\rhostIp:{}\n"
               "\rloglevel:{}\n"
               "\rossAddress[0]:{}\n"
               "\rossAddress[1]:{}\n",
               server.configfile, server.backupfile, server.logfile,
               server.daemonize, server.pid, server.hostname, server.hostIp,
               server.verbosity, server.ossAddresses[0],
               server.ossAddresses[1]);
}
int main(int argc, char **argv) {
  if (argc >= 2) {
    if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
      printf("mis_mon version %s\n", MIS_MON_VERSION);
      exit(0);
    }
    if (strcmp(argv[1], "-d") == 0 || strcmp(argv[1], "--daemon") == 0) {
      server.daemonize = false;
    }
  }



  server.configfile = JSON_CONFIG_PATH;
  server.backupfile = STATUS_FILE_PATH;
  server.logfile = "logs/mis_mon.log";

  initMisMonServer();
  initSignalHandler();
  info();

  // auto mismatchStatus = std::make_shared<MismatchStatus>(STATUS_FILE_PATH);
  // if (!mismatchStatus->load()) {
  //   spdlog::error("Failed load mismatch status");
  //   return -1;
  // }
  // GoldenParameterMonitor golden_parameter_monitor(
  //     AlarmRepository::getInstance(), ConfigProperties::getInstance(),
  //     mismatchStatus);
  //
  // std::thread t1(
  //     [&golden_parameter_monitor]() { golden_parameter_monitor.schedule(10); });
  //
  // t1.join();

  spdlog::info("Exit");

  return 0;
}
