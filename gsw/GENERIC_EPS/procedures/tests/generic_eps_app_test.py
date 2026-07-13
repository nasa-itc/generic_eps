from openc3.script import cmd, tlm, check
from generic_eps_lib import safe_eps, get_eps_hk, eps_cmd, GENERIC_EPS_TEST_LOOP_COUNT

def run_generic_eps_app_test():
    ##
    ## This script tests the standard cFS component application functionality.
    ## Currently this includes: 
    ##   Housekeeping, request telemetry to be published on the software bus
    ##   NOOP, no operation but confirm correct counters increment
    ##   Reset counters, increment as done in NOOP and confirm ability to clear repeatably
    ##   Invalid ground command, confirm bad lengths and codes are rejected
    ##

    # Get to known state
    safe_eps()

    ##
    ##   Housekeeping, request telemetry to be published on the software bus
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        get_eps_hk()


    ##
    ## NOOP, no operation but confirm correct counters increment
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        eps_cmd("GENERIC_EPS_DEBUG GENERIC_EPS_NOOP_CC")


    ##
    ## Reset counters, increment as done in NOOP and confirm ability to clear repeatably
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        eps_cmd("GENERIC_EPS_DEBUG GENERIC_EPS_NOOP_CC")
        cmd("GENERIC_EPS_DEBUG GENERIC_EPS_RST_COUNTERS_CC") # Note standard `cmd` as we can't reset counters and then confirm increment
        get_eps_hk()
        check("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_COUNT == 0")
        check("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_ERR_COUNT == 0")


    ##
    ##   Invalid ground command, confirm bad lengths and codes are rejected
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        # Bad length
        cmd_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_COUNT")
        cmd_err_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_ERR_COUNT")
        cmd(f"GENERIC_EPS_DEBUG GENERIC_EPS_NOOP_CC with CCSDS_LENGTH {n+2}") # Note +2 due to CCSDS already being +1
        get_eps_hk()
        check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_COUNT == {cmd_cnt}")
        check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_ERR_COUNT == {cmd_err_cnt+1}")

    for n in range(6, (5 + GENERIC_EPS_TEST_LOOP_COUNT) + 1):
        # Bad command codes
        cmd_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_COUNT")
        cmd_err_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_ERR_COUNT")
        cmd(f"GENERIC_EPS_DEBUG GENERIC_EPS_NOOP_CC with CCSDS_FC {n+1}")
        get_eps_hk()
        check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_COUNT == {cmd_cnt}")
        check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM CMD_ERR_COUNT == {cmd_err_cnt+1}")