#include <iostream>

#include "netlink.hh"

int main(void)
{
	Netlink netlink;

	netlink.get_scan("wlp1s0");

	return 0;
}

