#!/usr/bin/env python3

import sys
import time
import faulthandler

import pprint 

import iw

iface = "wlan1"
iface = "wlp1s0"
iface = sys.argv[1]

#print(iw.hello(iface))
print(iw.get_chanlist(iface))

scan_dump = iw.get_scan(iface)

pp = pprint.PrettyPrinter(width=128)
pp.pprint(scan_dump)

for bss in scan_dump:
    print("BSS {}".format(bss["BSSID"]))
    print("\tTSF: {} usec {}".format(bss["TSF"], iw.decode_tsf(bss["TSF"])))
    print("\tfreq: {}".format(bss["frequency"]))
    print("\tcapability: {} ({:#06x})".format(
        " ".join(iw.decode_capabilities(bss["capability"])),
        bss["capability"]))
    print("\tsignal: {} {}".format(
        bss["signal_strength"]["value"], 
        bss["signal_strength"]["units"]))
    print("\tlast seen: {} ms ago".format(bss["last_seen_ms"]))
    print("\tSSID: {}".format(bss["ie"][0]["SSID"]))
    print("\tDS Parameter set: (TODO)")
    print(bss.keys())

#faultfile = open("/tmp/dave.txt","w")
#faulthandler.register(15, faultfile)
#faulthandler.register(2, faultfile)
#
print("waiting on event_recv...")
fd = iw.event_recv()
print(fd)

#while 1:
#	print("foo")
#	time.sleep(5)

