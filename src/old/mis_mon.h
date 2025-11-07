#ifndef MISMATCHMONITOR_MIS_MON_H
#define MISMATCHMONITOR_MIS_MON_H

using namespace std;
// #include <alarm_insert_client/alarm_insert_client.h>
#include <csignal>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

// #include "support/utils.hpp"
#include "version.h"

const string META_CONFIG_PATH = "/home/gis/config/meta_data.json";
const string JSON_CONFIG_PATH =
    "/home/gis/config/Current_GoldenParameter.json";
const string STATUS_FILE_PATH = "/home/gis/MM/resources/backup_alarm.json";

void signal_handler(int signum);

/*
 * server log level
 */
#define LEVEL_INFO 0
#define LEVEL_WARN 1
#define LEVEL_ERROR 2

/*
 * Major Alarm
 */
class misMonAlarmStatus {
public:
  string prevGoldenParameterMismatch;
  string prevChecksum;

};

/*
 * Server
 */
class misMonServer {
public:
  string configfile;
  string backupfile;
  string logfile;
  bool daemonize;
  string pid;
  string hostname;
  string hostIp;
  int verbosity;
  vector<string> ossAddresses;

  void backup(void);
  void info();
};
extern misMonServer server;
#endif