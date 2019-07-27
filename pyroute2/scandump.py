import sys
import logging

from pyroute2 import IPRoute

from pyroute2.iwutil import IW
from pyroute2.netlink import NLM_F_REQUEST
from pyroute2.netlink import NLM_F_DUMP
from pyroute2.netlink.nl80211 import nl80211cmd
from pyroute2.netlink.nl80211 import NL80211_NAMES
from pyroute2.common import hexdump

from nl80211_scan import NL80211_GetScan

logger = logging.getLogger("scandump")

def print_bss(bss):

    # print BSS in a single line
    ies = bss['NL80211_BSS_INFORMATION_ELEMENTS']

    ssid = ies['NL80211_BSS_ELEMENTS_SSID']["SSID"]
    
    # CAREFULLY sanity check the string
    # first check for null SSIDs (some devices will tx NULL SSID instead of zero-length SSID)
    # warning: SSIDs with terminal control characters will screw with us
    if all([c == chr(0) for c in ssid]) or len(ssid)==0:
        # obviously someone could create an SSID with this name
        ssid = "<hidden>"

    # keep the ssid a little short
    if len(ssid) >= 16:
        ssid = ssid[:13] + "..."

    try:
        channel = ies['NL80211_BSS_ELEMENTS_CHANNEL']["channel"]
    except KeyError:
        channel = -1

    # TODO
    rate = "??M"
    sn = "??:?"  # signal/noise
    interval = "???" # TSF
    caps = "??" # capabilities

    ie_nums = [v["_ID"] for k,v in ies.items() if k not in ("TODO", "NL80211_BSS_ELEMENTS_VENDOR")]
    try:
        ie_nums.extend(ies['TODO'])
    except KeyError:
        pass
    ie_nums.sort()

    s =  " ".join(["%3d"%n for n in ie_nums])

    print("{0:16s} {1} {2:4d}  {3:4s} {4}  {5}  {6}    {7}".format(
        ssid,
        bss['NL80211_BSS_BSSID'],
        channel,
        rate, 
        sn, 
        interval, 
        caps,
        s
        ))

def main(ifname):
    iw = IW()

    ip = IPRoute()
    ifindex = ip.link_lookup(ifname=ifname)[0]
    ip.close()

    msg = NL80211_GetScan(ifindex)
#    msg['cmd'] = NL80211_NAMES['NL80211_CMD_GET_SCAN']
#    msg['attrs'] = [['NL80211_ATTR_IFINDEX', ifindex]]

    scan_dump = iw.nlm_request(msg, msg_type=iw.prid,
                               msg_flags=NLM_F_REQUEST | NLM_F_DUMP)

    print("SSID             BSSID              CHAN RATE  S:N   INT CAPS")
    for network in scan_dump:
        for attr in network['attrs']:
            if attr[0] == 'NL80211_ATTR_BSS':
                bss = dict(attr[1]['attrs'])
                print_bss(bss)

if __name__ == '__main__':
    logging.basicConfig(level=logging.ERROR)
#    logging.basicConfig(level=logging.INFO)
#    logging.basicConfig(level=logging.DEBUG)

#    logger.setLevel(level=logging.DEBUG)
    logger.setLevel(level=logging.INFO)

#    logging.getLogger("pyroute2").setLevel(level=logging.INFO)
#    logging.getLogger("pyroute2").setLevel(level=logging.DEBUG)

    # interface name to dump scan results
    ifname = sys.argv[1]
    main(ifname)

