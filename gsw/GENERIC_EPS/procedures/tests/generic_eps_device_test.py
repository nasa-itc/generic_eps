from generic_eps_lib import (
    safe_eps,
    get_eps_hk,
    confirm_eps_data_loop,
    GENERIC_EPS_TEST_LOOP_COUNT
)

def run_generic_eps_device_test():
    ##
    ## This script tests the cFS component device functionality.
    ## Currently this includes: 
    ##   Enable / disable, control hardware communications
    ##

    ##
    ## Enable / disable, control hardware communications
    ##
    for n in range(GENERIC_EPS_TEST_LOOP_COUNT):
        # Get to known state
        safe_eps()

        get_eps_hk()

        # Confirm device counters increment without errors
        confirm_eps_data_loop()