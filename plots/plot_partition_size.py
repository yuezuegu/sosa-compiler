
import os
import json
import itertools 

import matplotlib.pyplot as plt
from matplotlib import rc
import pickle 
import numpy as np
import sys 
import scipy.stats
sys.path.append('.')

colors = ["pink","tab:red","lightcoral","mistyrose","lightsteelblue","lightskyblue","gray","steelblue",]
markers = ["P", "o", "d", "v", "s", "p",  "X", "D", "d", "s", "p", "P"]
lines = ['--', '-', '-.', ':', '-.', ':','--', '-.', ':','--', '-.', ':']

plt.figure(figsize=(7, 5))

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})


def get_result(target, fields, out_jsons):
    res = []
    for o in out_jsons:
        is_found = True
        for k in fields:
            if o[k] != fields[k]:
                is_found = False

        if is_found:
            res.append(o[target])
    
    if len(res)==0:
        print("Result not found for {}".format(fields))
        raise ValueError
    elif len(res)>1:
        tmp = res[0]
        for i in range(1, len(res)):
            if tmp == res[i]:
                #print("Warning: duplicate result found.")
                pass
            else:
                print("Multiple different results found for {}".format(fields))
                raise ValueError
        return res[0]
    else:
        return res[0]

if __name__=="__main__":
    exp_dir = "experiments/run-2021_11_05-16_44_43"

    files = os.listdir(exp_dir)

    out_jsons = []
    for fname in files:
        try:
            f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
            out_jsons.append(json.load(f))
        except:
            print(fname + "/sim_result.json could not be opened!")


    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]

    array_size = [32, 32]
    no_array = 128

    e_data = 4.1e-12 #J per byte
    e_compute = 0.4e-12 #J per MAC
    freq = 1e9

    PARTITION_SIZES = [8, 16, 32, 48, 64, 128, 256, 512, 1024, 100000000]
    throughputs = {}
    for b in PARTITION_SIZES:
        throughputs[b] = {}

        for m in bert_models:
            throughputs[b][m] = {}
            for no_seq in no_seqs:
                try:
                    args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq, "partition_size":b}
                    no_cycles = get_result("no_cycles", args, out_jsons)
                    no_ops = get_result("no_ops", args, out_jsons)
                    if no_cycles > 0:
                        throughputs[b][m][no_seq] = no_ops / (no_cycles / freq) / 1e12
                except:
                    pass
        
            throughputs[b][m] = scipy.stats.hmean(list(throughputs[b][m].values()))

        for m in cnn_models:
            try:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq, "partition_size":b}
                no_cycles = get_result("no_cycles", args, out_jsons)
                no_ops = get_result("no_ops", args, out_jsons)
                if no_cycles > 0:
                    throughputs[b][m] = no_ops / (no_cycles / freq) / 1e12
            except:
                pass

        throughputs[b] = scipy.stats.hmean(list(throughputs[b].values()))

    plt.plot(PARTITION_SIZES[:-2], list(throughputs.values())[:-2])

    # for i, interconn in enumerate(interconn_keys):
    #     tdps = [total_power[interconn][no_arrays[i]] for i in range(len(no_arrays))]
    #     eff_thru = T_mean_norm[interconn]
    #     print("interconn: {} eff. throughput: {} TDP: {}".format(interconn, ["{:.5}".format(v) for v in eff_thru], ["{:.5}".format(v) for v in tdps]))
    #     plt.plot(tdps, eff_thru, 
    #                 label=labels[i], color=colors[i], linewidth=3, markersize=10, linestyle=lines[i], marker=markers[i])

    #plt.ylabel('Effective Throughput (TeraOps/s)')
    #plt.xlabel('Partition size')
    plt.ylim(bottom=0)
    #plt.xlim(right=750)
    #plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/partition_size.png")
    #plt.savefig("plots/partition_size.pdf")

    exit()