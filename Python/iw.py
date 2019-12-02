#from . import _iw
#from ._iw import *

import _iw
from _iw import *

import datetime

# iw scan.c print_capa_non_dmg()
def decode_capabilities(capa):
    s = []
    if capa & _iw.WLAN_CAPABILITY_ESS:
        s.append("ESS")
    if capa & _iw.WLAN_CAPABILITY_IBSS:
        s.append("IBSS")
    if capa & _iw.WLAN_CAPABILITY_CF_POLLABLE:
        s.append("CfPollable")
    if capa & _iw.WLAN_CAPABILITY_CF_POLL_REQUEST:
        s.append("CfPollReq")
    if capa & _iw.WLAN_CAPABILITY_PRIVACY:
        s.append("Privacy")
    if capa & _iw.WLAN_CAPABILITY_SHORT_PREAMBLE:
        s.append("ShortPreamble")
    if capa & _iw.WLAN_CAPABILITY_PBCC:
        s.append("PBCC")
    if capa & _iw.WLAN_CAPABILITY_CHANNEL_AGILITY:
        s.append("ChannelAgility")
    if capa & _iw.WLAN_CAPABILITY_SPECTRUM_MGMT:
        s.append("SpectrumMgmt")
    if capa & _iw.WLAN_CAPABILITY_QOS:
        s.append("QoS")
    if capa & _iw.WLAN_CAPABILITY_SHORT_SLOT_TIME:
        s.append("ShortSlotTime")
    if capa & _iw.WLAN_CAPABILITY_APSD:
        s.append("APSD")
    if capa & _iw.WLAN_CAPABILITY_RADIO_MEASURE:
        s.append("RadioMeasure")
    if capa & _iw.WLAN_CAPABILITY_DSSS_OFDM:
        s.append("DSSS-OFDM")
    if capa & _iw.WLAN_CAPABILITY_DEL_BACK:
        s.append("DelayedBACK")
    if capa & _iw.WLAN_CAPABILITY_IMM_BACK:
        s.append("ImmediateBACK")
    return s

def decode_tsf(tsf):
# iw scan.c print_bss_handler()
#    printf("\tTSF: %llu usec (%llud, %.2lld:%.2llu:%.2llu)\n",
#            tsf, tsf/1000/1000/60/60/24, (tsf/1000/1000/60/60) % 24,
#            (tsf/1000/1000/60) % 60, (tsf/1000/1000) % 60);
    # TSF is in microseconds
    return datetime.timedelta(microseconds=tsf)
