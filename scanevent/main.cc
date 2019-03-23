/* Simple CLI program to monitor cfg80211 netlink scan events
 *
 */
#include <iostream>
#include <vector>

#include <poll.h>

#include "netlink.hh"

int main(void)
{
	cfg80211::Cfg80211 iw;

	// set up iw to listen on scan multicast messages
	iw.listen_scan_events();

	// longer term goal is to use
	// https://doc.qt.io/qt-5/qsocketnotifier.html

	struct pollfd fds {};
	fds.fd = iw.get_fd();
	fds.events = POLLIN | POLLPRI;

	int ret;
	while (1) {
		// wait forever for socket socket
		std::cout << "waiting on poll...\n" << ret;
		fds.revents = 0;
		ret = poll(&fds, 1, -1);
		std::cout << "poll ret=" << ret << " revents=" << fds.revents << "\n";

		if (ret < 0) {
			// something went wrong
		}

		if (fds.revents & (POLLHUP | POLLERR | POLLNVAL) ) {
			// something went wrong
		}

		// get some messages
		if (fds.revents & (POLLIN|POLLPRI)) {
			ret = iw.fetch_scan_events();
			if (ret < 0) {
				// something went wrong
			}

		}
		// back around for more
	}

	return 0;
}

