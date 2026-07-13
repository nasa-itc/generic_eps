from openc3.testing import Group, Suite
from generic_eps_lib import safe_eps
from generic_eps_app_test import run_generic_eps_app_test
from generic_eps_device_test import run_generic_eps_device_test
from generic_eps_ast_test import run_generic_eps_ast_test

class GENERIC_EPS_Functional_Test(Group):
    def setup(self):
        safe_eps()

    def script_application(self):
        run_generic_eps_app_test()

    def script_device(self):
        run_generic_eps_device_test()

    def teardown(self):
        safe_eps()

class GENERIC_EPS_Automated_Scenario_Test(Group):
    def setup(self): 
        safe_eps()

    def script_AST(self):
        run_generic_eps_ast_test()

    def teardown(self):
        safe_eps()

class Generic_eps_Test(Suite):
    def __init__(self):
        super().__init__()
        self.add_group('GENERIC_EPS_Functional_Test')
        self.add_group('GENERIC_EPS_Automated_Scenario_Test')

    def setup(self):
        safe_eps()
  
    def teardown(self):
        safe_eps()