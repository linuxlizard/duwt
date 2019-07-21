import struct
import datetime
import collections
import logging

from pyroute2.netlink import nla_base
from pyroute2.common import map_namespace
from pyroute2.netlink.nl80211 import nl80211cmd
from pyroute2.common import hexdump

import oui

log = logging.getLogger("nl80211_scan")

NL80211_BSS_ELEMENTS_SSID = 0
NL80211_BSS_ELEMENTS_SUPPORTED_RATES = 1
NL80211_BSS_ELEMENTS_CHANNEL = 3
NL80211_BSS_ELEMENTS_TIM = 5
NL80211_BSS_ELEMENTS_COUNTRY = 7
NL80211_BSS_ELEMENTS_HT_CAPABILITIES = 45
NL80211_BSS_ELEMENTS_RSN = 48
NL80211_BSS_ELEMENTS_EXTENDED_RATE = 50
NL80211_BSS_ELEMENTS_HT_OPERATION = 61
NL80211_BSS_ELEMENTS_EXT_CAPABILITIES = 127
NL80211_BSS_ELEMENTS_VHT_CAPABILITIES = 191
NL80211_BSS_ELEMENTS_VHT_OPERATION = 192
NL80211_BSS_ELEMENTS_VENDOR = 221
(NL80211_BSS_ELEMENTS_NAMES, NL80211_BSS_ELEMENTS_VALUES) =\
    map_namespace('NL80211_BSS_ELEMENTS_', globals())

BSS_MEMBERSHIP_SELECTOR_HT_PHY = 127
BSS_MEMBERSHIP_SELECTOR_VHT_PHY = 126

OUI_IEEE = "00-0f-ac"
OUI_MSFT = "00-50-f2"
OUI_WFA = "50-6f-9a"

class IE:
    # note: using the raw integers in the descendents so can keep the hierarchy
    # in numerical order
    ID = -1

    Grammar = collections.namedtuple("Grammar",
                                     ("offset", "mask", "name"))

    Value = collections.namedtuple("Value",
                                   ("name", "value",))

    def __init__(self, data):
        # data is the buffer
        # Caller is responsible for parsing:
        #   id: octet
        #   len: octet
        #   data: octet(s) <-- we get this
        # and sending us the data
        self.data = data

        # array of IE.Value
        self.fields = []

    def __getitem__(self, idx):
        return self.fields[idx]

    def format_oui(self, data):
        return "%02x-%02x-%02x" % (data[0], data[1], data[2])

    def decode_integer(self, grammar, value):
        """An Information element consisting of a bit fields that fits into an
        intrinsic integer type. Straightforward to decode.
        """
        decode = [None] * (grammar[-1].offset + 1)
        for field in grammar:
            num = (value >> field.offset) & field.mask
            log.debug("num=%r name=%r", num, field.name)
            decode[field.offset] = IE.Value(field.name, num)
        return decode

    def decode(self):
        """IE child will decode its contents into self.fields."""
        pass

    def pretty_print(self):
        """IE child will decode its self.fields into a human useful value"""
        return ""


class SSID(IE):
    ID = 0

    def decode(self):
        # Be VERY careful with the SSID. Can contain hostile input.
        # SQL injection. Terminal characters. HTML injection. XSS. etc.
        fmt = '%is' % len(self.data)
        self.fields = IE.Value("SSID", struct.unpack(fmt, self.data)[0])

    def pretty_print(self):
        # TODO utf8 encoding of SSID is optional. Shouldn't be unconditionally 
        # treating as UTF8
        #
        try:
            s = self.fields.value.decode("utf8")
            # TODO check for unprintable chars (how badly will this hose unicode????)
            return s
        except UnicodeDecodeError:
            msg = "(error! SSID is invalid unicode) " + hexdump(self.fields.value)
            log.error(msg)
            return msg

class Supported_Rates(IE):
    ID = 1

    def _decode_byte(self, byte):
        # iw scan.c print_supprates()
        r = byte & 0x7f
        if r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY and byte & 0x80:
            # support for mandatory features required
            rate = "VHT"
        elif r == BSS_MEMBERSHIP_SELECTOR_HT_PHY and byte & 0x80:
            # support for mandatory features required
            rate = "HT"
        else:
            rate = float("%d.%d" % (r / 2, 5 * (r & 1)))

        required = bool(byte & 0x80)

        return (rate,required)
        
    def decode(self):
        # Supported Rates and BSS Membership Selectors
        # 9.4.2.3 80211-2016.pdf
        #
        # "The Supported Rates and BSS Membership Selectors element and
        # Extended Supported Rates and BSS Membership Selectors element in
        # Beacon and Probe Response frames is used by STAs in order to avoid
        # associating with a BSS if they do not support all of the data rates
        # in the BSSBasicRateSet parameter or all of the BSS membership
        # requirements in the BSSMembershipSelectorSet parameter."
        #
        # 11.1.4.6 80211-2016.pdf

        fmt = "%dB" % len(self.data) 
        decoded = [self._decode_byte(b) for b in struct.unpack(fmt, self.data)]

        basic_rate_set = [rate[0] for rate in decoded if rate[1] and not isinstance(rate[0], str)]
        oper_rate_set = [rate[0] for rate in decoded if not rate[1] and not isinstance(rate[0], str)]
        memb_selector_set = [rate[0] for rate in decoded if isinstance(rate[0],str)]

        self.fields = [ IE.Value("BSS Basic Rate Set", basic_rate_set),
                        IE.Value("BSS Operational Rate Set", oper_rate_set),
                        IE.Value("BSS Membership Selector Set", memb_selector_set),
                      ]

    def pretty_print(self):
        string = ""
        fmt = "%dB" % len(self.data) 
        for byte in struct.unpack(fmt, self.data):
            # if byte & 0x80 then it's a required rate
            # following code from iw scan.c print_supprates()
            r = byte & 0x7f
            if r == BSS_MEMBERSHIP_SELECTOR_VHT_PHY and byte & 0x80:
                string += "VHT"
            elif r == BSS_MEMBERSHIP_SELECTOR_HT_PHY and byte & 0x80:
                string += "HT"
            else:
                string += "%d.%d" % (r / 2, 5 * (r & 1))
            string += "%s " % ("*" if byte & 0x80 else "")

        return string


class DSSS_Parameter_Set(IE):
    ID = 3

    def decode(self):
#        self.fields = list(struct.unpack('B', self.data))
        self.fields = IE.Value("channel", struct.unpack('B', self.data)[0])

    def pretty_print(self):
        # single valued field
        return "channel {}".format(self.fields.value)


class TIM(IE):
    # Traffic Indication Map
    ID = 5

    def decode(self):
        (count, period, bitmapc,) = struct.unpack('BBB', self.data[0:3])
        bitmap_hex = hexdump(self.data[3:])

        self.fields = [
            IE.Value("DTIM Count", count),
            IE.Value("DTIM Period", period),
            IE.Value("Bitmap Control", bitmapc),
            IE.Value("Bitmap", bitmap_hex)]

    def pretty_print(self):
        count = self.fields[0].value
        period = self.fields[1].value
        bitmapc = self.fields[2].value
        bitmap = self.fields[3].value

        return ("DTIM Count {0} DTIM Period {1} Bitmap Control 0x{2} "
                "Bitmap {3}".format(count, period, bitmapc, bitmap))


class Country(IE):
    ID = 7

    # iw scan.c print_country()
    IEEE80211_COUNTRY_EXTENSION_ID  = 201 

    def decode(self):
        offset = 0
        # hostile input opportunity: invalid length in country IE
        if len(self.data) < 3:
            log.error("invalid length=%d in country code", len(self.data))
            return

        country_bytes, = struct.unpack("3s", self.data[0:3])

        # dot11CountryString Annex C, page 2787 80211_2016.pdf
        # must be valid ISO-3166-1
        # https://en.wikipedia.org/wiki/ISO_3166-1
        # https://www.iso.org/iso-3166-country-codes.html (US$40? seriously?)
        #
        # beware hostile input; this field is usually treated as a printable
        # string so need to do extra validation
        iso3166_char = lambda byte : chr(byte) if byte >= ord('A') and byte <= ord('Z') else '?'
        country_string = iso3166_char(country_bytes[0]) + iso3166_char(country_bytes[1])

        environment = country_bytes[2]
        if environment == ord(' '):
            environment = "Indoor/Outdoor"
        elif environment == ord('O'):
            environment = "Outdoor only"
        elif environment == ord('I'):
            environment = "Indoor only"
        elif environment == ord('X'):
            environment = "Non-country"
        else:
            environment = "(invalid!)"

        self.fields = [ IE.Value("Country", country_string), 
                        IE.Value("Environment", environment),
                      ]

        if len(self.data) == 3:
            # no country codes so can leave now
            log.debug("No country IE triplets present")
            return

        # now the decode gets weird
        # iw scan.c print_country()
        offset = 3
        subband_triplets = IE.Value("Subbands", [])
        while offset+2 < len(self.data):
            triplet = struct.unpack("BBB", self.data[offset:offset+3])
            if triplet[0] >= self.IEEE80211_COUNTRY_EXTENSION_ID:
                val_triplet = [IE.Value("Extension ID", triplet[0]), 
                               IE.Value("Regulatory Class", triplet[1]),
                               IE.Value("Coverage Class", triplet[2])]
            else:
                val_triplet = [IE.Value("First Channel", triplet[0]), 
                               IE.Value("Number of Channels", triplet[1]),
                               IE.Value("Max TX Power (dBm)", triplet[2])]
            offset += 3

            subband_triplets.value.append(val_triplet)

        self.fields.append(subband_triplets)

class HT_Capabilities(IE):
    # iw scan.c print_ht_capa()
    # iw util. print_ht_capability()
    ID = 45

    # Capability Info bit 1
    channel_width = ("HT20", "HT20/HT40")

    # Capability Info bits 2,3
    power_save = ("Static SM Power Save",
                  "Dynamic SM Power Save",
                  "Reserved",
                  "SM Power Save disabled")

    # Capability Info bits 8,9
    rx_stbc_streams = ("No RX STBC",
                       "RX STBC 1-stream",
                       "RX STBC 2-streams",
                       "RX STBC 3-streams")

    # Capability Info bit 11
    amsdu_length = (3839, 7935)

    # Capability Info
    capa_grammar = (
        IE.Grammar(0, 1, "RX LDPC"),
        IE.Grammar(1, 1, "HT20/HT40"),
        IE.Grammar(2, 3, "SM Power Save"),
        IE.Grammar(4, 1, "RX Greenfield"),
        IE.Grammar(5, 1, "RX HT20 SGI"),
        IE.Grammar(6, 1, "RX HT40 SGI"),
        IE.Grammar(7, 1, "TX STBC"),
        IE.Grammar(8, 3, "RX STBC"),
        IE.Grammar(10, 1, "HT Delayed Block Ack"),
        IE.Grammar(11, 1, "Max AMSDU length"),
        IE.Grammar(12, 1, "DSSS/CCK HT40"),
        # 13 is reserved
        IE.Grammar(14, 1, "40 Mhz Intolerant"),
        IE.Grammar(15, 1, "L-SIG TXOP protection"),
    )

    ampdu_grammar = (
        IE.Grammar(0, 2, "Maximum RX AMPDU length"),
        IE.Grammar(2, 3, "Minimum RX AMPDU time"),
    )

    extended_capa_grammar = (
        IE.Grammar(0, 1, "PCO"),
        IE.Grammar(1, 2, "PCO Transition Time"),
        # bits 3-7 reserved
        IE.Grammar(8, 2, "MCS Feedback"),
        IE.Grammar(10, 1, "HTC-HT Support"),
        IE.Grammar(11, 1, "RD Responder"),
        # bits 12-15 reserved
    )

    tx_beam_form_capabilities_grammar = (
        IE.Grammar(0, 1, "Implicit Tx Beamforming Receive"),
        IE.Grammar(1, 1, "Receive Staggered Sound"),
        IE.Grammar(2, 1, "Transmit Staggered Sound"),
        IE.Grammar(3, 1, "Receive NDP"),
        IE.Grammar(4, 1, "Transmit NDP"),
        IE.Grammar(5, 1, "Implicit Transmit Beamforming"),
        IE.Grammar(6, 2, "Calibration"),
        IE.Grammar(8, 1, "Explicit CSI Transmit Beamform"),
        IE.Grammar(9, 1, "Explicit Noncompressed Steering"),
        IE.Grammar(10, 1, "Explicit Compressed Steering"),
        IE.Grammar(11, 2, "Explicit Transform Beamforming CSI Feedback"),
        IE.Grammar(13, 2, "Explicit Noncompressed Beamforming Feedback"),
        IE.Grammar(15, 2, "Explicit Compressed Beamforming Feedback"),
        IE.Grammar(17, 2, "Minimal Grouping"),
        IE.Grammar(19, 2, "CSI Number of Beamformer Antennae"),
        IE.Grammar(21, 2, "Noncompressed Steering Num of Beamformer Antennae"),
        IE.Grammar(23, 2, "Compressed Steering Number of Beamformer Antennae"),
        IE.Grammar(25, 2, "CSI Max Number of Rows Beamformer"),
        IE.Grammar(27, 2, "Channel Estimation"),
    )

    asel_capabilities = (
        IE.Grammar(0, 1, "Antenna Selection Capable"),
        IE.Grammar(1, 1, "Explicit CSI Feedback Based TX ASEL"),
        IE.Grammar(2, 1, "Antenna Indices Feedback"),
        IE.Grammar(3, 1, "Explicit CSI Feedback"),
        IE.Grammar(4, 1, "Antennae Indices Feedback"),
        IE.Grammar(5, 1, "Receive ASEL"),
        IE.Grammar(6, 1, "Transmit Sounding PPDUs"),
        IE.Grammar(7, 1, "Reserved"),
    )

    # iw util.c compute_ampdu_length()
    #
    # "There are only 4 possible values, we just use a case instead of
    # computing it, but technically this can also be computed through the
    # formula:
    #   Max AMPDU length = (2 ^ (13 + exponent)) - 1 bytes"
    #
    ampdu_length = (8191,   # /* (2 ^(13 + 0)) -1 */
                    16383,  # /* (2 ^(13 + 1)) -1 */
                    32767,  # /* (2 ^(13 + 2)) -1 */
                    65535,  # /* (2 ^(13 + 3)) -1 */
                    )

    # iw util.c print_ampdu_space()
    ampdu_space = (
        "No restriction",
        "1/4 usec",
        "1/2 usec",
        "1 usec",
        "2 usec",
        "4 usec",
        "8 usec",
        "16 usec",
    )

    def decode(self):
        # 9.4.2.56.1. HT Capabilities Information Structure
        # (Section numbers are from 80211-2016.pdf)
        offset = 0

        # 9.4.2.56.2 HT Capability Info Field
        # 2 octets
        num, = struct.unpack_from("<H", self.data, offset)
        self.fields.append(IE.Value('HT Capability Info',
                           self.decode_integer(self.capa_grammar, num)))
        offset += 2

        # 9.4.2.56.3 A-MPDU Parameters
        # 1 octet
        num, = struct.unpack_from("B", self.data, offset)
        self.fields.append(IE.Value('AMPDU Parameters',
                           self.decode_integer(self.ampdu_grammar, num)))
        offset += 1

        # 9.4.2.56.4 Supported MCS Set
        # 16 octets
        # iw util.c print_ht_mcs
        mcs = struct.unpack_from("16B", self.data, offset)
        self.fields.append(IE.Value("Supported MCS Set", mcs))
        # TODO MCS crazy complicated so finish later
        # max_rx_supp_data_rate = (mcs[10] | ((mcs[11] & 0x3) << 8));
        # tx_mcs_set_defined = not(not(mcs[12] & (1 << 0)));
        # tx_mcs_set_equal = not(mcs[12] & (1 << 1));
        # tx_max_num_spatial_streams = ((mcs[12] >> 2) & 3) + 1;
        # tx_unequal_modulation = not(not(mcs[12] & (1 << 4)));
        offset += 16

        # HT Extended Capabilities
        # 2 octets
        num, = struct.unpack_from("<H", self.data, offset)
        self.fields.append(IE.Value("HT Extended Capabilities",
                           self.decode_integer(self.extended_capa_grammar,
                                               num)))
        offset += 2

        # TX beamforming capabilities
        # 4 octets
        num, = struct.unpack_from("<L", self.data, offset)
        val = self.decode_integer(self.tx_beam_form_capabilities_grammar, num)
        self.fields.append(IE.Value("TX Beamforming Capabilities", val))
        offset += 4

        # ASEL capabilities
        # 1 octet
        num, = struct.unpack_from("B", self.data, offset)
        self.fields.append(IE.Value("ASEL Capability",
                           self.decode_integer(self.asel_capabilities, num)))
        offset += 1

    def channel_width_str(self, value):
        return self.channel_width[value]

    def sm_power_save_str(self, value):
        return self.power_save[value]

    def rx_stbc_str(self, value):
        return self.rx_stbc_streams[value]

    def max_amsdu_len(self, value):
        return self.amsdu_length[value]


class RSN(IE):
    # Robust Security Network
    ID = 48

    # table 9-131  "Cipher Suite Selectors" 80211_2016.pdf 
    cipher_suite_selectors = (
        "Use group cipher suite",
        "WEP-40",
        "TKIP", 
        "Reserved",
        "CCMP-128",
        "WEP-104",
        "Group addressed traffic not allowed",
        "GCMP-128",
        "GCMP-256",
        "CCMP-256",
        "BIP-GMAC-128",
        "BIP-GMAC-256")

    auth_type_name_list = (
        "Reserved",
        "IEEE 8021.X",
        "PSK",
        "FT/IEEE 802.1X",
        "FT/PSK",
        "IEEE 802.1X/SHA-256",
        "PSK/SHA-256",
        "TDLS/TPK",
        "SAE",
        "SAE",
        "IEEE 802.1X/SUITE-B",
        "IEEE 802.1X/SUITE-B-192",
        "FT/IEEE 802.1X/SHA-384",
        "FILS/SHA-256",
        "FILS/SHA-384",
        "FT/FILS/SHA-256",
        "FT/FILS/SHA-384",
        "OWE")

    rsn_capabilities = (
        IE.Grammar(0, 1, "Preauth"),
        IE.Grammar(1, 1, "NoPairwise"),
        IE.Grammar(2, 2, "PTKSA-RC"),
        IE.Grammar(4, 2, "GTKSA-RC"),
        IE.Grammar(6, 1, "MFP-required"),
        IE.Grammar(7, 1, "MFP-capable"),
        IE.Grammar(8, 1, "Joint Multi-Band RSNA"),
        IE.Grammar(9, 1, "Peerkey-enabled"),
        IE.Grammar(10, 1, "SPP-AMSDU-capable"),
        IE.Grammar(11, 1, "SPP-AMSDU-required"),
        IE.Grammar(12, 1, "PBAC"),
        IE.Grammar(13, 2, "Ext Key ID"),
        IE.Grammar(14, 2, "Reserved"),
    )

    # 9.4.2.25 RSNE  80211_2016.pdf
    def decode(self):
        # (this one is super messy)
        # iw scan.c print_rsn() -> print_rsn_ie() -> _print_rsn_ie()
        # So _print_rsn_ie() is the best place to start.
        offset = 0
        version = self.data[0] + (self.data[1] << 8)

        self.fields = [IE.Value("Version", version), ]
        offset += 2

        # pretty much optional fields from here on down
        # so have to carefully check len(self.data) after every field

        # Group Data Cipher Suite
        group = struct.unpack("4B", self.data[offset:offset+4])
        self.fields.append( IE.Value("Group Cipher Suite", self._decode_cipher(group)))
        offset += 4
        if offset >= len(self.data): return

        # Pairwise Cipher Suite Count
        pairwise_count, = struct.unpack("<H", self.data[offset:offset+2])
        self.fields.append(IE.Value("Pairwise Count", pairwise_count))
        offset += 2
        if offset >= len(self.data): return

        # Pairwise Cipher Suite List
        length = 4 * pairwise_count
        fmt = "%dB" % length
        pairwise_bytes = struct.unpack(fmt, self.data[offset:offset+length])
        pairwise_list = [self._decode_cipher(pairwise_bytes[n:n+4]) for n in range(0, length, 4)]
        self.fields.append(IE.Value("Pairwise Bytes", pairwise_list))
        offset += length
        if offset >= len(self.data): return

        # AKM Suite Count
        akm_suite_count, = struct.unpack("<H", self.data[offset:offset+2])
        self.fields.append(IE.Value("AKM Suite Count", akm_suite_count))
        offset += 2
        if offset >= len(self.data): return

        # AKM Suite List
        length = 4 * akm_suite_count
        fmt = "%dB" % length
        akm_suite_bytes = struct.unpack(fmt, self.data[offset:offset+length])
        akm_suite_list = [self._decode_auth(akm_suite_bytes[n:n+4]) for n in range(0,length,4)]
        self.fields.append(IE.Value("AKM Suite", akm_suite_list))
        offset += length
        if offset >= len(self.data): return

        # RSN Capabilities
        rsn_capa, = struct.unpack("<H", self.data[offset:offset+2])
        self.fields.append(IE.Value("RSN Capabilities", 
                           self.decode_integer(self.rsn_capabilities, rsn_capa)))
        offset += 2
        if offset >= len(self.data): return

        # PMKID Count
        pmkid_count, = struct.unpack("<H", self.data[offset:offset+2])
        self.fields.append(IE.Value("PMKID Count", pmkid_count))
        offset += 2
        if offset >= len(self.data): return

        # PMKID List

        # Group Management Cipher Suite


    def _decode_cipher(self, bytelist):
        # iw scan.c print_cipher()
        oui = self.format_oui(bytelist[0:3])
        suite_type = bytelist[3]

        meaning = "Vendor-specific"
        if oui == OUI_IEEE:
            try:
                meaning = self.cipher_suite_selectors[suite_type]
            except IndexError:
                meaning = "Reserved"
        elif oui == OUI_MS:
            # TODO
            pass

        return [IE.Value("OUI", oui),
                IE.Value("Suite Type", suite_type), 
                IE.Value("Suite Name", meaning),
               ]

    def _decode_auth(self, bytelist):
        # iw scan.c print_auth()
        oui = self.format_oui(bytelist[0:3])
        suite_type = bytelist[3]

        meaning = "Vendor-specific"
        if oui == OUI_IEEE:
            try:
                meaning = self.auth_type_name_list[suite_type]
            except IndexError:
                meaning = "Reserved"
        elif oui == OUI_MS:
            # TODO
            pass

        return [IE.Value("OUI", oui),
                IE.Value("Suite Type", suite_type), 
                IE.Value("Suite Name", meaning),
               ]

class Extended_Rates(Supported_Rates):
    ID = 50
    # same format as supported rates so this one is easy!


class HT_Operation(IE):
    # iw scan.c print_ht_op()
    ID = 61

    # Note: using the same strings as being used in 'iw' to be as compatible as
    # possible
    info_1_grammar = (
        IE.Grammar(0, 2, "secondary channel offset"),
        IE.Grammar(2, 1, "STA channel width"),
        IE.Grammar(3, 1, "RIFS"),
        IE.Grammar(8, 2, "HT Protection"),
        IE.Grammar(10, 1, "non-GF present"),
        IE.Grammar(12, 1, "OBSS non-GF present"),
        IE.Grammar(13, 2047, "Channel Center Frequency Segment 2"),   # 11 bits
        IE.Grammar(30, 1, "dual beacon"),
        IE.Grammar(31, 1, "dual CTS protection"),
    )

    info_2_grammar = (
        IE.Grammar(0, 1, "STBC Beacon"),
        IE.Grammar(1, 1, "L-SIG TXOP Prot"),
        IE.Grammar(2, 1, "PCO active"),
        IE.Grammar(3, 1, "PCO phase"),
    )

    # iw scan.c ht_secondary_offset[]
    ht_secondary_offset = (
        "no secondary",
        "above",
        "[reserved!]",
        "below",
    )

    # iw scan.c sta_chan_width[]
    sta_chan_width = (
        "20 MHz",
        "any",
    )

    def decode(self):
        # 9.4.2.57 HT Operation element
        offset = 0

        # primary channel
        # 1 octet
        num, = struct.unpack_from("B", self.data, offset)
        self.fields.append(IE.Value("Primary Channel", num))
        offset += 1

        # HT Operation Information
        # 5 octets
        #  - fields are spread unevenly across bytes
        #  - want to be easily compatible with 32-bit systems
        #  - natural break occurs at B31 "Dual CTS Protection"
        # therefore decode into a uint32_t and uint16_t
        info = struct.unpack_from("<LH", self.data, offset)
        info_fields = self.decode_integer(self.info_1_grammar, info[0])
        info_fields.extend(self.decode_integer(self.info_2_grammar, info[1]))
        self.fields.append(IE.Value("Information", info_fields))
        offset += 5

        # Basic MT-MCS Set
        # 16 octets
        mcs = struct.unpack_from("16B", self.data, offset)
        self.fields.append(IE.Value("Basic HT-MCS Set", mcs))
        offset += 16

    def secondary_channel_offset(self, value):
        return self.ht_secondary_offset[value]

    def sta_channel_width(self, value):
        return self.sta_chan_width[value]


class Extended_Capabilities(IE):
    ID = 127

    # iw scan.c print_capabilities()
    bits = (
        (
            "HT Information Exchange Supported",  # 0
            "reserved (On-demand Beacon)",
            "Extended Channel Switching",
            "reserved (Wave Indication)",
            "PSMP Capability",
            "reserved (Service Interval Granularity)",
            "S-PSMP Capability",
            "Event",
        ),
        (
            "Diagnostics",  # 8
            "Multicast Diagnostics",
            "Location Tracking",
            "FMS",
            "Proxy ARP Service",
            "Collocated Interference Reporting",
            "Civic Location",
            "Geospatial Location",
        ),
        (
            "TFS",  # 16
            "WNM-Sleep Mode",
            "TIM Broadcast",
            "BSS Transition",
            "QoS Traffic Capability",
            "AC Station Count",
            "Multiple BSSID",
            "Timing Measurement",
        ),
        (
            "Channel Usage",  # 24
            "SSID List",
            "DMS",
            "UTC TSF Offset",
            "TDLS Peer U-APSD Buffer STA Support",
            "TDLS Peer PSM Support",
            "TDLS channel switching",
            "Interworking",
        ),
        (
            "QoS Map",  # 32
            "EBR",
            "SSPN Interface",
            "Reserved",
            "MSGCF Capability",
            "TDLS Support",
            "TDLS Prohibited",
            "TDLS Channel Switching Prohibited",
        ),
        (
            "Reject Unadmitted Frame",  # 40
            "SI Duration Bit0",
            "SI Duration Bit1",
            "SI Duration Bit2",
            "Identifier Location",
            "U-APSD Coexistence",
            "WNM-Notification",
            "Reserved",
        ),
        (
            "UTF-8 SSID",  # 48
            "QMFActivated",
            "QMFReconfigurationActivated",
            "Robust AV Streaming",
            "Advanced GCR",
            "Mesh GCR",
            "SCS",
            "QLoad Report",
        ),
        (
            "Alternate EDCA",  # 56
            "Unprotected TXOP Negotiation",
            "Protected TXOP egotiation",
            "Reserved",
            "Protected QLoad Report",
            "TDLS Wider Bandwidth",
            "Operating Mode Notification",
            "MAX AMSDU bit0",
        ),
        (
            "MAX AMSDU bit1",
            "Channel Schedule Management",
            "Geodatabase Inband Enabling Signal",
            "Network Channel Control",
            "White Space Map",
            "Channel Availability Query",
            "FTM Responder",
            "FTM Initiator",
        ),
        (
            "Reserved",
            "Extended Spectrum Management Capable",
            "Reserved",
        )
    )

    def decode(self):
        # variable length field

        nums = struct.unpack("%dB" % len(self.data), self.data)

        # TODO use is_vht to decode Max AMSDU (somehow...)
        self.fields = [IE.Value(self.bits[byte][bit], nums[byte] & (1 << bit))
                       for byte in range(min(8, len(nums)))
                       for bit in range(0, 8)]


class VHT_Capabilities(IE):
    # iw scan.c print_vht_capa()
    # iw util.c print_vht_info()
    # iw scan.c print_vht_oper()
    ID = 191

    # -1 for invalid/reserved
    max_mpdu = (3895, 7991, 11454, -1)

    channel_width = ("neither 160 nor 80+80",
                     "160 MHz",
                     "160 Mhz, 80+80 Mhz",
                     "(reserved)")

    capa_grammar = (
        # offset, mask, type, name
        IE.Grammar(0, 2, "Max MPDU length"),
        IE.Grammar(2, 2, "Supported Channel Width"),
        IE.Grammar(4, 1, "RX LDPC"),
        IE.Grammar(5, 1, "short GI (80 MHz)"),
        IE.Grammar(6, 1, "short GI (160/80+80 MHz)"),
        IE.Grammar(7, 1, "TX STBC"),
        # TODO RX STBC bits 8,9,10
        IE.Grammar(11, 1, "SU Beamformer"),
        IE.Grammar(12, 1, "SU Beamformee"),
        # TODO compressed steering bits 13,14,15
        # TODO num of sounding dimensions bits 16,17,18
        IE.Grammar(19, 1, "MU Beamformer"),
        IE.Grammar(20, 1, "MU Beamformee"),
        IE.Grammar(21, 1, "VHT TXOP PS"),
        IE.Grammar(22, 1, "+HTC-VHT"),
        # TODO max A-MPDU bits 23,24,25
        # TODO VHT link adaptation bits 26,27
        IE.Grammar(28, 1, "RX antenna pattern consistency"),
        IE.Grammar(29, 1, "TX antenna pattern consistency")
        # TODO NSS BW Support bits 30,31
    )

    def decode(self):
        offset = 0
        # 4 octets
        num, = struct.unpack_from("<L", self.data, offset)
        self.fields.append(IE.Value("VHT Capability Info",
                           self.decode_integer(self.capa_grammar, num)))
        offset += 4

        # 8 octets
        num = struct.unpack_from("<4H", self.data, offset)
        # TODO decode MCS
        self.fields.append(IE.Value("MCS", num))

    def max_mpdu_len(self, value):
        return self.max_mpdu[value]

    def supported_chan_width_str(self, value):
        return self.channel_width[value]


class VHT_Operation(IE):
    ID = 192

    # note this differs from iw scan.c print_vht_oper()
    # I'm going from 80211_2016.pdf  I think some fields were deprecated.
    channel_width = (
        "20 or 40 Mhz",
        "80, 160, 80+80 MHz",
        "160 MHz (deprecated)",
        "80+80 Mhz (deprecated)"
    )

    def decode(self):
        offset = 0
        oper_info = struct.unpack("3B", self.data[0:3])
        offset += 3
        self.fields.append(IE.Value("Operation Info",
                            [IE.Value("Channel Width", oper_info[0]),
                             IE.Value("Channel Center Frequency Segment 0", oper_info[1]),
                             IE.Value("Channel Center Frequency Segment 1", oper_info[2]),
                            ]))

        # bitmap of 16-bits
        vht_mcs_nss_set, = struct.unpack("<H", self.data[offset:offset+2])
        self.fields.append(IE.Value("VHT-MSS and NSS Set", vht_mcs_nss_set))

        offset += 2

    def channel_width_str(self, value):
        try:
            return self.channel_width[value]
        except IndexError:
            return "Reserved"
        

class Vendor_Specific(IE):
    ID = 221

    def decode(self):
        # raw bytes
        s = self.format_oui(self.data[0:3])
        try:
            vendor = oui.vendor_lookup(s.upper())
        except KeyError:
            vendor = "Unknown"
        self.fields.append(IE.Value("Vendor Specific", 
                                [
                                   IE.Value("OUI", s),
                                   IE.Value("vendor name", vendor),
                                   IE.Value("raw", self.data)
                                ]
                          ))
#        self.fields.append(IE.Value("Vendor Specific", hexdump(self.data)))


class NL80211_Scan(nl80211cmd):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        print("Hello from NL80211_Scan!")

    class bss(nl80211cmd.bss):
        nla_map = (('__NL80211_BSS_INVALID', 'hex'),
                   ('NL80211_BSS_BSSID', 'hex'),
                   ('NL80211_BSS_FREQUENCY', 'uint32'),
                   ('NL80211_BSS_TSF', 'TSF'),
                   ('NL80211_BSS_BEACON_INTERVAL', 'uint16'),
                   ('NL80211_BSS_CAPABILITY', 'capability'),
                   ('NL80211_BSS_INFORMATION_ELEMENTS', 'my_elementsBinary'),
                   ('NL80211_BSS_SIGNAL_MBM', 'SignalMBM'),
                   ('NL80211_BSS_SIGNAL_UNSPEC', 'uint8'),
                   ('NL80211_BSS_STATUS', 'uint32'),
                   ('NL80211_BSS_SEEN_MS_AGO', 'uint32'),
                   ('NL80211_BSS_BEACON_IES', 'my_elementsBinary'),
                   ('NL80211_BSS_CHAN_WIDTH', 'uint32'),
                   ('NL80211_BSS_BEACON_TSF', 'uint64'),
                   ('NL80211_BSS_PRESP_DATA', 'hex'),
                   ('NL80211_BSS_MAX', 'hex')
                   )

        def __init__(self, *args, **kwargs):
            print("Hello from bss!")
            for m in self.nla_map:
                print(m)
#                if m[1] == 'elementsBinary':
#                    m[1] = 'different_elementsBinary'
#            breakpoint()
            super().__init__(*args, **kwargs)

#        def decode(self, *args, **kwargs):
#            print(self)
#            assert 0

        class my_elementsBinary(nl80211cmd.bss.elementsBinary):
#            def __init__(self, *args, **kwargs):
#                breakpoint()
#                super().__init__(*args, **kwargs)
#                print("Hello from elementsBinary!")

            def decode(self):
#                breakpoint()
                nla_base.decode(self)

                self.value = {}

                init = offset = self.offset + 4

                ie_class_map = {
                    NL80211_BSS_ELEMENTS_SSID: SSID,
                    NL80211_BSS_ELEMENTS_SUPPORTED_RATES: Supported_Rates,
                    NL80211_BSS_ELEMENTS_CHANNEL: DSSS_Parameter_Set,
                    NL80211_BSS_ELEMENTS_TIM: TIM,
                    NL80211_BSS_ELEMENTS_COUNTRY: Country,
                    NL80211_BSS_ELEMENTS_HT_CAPABILITIES: HT_Capabilities,
                    NL80211_BSS_ELEMENTS_RSN: RSN,
                    NL80211_BSS_ELEMENTS_EXTENDED_RATE: Extended_Rates,
                    NL80211_BSS_ELEMENTS_HT_OPERATION: HT_Operation,
                    NL80211_BSS_ELEMENTS_EXT_CAPABILITIES:
                        Extended_Capabilities,
                    NL80211_BSS_ELEMENTS_VHT_CAPABILITIES: VHT_Capabilities,
                    NL80211_BSS_ELEMENTS_VHT_OPERATION: VHT_Operation,
                    NL80211_BSS_ELEMENTS_VENDOR: Vendor_Specific,
                }

                while (offset - init) < (self.length - 4):
                    (msg_type, length) = struct.unpack_from('BB',
                                                            self.data,
                                                            offset)

                    # TODO so I can keep keep an eye on what needs to be
                    # decoded
                    try:
                        msg_name = NL80211_BSS_ELEMENTS_VALUES[msg_type]
                    except KeyError:
                        log.warning("unhandled IE element type=%d", msg_type)
                        if "TODO" in self.value:
                            self.value["TODO"].append(msg_type)
                        else:
                            self.value["TODO"] = [msg_type, ]
                        offset += length + 2
                        continue

                    cls = ie_class_map[msg_type]
                    offset += 2
                    val = cls(self.data[offset:offset + length])
                    val.decode()
                    if msg_type == NL80211_BSS_ELEMENTS_VENDOR and msg_name in self.value:
                        # append to existing vendorid
                        self.value[msg_name].fields.extend(val.fields)
                    else:
                        self.value[msg_name] = val
                    offset += length

        class TSF(nla_base):
            """Timing Synchronization Function"""
            def decode(self):
                nla_base.decode(self)

                offset = self.offset + 4
                self.value = {}
                tsf, = struct.unpack_from('Q', self.data, offset)
                self.value["VALUE"] = tsf
                # TSF is in microseconds
                # TODO verify this won't overflow internally
                self.value["TIME"] = datetime.timedelta(microseconds=tsf)


        class SignalMBM(nla_base):
            def decode(self):
                nla_base.decode(self)
                offset = self.offset + 4
                self.value = {}
                ss, = struct.unpack_from('i', self.data, offset)
                self.value["VALUE"] = ss
                self.value["SIGNAL_STRENGTH"] = {"VALUE": ss / 100.0,
                                                 "UNITS": "dBm"}

        class capability(nla_base):
            # iw scan.c
            WLAN_CAPABILITY_ESS = (1 << 0)
            WLAN_CAPABILITY_IBSS = (1 << 1)
            WLAN_CAPABILITY_CF_POLLABLE = (1 << 2)
            WLAN_CAPABILITY_CF_POLL_REQUEST = (1 << 3)
            WLAN_CAPABILITY_PRIVACY = (1 << 4)
            WLAN_CAPABILITY_SHORT_PREAMBLE = (1 << 5)
            WLAN_CAPABILITY_PBCC = (1 << 6)
            WLAN_CAPABILITY_CHANNEL_AGILITY = (1 << 7)
            WLAN_CAPABILITY_SPECTRUM_MGMT = (1 << 8)
            WLAN_CAPABILITY_QOS = (1 << 9)
            WLAN_CAPABILITY_SHORT_SLOT_TIME = (1 << 10)
            WLAN_CAPABILITY_APSD = (1 << 11)
            WLAN_CAPABILITY_RADIO_MEASURE = (1 << 12)
            WLAN_CAPABILITY_DSSS_OFDM = (1 << 13)
            WLAN_CAPABILITY_DEL_BACK = (1 << 14)
            WLAN_CAPABILITY_IMM_BACK = (1 << 15)

#            def decode_nlas(self):
#                return

            def decode(self):
                nla_base.decode(self)

                offset = self.offset + 4
                self.value = {}
                capa, = struct.unpack_from('H', self.data, offset)
                self.value["VALUE"] = capa

                s = []
                if capa & self.WLAN_CAPABILITY_ESS:
                    s.append("ESS")
                if capa & self.WLAN_CAPABILITY_IBSS:
                    s.append("IBSS")
                if capa & self.WLAN_CAPABILITY_CF_POLLABLE:
                    s.append("CfPollable")
                if capa & self.WLAN_CAPABILITY_CF_POLL_REQUEST:
                    s.append("CfPollReq")
                if capa & self.WLAN_CAPABILITY_PRIVACY:
                    s.append("Privacy")
                if capa & self.WLAN_CAPABILITY_SHORT_PREAMBLE:
                    s.append("ShortPreamble")
                if capa & self.WLAN_CAPABILITY_PBCC:
                    s.append("PBCC")
                if capa & self.WLAN_CAPABILITY_CHANNEL_AGILITY:
                    s.append("ChannelAgility")
                if capa & self.WLAN_CAPABILITY_SPECTRUM_MGMT:
                    s.append("SpectrumMgmt")
                if capa & self.WLAN_CAPABILITY_QOS:
                    s.append("QoS")
                if capa & self.WLAN_CAPABILITY_SHORT_SLOT_TIME:
                    s.append("ShortSlotTime")
                if capa & self.WLAN_CAPABILITY_APSD:
                    s.append("APSD")
                if capa & self.WLAN_CAPABILITY_RADIO_MEASURE:
                    s.append("RadioMeasure")
                if capa & self.WLAN_CAPABILITY_DSSS_OFDM:
                    s.append("DSSS-OFDM")
                if capa & self.WLAN_CAPABILITY_DEL_BACK:
                    s.append("DelayedBACK")
                if capa & self.WLAN_CAPABILITY_IMM_BACK:
                    s.append("ImmediateBACK")

                self.value['CAPABILITIES'] = " ".join(s)


