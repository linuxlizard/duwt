#!/usr/bin/env python3

import pprint 

import iw

iface = "wlan1"

print(iw.hello(iface))
print(iw.get_chanlist(iface))

scan_dump = iw.get_scan(iface)

pp = pprint.PrettyPrinter()
pp.pprint(scan_dump)

