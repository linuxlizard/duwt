#!/usr/bin/env python3

import sys
import logging

from pyroute2 import IPRoute

from pyroute2.iwutil import IW
from pyroute2.netlink import NLM_F_REQUEST
from pyroute2.netlink import NLM_F_DUMP
from pyroute2.netlink.nl80211 import nl80211cmd
from pyroute2.netlink.nl80211 import NL80211_NAMES
from pyroute2.common import hexdump

logging.basicConfig(level=logging.DEBUG)

logger = logging.getLogger("scandump")
logger.setLevel(level=logging.DEBUG)
# logger.setLevel(level=logging.INFO)

logging.getLogger("pyroute2").setLevel(level=logging.DEBUG)

def print_ssid(ssid):
    # Be VERY careful with the SSID!  Can contain hostile input.
    # For example, this print is vulnerable to an SSID with terminal escape
    # chars. https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
    if len(ssid.data):
        ssid_hex = hexdump(ssid.data)
        print("\tSSID: {}\n\tSSID hex: {}".format(
              ssid.pretty_print(), ssid_hex))
    else:
        # empty/null SSID
        print("\tSSID:")

def print_supported_rates(supp_rates):
    print("\tSupported rates: {}".format(supp_rates.pretty_print()))

def print_extended_supported_rates(ext_supp_rates):
    print("\Extended Upported rates: {}".format(ext_supp_rates.pretty_print()))

def print_channel(channel):
    print("\tDS Parameter set: {}".format(channel.pretty_print()))

def print_ext_capabilities(extcapa):
    if not extcapa:
        return

    # TODO there are some VHT-only fields in here, I think
    s = "\n\t\t* ".join([ec.name for ec in extcapa if ec.value])
    print("\tExtended capabilities:\n\t\t* " + s)


def print_country(country):
    isocode = country[0].value
    environment = country[1].value
    print("\tCountry: {}\tEnvironment: {}".format(isocode, environment))
    if len(country.fields) == 2:
        print("\t\tNo country IE triplets present")
        return

    for triplet in country[2].value:
        if triplet[0].name == "First Channel":
            print("\t\tChannels [{0} - {1}] @ {2} dBm".format(
                triplet[0].value, triplet[0].value+triplet[1].value-1, triplet[2].value))

def print_ht_capabilities(ht_capa):
    if not ht_capa:
        return

    print("\tHT capabilities:")
    # TODO get actual value
    print("\t\tcapabilities: -1")

    # Capability Information field
    _, ht_cap_info = ht_capa[0]
    for i, field in enumerate(ht_cap_info):
        if field is None:
            # ignore reserved bits
            continue

        # need special purpose print for some fields
        s = None
        if i == 1:
            s = "%s" % ht_capa.channel_width_str(field.value)
        elif i == 2:
            # powersave:
            s = "%s" % ht_capa.sm_power_save_str(field.value)
        elif i == 8:
            s = "%s" % ht_capa.rx_stbc_str(field.value)
        elif i == 11:
            s = "%s: %d" % (field[0], ht_capa.max_amsdu_len(field.value))
        elif field.value:
            # assume it's a boolen bit field that's only printed if true
            s = "%s" % field.name
        if s:
            print("\t\t\t%s" % s)

    # TODO AMPDU print
    _, ampdu = ht_capa[1]

    # TODO print MCS indices
    _, mcs = ht_capa[2]


def print_ht_operation(ht_capa):
    if not ht_capa:
        return

    print("\tHT operation:")

    # Primary Channel
    print("\t\t* %s: %d" % (ht_capa[0].name, ht_capa[0].value))

    # Information field
    info = ht_capa[1].value
    for idx, field in enumerate(info):
        # reserved fields are None
        if field is None:
            continue
        # some fields need special interpretation
        if idx == 0:
            s = "%s: %s" % (field.name,
                            ht_capa.secondary_channel_offset(field.value))
        elif idx == 1:
            s = "%s: %s" % (field.name, ht_capa.sta_channel_width(field.value))
        else:
            s = "%s: %d" % (field.name, field.value)

        print("\t\t* %s" % s)

    # MCS field TODO
    mcs = ht_capa[2]
    _ = mcs


def print_vht_capabilities(vht_capa):
    if not vht_capa:
        return

    # TODO need a value for the 32-bit cap info field
    print("\tVHT capabilities:\n\t\tVHT Capabilities (-1):")

    _, vht_cap_info = vht_capa[0]
    for i, field in enumerate(vht_cap_info):
        if field is None:
            continue

        s = None
        if i == 0:
            s = "%s: %d" % (field[0], vht_capa.max_mpdu_len(field[1]))
        if i == 2:
            s = "%s: %s" % (
                field[0],
                vht_capa.supported_chan_width_str(field[1]))
        elif field[1]:
            s = field[0]
        if s:
            print("\t\t\t%s" % s)

def print_vht_operation(vht_oper):
    if not vht_oper:
        return

    print("\tVHT operation:")
    oper_info = vht_oper.fields[0]
    chwidth = oper_info.value[0].value
    print("\t\t* channel width: {} ({})".format(chwidth, vht_oper.channel_width_str(chwidth)))
    print("\t\t* center freq segment 1: {}".format(oper_info.value[1].value))
    print("\t\t* center freq segment 2: {}".format(oper_info.value[2].value))
    print("\t\t* VHT basic MCS set: {0:#06x}".format(vht_oper.fields[1].value))

def print_rsn(rsn):
    # 9.4.2.25 RSNE 80211_2016.pdf
    print("\tRSN:\t* Version: {}".format(rsn.fields[0].value))
    group_cipher = rsn.fields[1].value
    cipher_name, = [ v for v in group_cipher if v.name == "Suite Name"]
    print("\t\t* Group cipher: {}".format(cipher_name.value))

    pair_cipher = rsn.fields[3].value
    cipher_names = [v.value for pair in pair_cipher for v in pair if v.name == "Suite Name"]
    print("\t\t* Pairwise ciphers: {}".format(" ".join(cipher_names)))

    akm_suite = rsn.fields[5].value
    cipher_names = [v.value for akm in akm_suite for v in akm if v.name == "Suite Name"]
    print("\t\t* Authentication suites: {}".format(" ".join(cipher_names)))

    # slide the PTKSA and GTKSA fields in between to match how the 
    # iw scan.c _print_rsn_ie() does decode
    def capa_str(field):
        # a few special cases
        if field.name == "PTKSA-RC" or field.name == "GTKSA-RC":
            return "{}-{}".format(2**field.value, field.name)
        if field.value:
            return field.name
        return None

    rsn_capa = rsn.fields[6]
    capa_list = [capa_str(v) for v in rsn_capa.value if v is not None]
    capa_list = [v for v in capa_list if v is not None]
    print("\t\t* Capabilities: {} ({})".format(" ".join(capa_list), -1))

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

    for p in ie_printers:
        try:
            p[1](ies[p[0]])
        except KeyError:
            pass


def main(ifname):
    iw = IW()

    ip = IPRoute()
    ifindex = ip.link_lookup(ifname=ifname)[0]
    ip.close()

    # CMD_GET_SCAN doesn't require root privileges.
    # Can use 'nmcli device wifi' or 'nmcli d w' to trigger a scan which will
    # fill the scan results cache for ~30 seconds.
    # See also 'iw dev $yourdev scan dump'
    msg = nl80211cmd()
    msg['cmd'] = NL80211_NAMES['NL80211_CMD_GET_SCAN']
    msg['attrs'] = [['NL80211_ATTR_IFINDEX', ifindex]]

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
    ifname = sys.argv[1]
    main(ifname)
