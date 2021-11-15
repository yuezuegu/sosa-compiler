import os
import json
import numpy as np 
import matplotlib.pyplot as plt 
from plot_interconnect import get_result

e_data = 4.1e-12 #J per byte
e_compute = 0.4e-12 #J per MAC
I_0 = 0.45e-6
freq = 1e9 #Hz
tdp = 250


array_size = (32,32)
no_array = 128

exp_dir = "experiments/run-2021_11_13-12_09"

files = os.listdir(exp_dir)

out_jsons = []
for fname in files:
    try:
        f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
        out_jsons.append(json.load(f))
    except:
        print(fname + "/sim_result.json could not be opened!")

batch_sizes = [1,2,4,8]

models = ["resnet152", "bert_medium", "resnet152"]
extra_models = ["None", "None","bert_medium"]
sentence_len = 100

for ind, m in enumerate(models):
    extra_model = extra_models[ind]

    for batch_size in batch_sizes:
        args = {"model":m, "extra_models": [extra_model], "batch_size":batch_size}
        no_cycles = get_result("no_cycles", args, out_jsons)
        assert(no_cycles != -1)
        no_ops = get_result("no_ops", args, out_jsons)

        throughput = no_ops/(no_cycles/freq)*1e-12
        print(throughput)
        print(get_result("memory_stall_cycles", args, out_jsons))