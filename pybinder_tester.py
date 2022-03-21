import sys 
sys.path.append('./build-Release')
import json 
from io import StringIO


from precompiler.precompile import precompile_model
from precompiler.benchmarks import benchmark, get_benchmarks

import pythonbinder


# This is an example code of python binder for C binaries

# Get RESNET50 model
bm = get_benchmarks("resnet50", batch_size=1, image_size=224, seq_len=None)

# Convert Pytorch model to keras
keras_model = bm.get_keras_model() 

# Precompiler the keras model for a given array size
layers = precompile_model(keras_model, array_size=[32,32], partition_size=None)

# Encode precompiled model in JSON format
json_out = {"args":{}, "model1":{"order":list(layers.keys()), "layers":layers, "no_repeat":1, "no_ops":bm.no_ops}}
io = StringIO()
json.dump(json_out, io)
json_dump = io.getvalue()

# Parameters for the test
no_array = 1
no_rows = 128
no_cols = 128
bank_size = 524288 
bandwidth = 1200
prefetch_limit = 10 
interconnect_type = "crossbar"

# Call the C binaries
sim_res = pythonbinder.csim(json_dump, no_array, no_rows, no_cols, bank_size, bandwidth, prefetch_limit, interconnect_type)

# Print results
print(sim_res)
