import sys 
sys.path.append('.')
import json 
from io import StringIO

import pythonbinder

def run_csim(json_out):
	io = StringIO()
	json.dump(json_out, io)
	json_dump = io.getvalue()

	no_array = 1
	no_rows = 128
	no_cols = 128
	bank_size = 524288 
	bandwidth = 1200
	prefetch_limit = 10 
	interconnect_type = "crossbar"

	sim_res = pythonbinder.csim(json_dump, no_array, no_rows, no_cols, bank_size, bandwidth, prefetch_limit, interconnect_type)
	print(sim_res)



# fname = "experiments/tmp/precompiled_model.json"
# json_dump = open(fname, 'r').read()
# no_array = 1
# no_rows = 128
# no_cols = 128
# bank_size = 524288 
# bandwidth = 1200
# prefetch_limit = 10 
# interconnect_type = "crossbar"

# sim_res = pythonbinder.csim(json_dump, no_array, no_rows, no_cols, bank_size, bandwidth, prefetch_limit, interconnect_type)
# print(sim_res)


