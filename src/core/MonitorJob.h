#ifndef MISMATCHMONITOR_MONITORJOB_H
#define MISMATCHMONITOR_MONITORJOB_H

#pragma once

#include "Target.h"
#include "../interfaces/IComparisonStrategy.h"

/*
 * 엔진이 실행할 작업의 단위
 */
struct MonitorJob {
	Target target;
	IComparisonStrategy* strategy;
};
#endif // MISMATCHMONITOR_MONITORJOB_H
