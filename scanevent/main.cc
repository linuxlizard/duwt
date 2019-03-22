/* Simple CLI program to monitor scan cfg80211 netlink events
 *
 */
#include <iostream>
#include <vector>

#include "netlink.hh"

int main(void)
{
	cfg80211::Cfg80211 iw;

	std::array<uint8_t, 10> buf {};
	auto ie = cfg80211::IE(255,12, buf.data());

	std::vector<cfg80211::BSS> bss_list;
	iw.get_scan("wlp1s0", bss_list);

	for ( auto&& bss : bss_list ) {
		std::cout << "found BSS " << bss << " num_ies=" << bss.ie_list.size() << "\n";
		std::cout << "SSID=" << bss.get_ssid() << "\n";
		for (auto&& ie : bss.ie_list) {
//			std::cerr << "ie=" << ie << std::endl;
			std::cout << "ie=" << ie << "\n";
		}
	}

	return 0;
}

