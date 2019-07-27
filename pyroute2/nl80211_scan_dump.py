#!/usr/bin/env python3

import sys
import struct
import logging

from pyroute2 import IPRoute

from pyroute2.iwutil import IW
from pyroute2.netlink import NLM_F_REQUEST
from pyroute2.netlink import NLM_F_DUMP
#from pyroute2.netlink.nl80211 import nl80211cmd
from pyroute2.netlink.nl80211 import NL80211_NAMES
from pyroute2.common import hexdump

import oui
import nl80211_scan
#from nl80211_scan import NL80211_GetScan, NL80211_BSS_ELEMENTS_VALUES, NL80211_BSS_ELEMENTS_NAMES

logger = logging.getLogger("scandump")

def print_ssid(ssid_dict):
    # Be VERY careful with the SSID!  Can contain hostile input.
    # For example, this print is vulnerable to an SSID with terminal escape
    # chars. https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
    ssid = ssid_dict["SSID"]
    if all([c == chr(0) for c in ssid]) or len(ssid)==0:
        # empty/null SSID
        print("\tSSID:")
    else:
        ssid_hex = ssid_dict["_hex"]
        print("\tSSID: {}\n\tSSID hex: {}".format(
              ssid, ssid_hex))

def print_supported_rates(supp_rates):
    s = nl80211_scan.Supported_Rates.pretty_print(supp_rates["_raw"])
    print("\tSupported rates:", s)

def print_extended_supported_rates(ext_supp_rates):
    s = nl80211_scan.Extended_Rates.pretty_print(ext_supp_rates["_raw"])
    print("\Extended Supported rates:", s)

def print_channel(ds_param):
    print("\tDS Parameter set: channel {}".format(ds_param["channel"]))

def print_ext_capabilities(extcapa):

    # TODO there are some VHT-only fields in here, I think
    s = "\n\t\t* ".join([k for k,v in extcapa.items() if v and k[0] != '_'])
    print("\tExtended capabilities:\n\t\t* " + s)


def print_country(country_dict):
    isocode = country_dict["Country"]
    environment = country_dict["Environment"]
    print("\tCountry: {}\tEnvironment: {}".format(isocode, environment))
    if not 'Subbands' in country_dict:
        print("\t\tNo country IE triplets present")
        return

    for triplet in country_dict['Subbands']:
        first = triplet["First Channel"]
        # can be zero
        if first:
            ch_max = first +  triplet["Number of Channels"] - 1
            dbm = triplet["Max TX Power (dBm)"]
            print("\t\tChannels [{0} - {1}] @ {2} dBm".format(
                first, ch_max, dbm))

def print_ht_capabilities(ht_capa_dict):
    print("\tHT capabilities:")
    # get actual value
    num, = struct.unpack("<H", ht_capa_dict["_raw"][:2])
    print("\t\tcapabilities: {0:#x}".format(num))

    # Capability Information field
    cap_info = ht_capa_dict["HT Capability Info"]
    for k,v in cap_info.items():
        # need special purpose print for some fields
        s = None
        if k == 'HT20/HT40':
            s = "%s" % nl80211_scan.HT_Capabilities.channel_width_str(v)
        elif k == "SM Power Save":
            # powersave:
            s = "%s" % nl80211_scan.HT_Capabilities.sm_power_save_str(v)
        elif k == "RX STBC":
            s = "%s" % nl80211_scan.HT_Capabilities.rx_stbc_str(v)
        elif k == "Max AMSDU length":
            s = "%s: %d" % (k, nl80211_scan.HT_Capabilities.max_amsdu_len(v))
        elif v:
            # assume it's a boolen bit field that's only printed if true
            s = "%s" % k
        if s:
            print("\t\t\t%s" % s)

    # TODO AMPDU print
    ampdu = ht_capa_dict["AMPDU Parameters"]

    # TODO print MCS indices
#    mcs = ht_capa_dict[]


def print_ht_operation(ht_oper_dict):
    print("\tHT operation:")

    # Primary Channel
    print("\t\t* primary channel: %d" % ht_oper_dict["Primary Channel"])

    # Information field
    info = ht_oper_dict["Information"]
    for k,v in info.items():
        # some fields need special interpretation
        if k == "secondary channel offset":
            s = "%s: %s" % (k, nl80211_scan.HT_Operation.secondary_channel_offset(v))
        elif k == "STA channel width":
            s = "%s: %s" % (k, nl80211_scan.HT_Operation.sta_channel_width(v))
        else:
            s = "%s: %d" % (k, v)

        print("\t\t* %s" % s)

    # MCS field TODO
    mcs = ht_oper_dict["Basic HT-MCS Set"]


def print_vht_capabilities(vht_capa_dict):
    # need a value for the 32-bit cap info field
    num, = struct.unpack("<L", vht_capa_dict["_raw"][:4])
    print("\tVHT capabilities:\n\t\tVHT Capabilities ({0:#0x}):".format(num))

    vht_cap_info = vht_capa_dict['VHT Capability Info']
    for k,v in vht_cap_info.items():
        if k[0] == '_':
            continue
        s = None
        if k == 'Max MPDU length':
            s = "%s: %d" % (k, nl80211_scan.VHT_Capabilities.max_mpdu_len(v))
        elif k == 'Supported Channel Width':
            s = "%s: %s" % (k,
                nl80211_scan.VHT_Capabilities.supported_chan_width_str(v))
        elif v:
            # we have a set bit so print the bit's name
            s = str(k)
        if s:
            print("\t\t\t%s" % s)

def print_vht_operation(vht_oper_dict):
    print("\tVHT operation:")
    oper_info = vht_oper_dict["Operation Info"]
    chwidth = oper_info["Channel Width"]
    print("\t\t* channel width: {} ({})".format(chwidth, nl80211_scan.VHT_Operation.channel_width_str(chwidth)))
    print("\t\t* center freq segment 1: {}".format(oper_info['Channel Center Frequency Segment 0']))
    print("\t\t* center freq segment 2: {}".format(oper_info['Channel Center Frequency Segment 1']))
    print("\t\t* VHT basic MCS set: {0:#06x}".format(vht_oper_dict['VHT-MSS and NSS Set']))

def print_rsn(rsn_dict):
    # 9.4.2.25 RSNE 80211_2016.pdf
    print("\tRSN:\t* Version: {}".format(rsn_dict["Version"]))
    group_cipher = rsn_dict['Group Cipher Suite']
    cipher_name, = [ v for k,v in group_cipher.items() if k == "Suite Name"]
    print("\t\t* Group cipher: {}".format(cipher_name))

    pair_cipher = rsn_dict['Pairwise Cipher Suite']
    cipher_names = [v for cipher in pair_cipher for k,v in cipher.items() if k== 'Suite Name' ]
    print("\t\t* Pairwise ciphers: {}".format(" ".join(cipher_names)))

    akm_suite = rsn_dict['AKM Suite']
    cipher_names = [v for akm in akm_suite for k,v in akm.items() if k == "Suite Name"]
    print("\t\t* Authentication suites: {}".format(" ".join(cipher_names)))

    # slide the PTKSA and GTKSA fields in between to match how the 
    # iw scan.c _print_rsn_ie() does decode
    def capa_str(name, value):
        # a few special cases
        if name == "PTKSA-RC" or name == "GTKSA-RC":
            return "{}-{}".format(2**value, name)
        if value:
            return name
        return None

    rsn_capa = rsn_dict['RSN Capabilities']
    capa_list = [capa_str(k,v) for k,v in rsn_capa.items() if v is not None]
    capa_list = [v for v in capa_list if v is not None]
    print("\t\t* Capabilities: {}".format(" ".join(capa_list)))

def print_bss(bss):
    # NOTE: the contents of beacon and probe response frames may or may not
    # contain all these fields.  Very likely there could be a keyerror in the
    # following code. Needs a bit more bulletproofing.

    # print like 'iw dev $dev scan dump"
    print("BSS {}".format(bss['NL80211_BSS_BSSID']))
    print("\tTSF: {0[VALUE]} ({0[TIME]})".format(bss['NL80211_BSS_TSF']))
    print("\tfreq: {}".format(bss['NL80211_BSS_FREQUENCY']))
    print("\tcapability: {}".format(
        bss['NL80211_BSS_CAPABILITY']['CAPABILITIES']))
    print("\tsignal: {0[VALUE]} {0[UNITS]}".format(
        bss['NL80211_BSS_SIGNAL_MBM']['SIGNAL_STRENGTH']))
    print("\tlast seen: {} ms ago".format(bss['NL80211_BSS_SEEN_MS_AGO']))

    # each IE should be an instance of nl80211.IE
    ies = bss['NL80211_BSS_INFORMATION_ELEMENTS']

    ie_printers = (
        ("NL80211_BSS_ELEMENTS_SSID", print_ssid),
        ("NL80211_BSS_ELEMENTS_SUPPORTED_RATES", print_supported_rates),
        ("NL80211_BSS_ELEMENTS_CHANNEL", print_channel),
        ("NL80211_BSS_ELEMENTS_COUNTRY", print_country),
        ("NL80211_BSS_ELEMENTS_EXTENDED_RATES", print_extended_supported_rates),
        ("NL80211_BSS_ELEMENTS_HT_CAPABILITIES", print_ht_capabilities),
        ("NL80211_BSS_ELEMENTS_HT_OPERATION", print_ht_operation),
        ("NL80211_BSS_ELEMENTS_EXT_CAPABILITIES", print_ext_capabilities),
        ("NL80211_BSS_ELEMENTS_VHT_CAPABILITIES", print_vht_capabilities),
        ("NL80211_BSS_ELEMENTS_VHT_OPERATION", print_vht_operation),
        ("NL80211_BSS_ELEMENTS_RSN", print_rsn),
        # TODO more IE decodes
    )

    for msg_type, printer in ie_printers:
        try:
            ies[msg_type]
        except KeyError:
            # no way to print this
            pass
        else:
            printer(ies[msg_type])


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
#    msg['cmd'] = NL80211_NAMES['NL80211_CMD_GET_SCAN']
#    msg['attrs'] = [['NL80211_ATTR_IFINDEX', ifindex]]

    scan_dump = iw.nlm_request(msg, msg_type=iw.prid,
                               msg_flags=NLM_F_REQUEST | NLM_F_DUMP)

    for network in scan_dump:
        for attr in network['attrs']:
            if attr[0] == 'NL80211_ATTR_BSS':
                # handy debugging; see everything we captured
                for bss_attr in attr[1]['attrs']:
                    logger.debug("bss attr=%r", bss_attr)

                bss = dict(attr[1]['attrs'])
                print_bss(bss)

    iw.close()


if __name__ == '__main__':
    # interface name to dump scan results
    logging.basicConfig(level=logging.INFO)
#    logging.basicConfig(level=logging.DEBUG)

#    logger.setLevel(level=logging.DEBUG)
    logger.setLevel(level=logging.INFO)

#    logging.getLogger("pyroute2").setLevel(level=logging.DEBUG)

    ifname = sys.argv[1]
    main(ifname)
