from openc3.script import tlm, wait_check
import time
from generic_eps_lib import (
    eps_prepare_ast, 
    eps_sim_disable, 
    eps_sim_enable, 
    confirm_eps_data_loop,
    get_eps_hk,
    GENERIC_EPS_TEST_LOOP_COUNT,
    GENERIC_EPS_RESPONSE_TIMEOUT
)

def run_generic_eps_ast_test():
    ##
    ## This script tests the cFS component in an automated scenario.
    ## Currently this includes: 
    ##   Hardware failure
    ##   Hardware status reporting fault
    ##

    ##
    ## Hardware failure
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        # Prepare
        eps_prepare_ast()

        # Disable sim and confirm device error counts increase
        dev_cmd_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM DEVICE_COUNT")
        dev_cmd_err_cnt = tlm("GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM DEVICE_ERR_COUNT")
        eps_sim_disable()
        
        # Allow simulator bridge fault state to process
        time.sleep(1)
        
        # Actively force FSW to poll dead hardware
        get_eps_hk()
        
        wait_check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM DEVICE_COUNT >= {dev_cmd_cnt}", GENERIC_EPS_RESPONSE_TIMEOUT)
        wait_check(f"GENERIC_EPS_DEBUG GENERIC_EPS_HK_TLM DEVICE_ERR_COUNT >= {dev_cmd_err_cnt}", GENERIC_EPS_RESPONSE_TIMEOUT)

        # Enable sim and confirm return to nominal operation
        eps_sim_enable()
        confirm_eps_data_loop()