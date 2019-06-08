#!/usr/bin/env python3

import iw

iface = "wlan1"

print(iw.hello(iface))
print(iw.get_chanlist(iface))

print(iw.get_scan(iface))

