#include "logging.h"

void logging_init(const char* logfilename)
{
//	spdlog::set_level(spdlog::level::info);
	spdlog::set_level(spdlog::level::trace);

	auto logcfg = spdlog::basic_logger_mt("cfg80211", logfilename);
//	logcfg->set_level(spdlog::level::trace);

	auto logie = spdlog::basic_logger_mt("ie", logfilename);
//	logie->set_level(spdlog::level::trace);

	auto logger = spdlog::basic_logger_mt("main", logfilename);

}

