#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>

#include "logging.h"
#include "cfg80211.h"

#define PORT 8888

// https://www.gnu.org/software/libmicrohttpd/tutorial.html
int answer_to_connection (void *cls, struct MHD_Connection *connection,
                          const char *url,
                          const char *method, const char *version,
                          const char *upload_data,
                          size_t *upload_data_size, void **con_cls)
{
	const char *page  = "<html><body>Hello, browser!</body></html>";
	struct MHD_Response *response;
	int ret;

	response = MHD_create_response_from_buffer (strlen (page),
                                            (void*) page, MHD_RESPMEM_PERSISTENT);

	ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
	std::cout << __func__ << " " << ret << "\n";
	MHD_destroy_response (response);

	return ret;
}

int main(int argc, char* argv[] )
{
	struct MHD_Daemon *daemon;

	if (argc != 2) {
		fprintf(stderr, "usage: %s ifname\n", argv[0]);
		exit(1);
	}

	logging_init("scandump.log");
	auto logger = spdlog::get("main");
	logger->info("hello, world");

	// TODO use getopt 
	const char* ifname = argv[1];

	std::vector<cfg80211::BSS> bss_list;
	try {
		cfg80211::Cfg80211 iw;
		iw.get_scan(ifname, bss_list);
	}
	catch (cfg80211::NetlinkException& err) {
		logger->error(err.what());
		std::terminate();
	}

	daemon = MHD_start_daemon (MHD_USE_INTERNAL_POLLING_THREAD, PORT, NULL, NULL,
                             &answer_to_connection, NULL, MHD_OPTION_END);
	if (NULL == daemon) return 1;

	getchar ();

	MHD_stop_daemon (daemon);

	return 0;
}

