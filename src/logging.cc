#include <memory>
#include <cassert> // XXX workaround for spdlog pattern_formatter 

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h" // support for basic file logging
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "logging.h"

void logging_init(const char* logfilename)
{
//	spdlog::set_level(spdlog::level::info);
	spdlog::set_level(spdlog::level::trace);

//	auto logcfg = spdlog::basic_logger_mt("cfg80211", logfilename);
//	logcfg->set_level(spdlog::level::trace);

//	auto logie = spdlog::basic_logger_mt("ie", logfilename);
//	logie->set_level(spdlog::level::trace);

//	auto logger = spdlog::basic_logger_mt("main", logfilename);

	// https://github.com/gabime/spdlog/wiki/4.-Sinks
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt> (logfilename, 1024*1024*10, 10, false);
//	auto file_logger = spdlog::rotating_logger_mt("file_logger", logfilename, 1048576 * 10, 10);

	auto logger = std::make_shared<spdlog::logger>("main", rotating);
	spdlog::register_logger(logger);
	logger->set_level(spdlog::level::debug);
	logger->info("logging started");

	logger = std::make_shared<spdlog::logger>("cfg80211", rotating);
	spdlog::register_logger(logger);
	logger->set_level(spdlog::level::debug);
	logger->info("logging started");

	logger = std::make_shared<spdlog::logger>("ie", rotating);
	spdlog::register_logger(logger);
	logger->set_level(spdlog::level::debug);
	logger->info("logging started");

       logger = std::make_shared<spdlog::logger>("bss", rotating);
       spdlog::register_logger(logger);
       logger->set_level(spdlog::level::debug);
       logger->info("logging started");
}

