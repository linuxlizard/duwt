/* Simple CLI program to retrieve and decode scan results
 */
#include <iostream>
#include <vector>

#include "fmt/format.h"
#include "fmt/ostream.h"

#include "logging.h"
#include "cfg80211.h"

int main(int argc, char* argv[])
{
	if (argc == 1) {
		fprintf(stderr, "usage: %s [--json|--text] ifname\n", argv[0]);
		exit(1);
	}

	logging_init("scandump.log");

//	auto logger = spdlog::stdout_color_mt("basic_logger");
//	auto logger = spdlog::basic_logger_mt("basic_logger", "scandump.log");
	auto logger = spdlog::get("main");
	logger->info("hello, world");

//	spdlog::info("Hello, {}!", "World");

	// TODO use getopt 
	const char* ifname = argv[1];
	if (ifname[0] == '-' && ifname[1] == '-') {
		ifname = argv[2];
	}

	// quicky test code; deleteme
//	std::array<uint8_t, 10> buf {};
//	auto ie = cfg80211::IE(255,12, buf.data());

	std::vector<cfg80211::BSS> bss_list;
	try {
		cfg80211::Cfg80211 iw;
		iw.get_scan(ifname, bss_list);
	}
	catch (cfg80211::NetlinkException& err) {
		logger->error(err.what());
		std::terminate();
	}

	Json::Value scan_dump;
	for ( auto& bss : bss_list ) {
//		fmt::print("found BSS {} num_ies={}\n", bss, bss.ie_count());
//		spdlog::info(fmt::format("found BSS {} num_ies={}", bss, bss.ie_count()));
		logger->info("found BSS {} num_ies={}", bss, bss.ie_count());
//		std::cout << "found BSS " << bss << " num_ies=" << bss.ie_list_js.size() << "\n";
		logger->info("SSID={}", bss.get_ssid());

		Json::Value bss_js;
		bss_js["bssid"] = bss.get_bssid();

		Json::Value ie_list_js;
		for (auto&& ie = bss.cbegin() ; ie != bss.cend() ; ++ie) {
			// iterator across a shared ptr so one * for iterator and another * for deference
			logger->info("ie={}", **ie);
//			fmt::print("\tie={}\n", **ie);
			Json::Value v { (*ie)->make_json() };

			ie_list_js.append(v);

			// brute force find the SSID and make a top level copy because it's
			// most often required
			if ((*ie)->get_id() == 0) {
				bss_js["SSID"] = v["SSID"];
			}

//			for (auto&& s = ie->cbegin() ; s != ie->cend() ; ++s) {
//				std::cout << "\t\t" << *s << "\n";
//			}

//			for (auto& const s : ie) {
//				fmt::print("\t\t{}\n", s);
//			}
		}
		bss_js["ie_list"] = ie_list_js;
		scan_dump["bss"].append(bss_js);
	}

//	std::cout << scan_dump << "\n";

	Json::StreamWriterBuilder writer_builder;
	writer_builder["indentation"] = " ";
	std::unique_ptr<Json::StreamWriter> writer(writer_builder.newStreamWriter());
	writer->write(scan_dump, &std::cout);
	std::cout << "\n";

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

