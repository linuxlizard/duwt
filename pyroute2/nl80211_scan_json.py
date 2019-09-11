import sys
import json
import logging
import datetime
import pprint

import pyroute2
from pyroute2 import IPRoute

from pyroute2.iwutil import IW
from pyroute2.netlink import NLM_F_REQUEST
from pyroute2.netlink import NLM_F_DUMP
import pyroute2.netlink.nl80211 as nl80211
from pyroute2.netlink.nl80211 import nl80211cmd
from pyroute2.netlink.nl80211 import NL80211_NAMES
#from pyroute2.netlink.nl80211 import NL80211_BSS_ELEMENTS_VALUES, NL80211_BSS_ELEMENTS_NAMES
from pyroute2.common import hexdump
from pyroute2.common import load_dump

import oui
import nl80211_scan
#from nl80211_scan import NL80211_GetScan, NL80211_BSS_ELEMENTS_VALUES, NL80211_BSS_ELEMENTS_NAMES

logger = logging.getLogger("scanjson")
logger.setLevel(level=logging.DEBUG)
# logger.setLevel(level=logging.INFO)

logging.getLogger("pyroute2").setLevel(level=logging.DEBUG)

def peek(scan_dump):
    # experimntal code to learn what's in the scan dump
    # scan_dump is array of nl80211cmd
    # +-nl80211cmd 
    #   +---genlmsg 
    #       +---nlmsg
    #           +---nlmsg_atoms
    #               +---nlmsg_base
    #                   +---dict
    #
    print("scan_dump is {} of len={}".format(type(scan_dump), len(scan_dump)))
    for network in scan_dump:
        print("network {} is {}".format(id(network), type(network)))
        print("network has", dir(network))
        print("network keys=", network.keys())
        print("network size={} data id={} len={} offset={}".format(network.get_size(), id(network.data), len(network.data), network.offset))
        print("network value=", network.value)
        print("network header=", network['header'])
        print("network fields=", network.fields)
#        print(network.sql_schema())
#        print(network.dump())
        for field in network.items():
            # field will be a tuple of key:value
            # the key can be 'attrs', 'header', 'event', etc
            key, value = field
            print("network field {} is {}".format(key, type(value)))

        bss = None
        for attr in network['attrs']:
            # array of key/value tuple
            # key will be NL80211_ATTR_xxx
            # value varies depending on which attribute
            # nl80211cmd.nla_map is the name->value
            # e.g, 
            # NL80211_ATTR_BSS is an instance of nl80211.nl80211cmd.bss 
            key, value = attr
            print("network attr {} is {}".format(key, type(value)))

            # save the BSS for later use
            if key == 'NL80211_ATTR_BSS':
                bss = value

        # now let's peer into the BSS
        # bss
        # +-nla (netlink attribute)
        #   +---nla_base
        #       +---nlmsg_base
        #       |   +---dict
        #       |
        #       nlmsg_atoms
        #       +---nlmsg_base
        #           +---dict
        #
        print("BSS is {}".format(type(bss)))
        print("BSS has ", dir(bss))
#        print("BSS={}".format(bss))

        bss_dict = {}
        # bss contains 'attrs' which is the meaty goodness we want
        for attr in bss['attrs']:
            # key should be one of NL80211_BSS_xxx
            key, value = attr
            print("BSS ATTR {} is {}".format(key, type(value)))


def save(scan_dump, dumpfilename):
    # I can save an entire dump in one file!
    # The scan_dump is an array but each element holds a ref to the blob and an
    # offset into the blob. So just need to save the blob.
    with open(dumpfilename,"w") as outfile:
        print(hexdump(scan_dump[0].data), file=outfile)

    with open(dumpfilename+".bin","wb") as outfile:
        outfile.write(scan_dump[0].data)


def load(dumpfilename):
    with open(dumpfilename, "r") as infile:
        data = load_dump(infile)

    print("load len={}".format(len(data)))

    # from tests/decoder/decoder.py
    offset = 0
    scan_dump = []
    counter = 0
    while offset < len(data):
        print("counter={} offset={} len={}".format(counter, offset, len(data)))
        msg = nl80211cmd(data[offset:])
        msg.decode()

        offset += msg['header']['length']
        counter += 1    
        scan_dump.append(msg)

    return scan_dump

def json_from_ie(iename, ie):
    iename.islower() # verify is string
    logger.debug("IE %s is %s", iename, type(ie))

    # list ?
    if isinstance(ie,list):
        return [json_from_ie(iename, v) for v in ie]
    else:
        # expect a dict built from nl80211_scan.IE 
        ie["_ID"]

    d = { "name": nl80211_scan.NL80211_BSS_ELEMENTS_VALUES[ie["_ID"]],
          "id": ie["_ID"],
          "len": len(ie["_raw"]),
          "value": {k:v for k,v in ie.items() if k[0] != '_'},
          "hex": ie["_hex"],
        }
#    pprint.pprint(d, width=128)
    return d


def to_json(scan_dump):
    for network in scan_dump:
        bss = None

        for attr in network['attrs']:
            # 'attrs' is array of key/value tuple
            # key will be NL80211_ATTR_xxx
            # value varies depending on which attribute
            # nl80211cmd.nla_map is the name->value
            # e.g, 
            # NL80211_ATTR_BSS is an instance of nl80211.nl80211cmd.bss 
            key, value = attr

            # save the BSS for later use
            if key == 'NL80211_ATTR_BSS':
                bss = value

        bss_dict = {}
        # bss contains 'attrs' which is the meaty goodness we want
        for attr in bss['attrs']:
            # key should be one of NL80211_BSS_xxx
            key, value = attr
            logger.debug("BSS ATTR %s is %s", key, type(value))

            if key == "NL80211_BSS_BEACON_IES" or key == "NL80211_BSS_INFORMATION_ELEMENTS":
                # IEs can come from either a beacon or a probe response
                ies = value
                name = "beacon_ies" if key == "NL80211_BSS_BEACON_IES" else "ies"

                # filter out my fake "TODO" element
                bss_dict[name] = { nl80211_scan.NL80211_BSS_ELEMENTS_NAMES[k]:json_from_ie(k, ies[k]) for k in ies.keys() if k != "TODO" }

            elif key == "NL80211_BSS_BSSID":
                bss_dict["bssid"] = value

            elif key == "NL80211_BSS_FREQUENCY":
                bss_dict["frequency"] = value

            elif key == "NL80211_BSS_BEACON_TSF":
                # TSF is stored as the original 64-bit integer
                decode = datetime.timedelta(microseconds=value)
                bss_dict["tsf"] = { "value": value, "units" : "us", "time": str(decode) }

            elif key == "NL80211_BSS_SEEN_MS_AGO":
                bss_dict["seen"] = {"value": value, "units": "ms"}

            elif key == "NL80211_BSS_SIGNAL_MBM":
                bss_dict["signal"] = {"value": value['SIGNAL_STRENGTH']['VALUE'], "units": "dBm"}

        yield bss_dict


def test_json_encode(dumpfilename):
    scan_dump = load(dumpfilename)
    print("loaded len=%d networks" % len(scan_dump))

#    for network in scan_dump:
#        print("cmd={} version={}".format(network['cmd'], network['version']))

    jsonator = to_json(scan_dump)
    d = {n["bssid"]:n for n in jsonator}

    # test individual fields as json-able before trying to save the whole thing
    for bssid,network in d.items():
        print("BSSID={}".format(bssid))
        for ies in ("beacon_ies", "ies"):
            if ies in network:
                for k,v in network[ies].items():
                    print("bssid {} {} {}={}".format(bssid, ies, k, v))
                    json.dumps(v)

        json.dumps(network)
            
    with open("out.json", "w") as outfile:
        print(json.dumps(d), file=outfile)


def test_save(scan_dump, outfilename):
    save(scan_dump, outfilename)
    new_scan_dump = load(outfilename)
    print("saved len=%d networks" % len(scan_dump))
    print("loaded len=%d networks" % len(new_scan_dump))

    for foo in zip(scan_dump, new_scan_dump):
        print(foo[0] == foo[1])
        print(foo[0]['header'], foo[1]['header'])

def test_oui():
    import time
    for s in ("00-30-44", "00-40-68"):
        start = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        vendor = oui.vendor_lookup(s)
        end = time.clock_gettime_ns(time.CLOCK_MONOTONIC)
        print(vendor)

def test_ssid():
    # check for evil ssid decoding
    import struct
    evil_ssid = ("hello, world", 
                 "", 
                "\x00\x00\x00\x00\x00\x00", 
                "(╯°□°）╯︵ ┻━┻",
                "01234567890123456789012345678901")
    for ssid in evil_ssid:
        print(len(ssid), ssid)

        buf = bytes(ssid,"utf8")
        print(buf)
        ie_id = 1
        data = struct.pack("%ds"%len(buf), buf)
        ssid_ie = nl80211.SSID(data)
        ssid_ie.decode()
        new_ssid = ssid_ie.pretty_print()
        print(len(new_ssid), new_ssid)
        assert len(ssid) == len(new_ssid), (len(ssid), len(new_ssid))

        assert ssid == new_ssid, (hexdump(buf), ssid_ie.fields.value)
        
def run(ifname):
    iw = IW()

    ip = IPRoute()
    ifindex = ip.link_lookup(ifname=ifname)[0]
    ip.close()

    # CMD_GET_SCAN doesn't require root privileges.
    # Can use 'nmcli device wifi' or 'nmcli d w' to trigger a scan which will
    # fill the scan results cache for ~30 seconds.
    # See also 'iw dev $yourdev scan dump'
    msg = nl80211_scan.NL80211_GetScan(ifindex)

    scan_dump = iw.nlm_request(msg, msg_type=iw.prid,
                               msg_flags=NLM_F_REQUEST | NLM_F_DUMP)

    jsonator = to_json(scan_dump)
    return json.dumps({n["bssid"]:n for n in jsonator})

def main(ifname):
    iw = IW()

    ip = IPRoute()
    ifindex = ip.link_lookup(ifname=ifname)[0]
    ip.close()

    # CMD_GET_SCAN doesn't require root privileges.
    # Can use 'nmcli device wifi' or 'nmcli d w' to trigger a scan which will
    # fill the scan results cache for ~30 seconds.
    # See also 'iw dev $yourdev scan dump'
    msg = nl80211_scan.NL80211_GetScan(ifindex)

    scan_dump = iw.nlm_request(msg, msg_type=iw.prid,
                               msg_flags=NLM_F_REQUEST | NLM_F_DUMP)

#    peek(scan_dump)
#    test_save(scan_dump, "iw_scan_dump_rsp")
#    return

    jsonator = to_json(scan_dump)
    with open("out.json", "w") as outfile:
        print(json.dumps({n["bssid"]:n for n in jsonator}), file=outfile)

if __name__ == '__main__':
#    logging.basicConfig(level=logging.INFO)
    logging.basicConfig(level=logging.DEBUG)

    logger.setLevel(level=logging.DEBUG)
#    logger.setLevel(level=logging.INFO)

#    logging.getLogger("pyroute2").setLevel(level=logging.DEBUG)

#    test_oui()
#    test_ssid()
#    test_json_encode("iw_scan_dump_rsp")

    # interface name to dump scan results
    ifname = sys.argv[1]
    main(ifname)
