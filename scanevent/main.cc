/* Simple CLI program to monitor cfg80211 netlink scan events
 *
 */
#include <cassert>

#include <iostream>
#include <sstream>
#include <vector>

#include <poll.h>

#include "fmt/format.h"
#include "logging.h"
#include "cfg80211.h"

// I made a thing!
template <typename Input, typename T>
std::string join(Input& iterable)
{
	std::ostringstream s;
	std::copy(iterable.cbegin(), iterable.cend(), 
			std::ostream_iterator<T>(s, " "));
	return s.str();
}

int main(void)
{
	logging_init("scanevent.log");
	auto logger = spdlog::get("main");
	logger->info("hello, world");

	cfg80211::Cfg80211 iw;

	// set up iw to listen on scan multicast messages
	iw.listen_scan_events();

	// longer term goal is to use
	// https://doc.qt.io/qt-5/qsocketnotifier.html

	struct pollfd fds {};
	fds.fd = iw.get_fd();
	fds.events = POLLIN | POLLPRI;

	int ret;
	cfg80211::ScanEvent sev {};
	while (1) {
		// wait forever for socket socket
		std::cout << "waiting on poll...\n" << ret;
		fds.revents = 0;
		ret = poll(&fds, 1, -1);
		std::cout << "poll ret=" << ret << " revents=" << fds.revents << "\n";

		if (ret < 0) {
			// something went wrong
			assert(0); // TODO better handling
		}

		if (fds.revents & (POLLHUP | POLLERR | POLLNVAL) ) {
			// something went wrong
			assert(0); // TODO better handling
		}

		// get some messages
		if (fds.revents & (POLLIN|POLLPRI)) {
			iw.fetch_scan_events(sev);

			std::cout << "scan event on ifname=" << sev.ifname << " ifidx=" << sev.ifidx << "\n";
			std::cout << "scan event on phyname=" << sev.phyname << " phyidx=" << sev.phyidx << "\n";

			std::cout << "found count=" << sev.frequencies.size() << " freq\n";
		//	for (auto f : frequencies) {
		//		std::cout << "f=" << f << "\n";
		//	}
			// https://en.cppreference.com/w/cpp/algorithm/copy
			std::copy(sev.frequencies.cbegin(), sev.frequencies.cend(), 
						std::ostream_iterator<uint32_t>(std::cout, " "));
			std::cout << "\n";

			std::ostringstream s;
			std::copy(sev.frequencies.cbegin(), sev.frequencies.cend(), 
						std::ostream_iterator<uint32_t>(s, ","));
			std::string joined = s.str();
			joined.pop_back();
			std::cout << joined << "*\n";

			std::string joined2 = join<std::vector<uint32_t>, uint32_t>(sev.frequencies);
			fmt::print("found freqlist={}**\n", joined2);

		}
		// back around for more
	}

	return 0;
}

