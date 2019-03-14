#include <iostream>
#include <vector>

#include "netlink.hh"

int main(void)
{
	Cfg80211 cfg80211;

	std::vector<BSS> bss_list;
	cfg80211.get_scan("wlp1s0", bss_list);

	for (auto bss : bss_list) {
		std::cout << "found BSS " << bss << "\n";
	}

	return 0;
}

