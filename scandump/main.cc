/* Simple CLI program to retrieve and decode scan results
 */
#include <iostream>
#include <vector>

#include "fmt/format.h"
#include "fmt/ostream.h"
#include "logging.h"

#include "netlink.hh"

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s ifname\n", argv[0]);
		exit(1);
	}

	logging_init("scandump.log");

//	auto logger = spdlog::stdout_color_mt("basic_logger");
//	auto logger = spdlog::basic_logger_mt("basic_logger", "scandump.log");
	auto logger = spdlog::get("main");
	logger->info("hello, world");

//	spdlog::info("Hello, {}!", "World");

	const char* ifname = argv[1];

	// quicky test code; deleteme
	std::array<uint8_t, 10> buf {};
	auto ie = cfg80211::IE(255,12, buf.data());

	std::vector<cfg80211::BSS> bss_list;
	try {
		cfg80211::Cfg80211 iw;
		iw.get_scan(ifname, bss_list);
	}
	catch (cfg80211::NetlinkException& err) {
		logger->error(err.what());
		std::terminate();
	}

	for ( auto&& bss : bss_list ) {
		fmt::print("found BSS {} num_ies={}\n", bss, bss.ie_list.size());
		spdlog::info(fmt::format("found BSS {} num_ies={}", bss, bss.ie_list.size()));
		logger->info("found BSS {} num_ies={}", bss, bss.ie_list.size());
//		std::cout << "found BSS " << bss << " num_ies=" << bss.ie_list.size() << "\n";
		logger->info("SSID={}", bss.get_ssid());
		for (auto&& ie : bss.ie_list) {
			logger->info("ie={}", ie);
		}
	}

	spdlog::info("Goodbye, {}!", "World");
	logger->info("goodbye, world");

//	auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
//	auto my_logger= std::make_shared<spdlog::logger>("mylogger", sink);
//	spdlog::register_logger(my_logger);
//	my_logger->info("hello from mylogger");
//	auto also_my_logger = spdlog::get("mylogger");
//	also_my_logger->info("hello again from mylogger");
//
//	auto logger2 = spdlog::stdout_logger_mt("logger2");
//	logger2->warn("foo bar baz");

	return 0;
}

