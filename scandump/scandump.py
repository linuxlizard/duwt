# davep 20190519 ; demo program to get json from scandump

import sys
import subprocess
import json

def scandump(ifname):
	s = subprocess.run(("./scandump", "--json", ifname), capture_output=True, check=True)

	dump_json = json.loads(s.stdout)
#	print(dump_json)

	for bss in dump_json["bss"]:
		print(f"found bss={bss['bssid']} SSID={bss['SSID']}")

if __name__ == '__main__':
	scandump(sys.argv[1])
