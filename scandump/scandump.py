# davep 20190519 ; demo program to get json from scandump
#
# Note: must be portable back to Python3.5

import sys
import subprocess
import json

def scandump(ifname):
	s = subprocess.run(("./scandump", "--json", ifname), stdout=subprocess.PIPE, check=True)

	dump_json = json.loads(s.stdout.decode("utf8"))

	header = "                               SSID  BSSID              dBm  Freq   Ch Seen"
	print(header)
	for bss in dump_json["bss"]:
		last_seen = bss['last_seen_ms'] / 1000.0
		units = "s"
		if last_seen > 60:
			last_seen /= 60
			units = "m"
		try:
			channel = bss['channel']
		except KeyError:
			channel = 0
		print("{0[SSID]!r:>36s} {0[bssid]} {0[signal_strength]} {0[frequency]} {1:4d} {2}{3}".format(
			bss, channel, last_seen, units))

def read_json_dump(infilename):
	with open(infilename,"r") as infile:
		dump_json = json.loads(infile.read())

	for bss in dump_json["bss"]:
		channel = 0
		for ie in bss["ie_list"]:
			if ie["id"] == 3:
				# DSSS parameter set
				channel = int(ie["hex"], 16)
		print("found bss={0[bssid]} SSID={0[SSID]} channel={1}".format(bss, channel))

if __name__ == '__main__':
	scandump(sys.argv[1])
#	read_json_dump(sys.argv[1])
