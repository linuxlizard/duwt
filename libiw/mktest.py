#!/usr/bin/env python3

# I've visually inspected the results of the initial decode so these tests
# should hopefully keep the code working when I change the decode yet again.
#
# At this point, I'm just amusing myself. Want a way to validate the decode but
# don't want to have to manually write 100s of tests for each individual field. So,
# start with a buffer, decode it, build+run the decode, then start changing the
# test until the run no longer fails. Yay! I'm doing "machine learning" (ha ha).
# davep 20191225

import os
import re
import itertools
import subprocess

# e.g., XASSERT fail: test_ie_he.c:44 "mac->htc_he_support == 0" value=0x1
# global so only have to compile once
_file_line = r"([a-zA-Z09_\.]+:\d+)"
_qstring = r"(\"[^\"]+\")"
_hex_num = r"(0x[a-fA-F\d]+)"
_value = r"value=" + _hex_num

_assert_str = _file_line + " " + _qstring + " " + _value
_assert_re = re.compile(_assert_str)

he_capa_mac = [
    "htc_he_support",
    "twt_requester_support",
    "twt_responder_support",
    "fragmentation_support",
    "max_number_fragmented_msdus",
    "min_fragment_size",
    "trigger_frame_mac_padding_dur",
    "multi_tid_aggregation_support",
    "he_link_adaptation_support",
    "all_ack_support",
    "trs_support",
    "bsr_support",
    "broadcast_twt_support",
    "_32_bit_ba_bitmap_support",
    "mu_cascading_support",
    "ack_enabled_aggregation_support",
    "reserved_b24",
    "om_control_support",
    "ofdma_ra_support",
    "max_a_mpdu_length_exponent_ext",
    "a_msdu_fragmentation_support",
    "flexible_twt_schedule_support",
    "rx_control_frame_to_multibss",
    "bsrp_bqrp_a_mpdu_aggregation",
    "qtp_support",
    "bqr_support",
    "srp_responder",
    "ndp_feedback_report_support",
    "ops_support",
    "a_msdu_in_a_mpdu_support",
    "multi_tid_aggregation_tx_support",
    "subchannel_selective_trans_support",
    "ul_2_996_tone_ru_support",
    "om_control_ul_mu_data_disable_rx_support",
]

he_capa_phy = [
    "reserved_b0",
    "ch40mhz_channel_2_4ghz",
    "ch40_and_80_mhz_5ghz",
    "ch160_mhz_5ghz",
    "ch160_80_plus_80_mhz_5ghz",
    "ch242_tone_rus_in_2_4ghz",
    "ch242_tone_rus_in_5ghz",
    "reserved_b7",
    "punctured_preamble_rx",
    "device_class",
    "ldpc_coding_in_payload",
    "he_su_ppdu_1x_he_ltf_08us",
    "midamble_rx_max_nsts",
    "ndp_with_4x_he_ltf_32us",
    "stbc_tx_lt_80mhz",
    "stbc_rx_lt_80mhz",
    "doppler_tx",
    "doppler_rx",
    "full_bw_ul_mu_mimo",
    "partial_bw_ul_mu_mimo",
    "dcm_max_constellation_tx",
    "dcm_max_nss_tx",
    "dcm_max_constellation_rx",
    "dcm_max_nss_rx",
    "rx_he_muppdu_from_non_ap",
    "su_beamformer",
    "su_beamformee",
    "mu_beamformer",
    "beamformer_sts_lte_80mhz",
    "beamformer_sts_gt_80mhz",
    "number_of_sounding_dims_lte_80",
    "number_of_sounding_dims_gt_80",
    "ng_eq_16_su_fb",
    "ng_eq_16_mu_fb",
    "codebook_size_eq_4_2_fb",
    "codebook_size_eq_7_5_fb",
    "triggered_su_beamforming_fb",
    "triggered_mu_beamforming_fb",
    "triggered_cqi_fb",
    "partial_bw_extended_range",
    "partial_bw_dl_mu_mimo",
    "ppe_threshold_present",
    "srp_based_sr_support",
    "power_boost_factor_ar_support",
    "he_su_ppdu_etc_gi",
    "max_nc",
    "stbc_tx_gt_80_mhz",
    "stbc_rx_gt_80_mhz",
    "he_er_su_ppdu_4xxx_gi",
    "_20mhz_in_40mhz_24ghz_band",
    "_20mhz_in_160_80p80_ppdu",
    "_80mgz_in_160_80p80_ppdu",
    "he_er_su_ppdu_1xxx_gi",
    "midamble_rx_2x_xxx_ltf",
    "dcm_max_bw",
    "longer_than_16_he_sigb_ofdm_symbol_support",
    "non_triggered_cqi_feedback",
    "tx_1024_qam_242_tone_ru_support",
    "rx_1024_qam_242_tone_ru_support",
    "rx_full_bw_su_using_he_muppdu_w_compressed_sigb",
    "rx_full_bw_su_using_he_muppdu_w_non_compressed_sigb",
    "nominal_packet_padding",
]


def find_test_function(src, fn_name):
    # Find the line number of the C function with the given name. Note this is
    # doing a crappy startswith search assuming the function is 'static void'
    # which is what I"m using in my test code so NBD
    #
    # another linear search (barf)
    s = "static void " + fn_name + "("
    found = itertools.takewhile(
        lambda m: not m[0].startswith(s), zip(src, itertools.count())
    )

    _, line = list(found)[-1]

    try:
        return src[line + 1], line + 1
    except IndexError as err:
        raise ValueError(fn_name + " not found") from err


def make_test_function_name(testname):
    return "test_" + testname


def test_exists(src, testname):
    try:
        find_test_function(src, make_test_function_name(testname))
        return True
    except ValueError:
        return False


def add_test_function(src, testname, new_test):
    main_start = src.index("int main(void)\n")
    main_return = src.index("\treturn EXIT_SUCCESS;\n", main_start)
    print(f"found main at {main_start}:{main_return}")

    return (
        src[:main_start]
        + new_test
        + src[main_start:main_return]
        + ["\ttest_{}(mac);\n".format(testname),]
        + src[main_return:]
    )


def save_src(src, filename):
    with open(filename, "w") as outfile:
        outfile.write("".join([s for s in src]))


def parse_assert_fail(err):
    # b'XASSET fail: test_ie_he.c:44 "mac->htc_he_support == 0" value=0x1\n'

    if not err.startswith("XASSERT fail:"):
        raise ValueError("Not a proper assert fail string.")

    robj = _assert_re.search(err)
    assert robj

    filename, line = robj.group(1).split(":")

    return {
        "filename": filename,
        # -1 because assert line numbers (__LINE__) are 1-based but Python
        # arrays are 0-based
        "linenumber": int(line) - 1,
        # note we slice off the enclosing quotes
        "expression": robj.group(2)[1:-1],
        # always assume it's a hex
        "value": int(robj.group(3), 16),
    }


def new_assert(expression, value):
    # replace e.g., "mac->htc_he_support == 0" with new value
    # "mac->htc_he_support == 1"

    # let's assume all expressions are string == value
    value_pos = expression.index("=")

    new_str = "\tXASSERT({0} == {1}, {0});\n".format(expression[: value_pos - 1], value)

    return new_str


def build(exe):
    cmd = ["/usr/bin/make", exe]
    subprocess.run(cmd, check=True, shell=False, stdout=subprocess.DEVNULL)


def run(exe):
    try:
        job = subprocess.run(
            "./" + exe, check=True, shell=False, capture_output=True, text=True
        )
    except subprocess.CalledProcessError as err:
        print("err=", err)
        print("return=", err.returncode)
        print("stderr=", err.stderr)
        stderr = err.stderr
        returncode = err.returncode
    else:
        stderr = None
        returncode = job.returncode

    return returncode, stderr


# def test_mac_buf1(test_src):


def build_run_fix_loop(filename, exe_name, test_src):
    counter = 0
    while counter < 20:
        counter += 1
        build(exe_name)

        returncode, stderr = run(exe_name)
        print(f"{counter} job return={returncode}")
        if returncode == 0:
            print(f"{filename} runs successfully")
            return

        failure = parse_assert_fail(stderr)
        print(f"{counter} failure={failure}")
        linenum = failure["linenumber"]
        print("line=", test_src[linenum])
        print(test_src[0:10])
        print(test_src[linenum - 2 : linenum + 2])

        test_src[failure["linenumber"]] = new_assert(
            failure["expression"], failure["value"]
        )

        print("line=", test_src[linenum])
        print(test_src[linenum - 2 : linenum + 2])
        save_src(test_src, filename)

    assert 0, counter


def main():
    filename = "test_ie_he.c"
    exe_name, _ = os.path.splitext(filename)

    with open(filename, "r") as infile:
        test_src = infile.readlines()
    print(len(test_src))

    testname = "mac_buf1"

    if test_exists(test_src, testname):
        print(f"the {testname} test already exists")
    else:
        new_test = []
        new_test.append(
            "static void test_{}(const struct IE_HE_MAC* mac)\n".format(testname)
        )
        new_test.append("{\n")
        for field in he_capa_mac:
            new_test.append("\tXASSERT(mac->{0} == 0, mac->{0});\n".format(field))
        new_test.append("}\n\n")
        test_src = add_test_function(test_src, testname, new_test)
        save_src(test_src, filename)

    build_run_fix_loop(filename, exe_name, test_src)


if __name__ == "__main__":
    main()
