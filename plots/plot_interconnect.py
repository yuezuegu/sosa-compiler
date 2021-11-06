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

    exp_dir = "experiments/run-2021_11_02-16_20_50"

    files = os.listdir(exp_dir)

    out_jsons = []
    for fname in files:
        try:
            f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
            out_jsons.append(json.load(f))
        except:
            print(fname + "/sim_result.json could not be opened!")

    interconn_keys = [128, 129, 130, 131, 16, 32]
    labels = ["Butterfly-1", "Butterfly-2", "Butterfly-4", "Butterfly-8", "Crossbar", "Benes"]

    no_arrays = [32, 64, 128, 256]
    array_size = [32, 32]

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]

    bert_models = ["bert_medium", "bert_base", "bert_large"]
    no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]

    e_data = 4.1e-12 #J per byte
    e_compute = 0.4e-12 #J per MAC
    freq = 1e9

    total_power = {}
    for interconn in interconn_keys:
        total_power[interconn] = {}
        for no_array in no_arrays:
            interconn_power = get_result("interconnect_tdp", {"no_array":no_array, "interconnect_type":interconn}, out_jsons)
            switch_width = array_size[0] + 5 * array_size[1]
            p_data = no_array * switch_width * e_data * freq
            p_compute = no_array * array_size[0] * array_size[1] * e_compute * freq
            total_power[interconn][no_array] = p_data + p_compute + interconn_power

    throughputs = {}
    for cnt, interconn in enumerate(interconn_keys):
        throughputs[interconn] = {}
        for no_array in no_arrays:
            throughputs[interconn][no_array] = {}

            for m in bert_models:
                throughputs[interconn][no_array][m] = {}
                for no_seq in no_seqs:
                    try:
                        no_cycles = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                        no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                        if no_cycles > 0:
                            thru = no_ops / (no_cycles / freq) / 1e12
                            throughputs[interconn][no_array][m][no_seq] = thru
                    except:
                        print("Result couldn't be found")
                thru_hmean = scipy.stats.hmean(list(throughputs[interconn][no_array][m].values()))
                print(f"{interconn} - {no_array} - {m}: {thru_hmean}")
                throughputs[interconn][no_array][m] = thru_hmean

            for m in cnn_models:
                try:
                    no_cycles = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    if no_cycles > 0:
                        thru = no_ops / (no_cycles / freq) / 1e12
                        print(f"{interconn} - {no_array} - {m}: {thru}")
                        throughputs[interconn][no_array][m] = thru
                except:
                    print("Result couldn't be found")
            #throughputs.append([no_ops / (n / freq) / 1e12 for n in no_cycles])

            throughputs[interconn][no_array] = scipy.stats.hmean(list(throughputs[interconn][no_array].values()))

        #T_hmean = scipy.stats.hmean(throughputs)
        #T_mean_norm[interconn] = [T_hmean[i] for i in range(len(no_arrays))]

            
        
        plt.plot(total_power[interconn].values(), throughputs[interconn].values(), 
                    label=labels[cnt], color=colors[cnt], linewidth=3, markersize=10, linestyle=lines[cnt], marker=markers[cnt])

    # for i, interconn in enumerate(interconn_keys):
    #     tdps = [total_power[interconn][no_arrays[i]] for i in range(len(no_arrays))]
    #     eff_thru = T_mean_norm[interconn]
    #     print("interconn: {} eff. throughput: {} TDP: {}".format(interconn, ["{:.5}".format(v) for v in eff_thru], ["{:.5}".format(v) for v in tdps]))
    #     plt.plot(tdps, eff_thru, 
    #                 label=labels[i], color=colors[i], linewidth=3, markersize=10, linestyle=lines[i], marker=markers[i])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    #plt.ylim(bottom=0, top=420)
    #plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/interconn_throughput.png")
    #plt.savefig("plots/interconn_throughput.pdf")
