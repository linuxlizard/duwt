#ifndef LOGGING_H
#define LOGGING_H

// https://github.com/gabime/spdlog

#include <cassert> // XXX workaround for spdlog pattern_formatter 

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"

void logging_init(const char* logfilename);

#endif

