#include <iostream>
#include <vector>

#include "netlink.hh"

int main(void)
{
	Cfg80211 cfg80211;

	std::vector<BSS> bss_list;
	cfg80211.get_scan("wlp1s0", bss_list);

	for ( auto&& bss : bss_list ) {
//	for ( auto bss=std::cbegin(bss_list) ; bss < std::cend(bss_list) ; ++bss) {
		std::cout << "found BSS " << bss << " num_ies=" << bss.ie_list.size() << "\n";
		for (auto&& ie : bss.ie_list) {
			std::cout << "ie=" << ie << "\n";
		}
		for (auto ie=std::cbegin(bss.ie_list) ; ie<std::cend(bss.ie_list) ; ++ie ) {
			std::cout << "ie=" << *ie << "\n";
		}

	}

	return 0;
}

