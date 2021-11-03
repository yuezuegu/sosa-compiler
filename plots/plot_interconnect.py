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

colors = ["lightcoral","tab:red","pink","steelblue","lightskyblue","plum","gray"]
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
    labels = ["banyan_exp_0", "banyan_exp_1", "banyan_exp_2", "banyan_exp_3", "crossbar", "benes_copy"]

    no_arrays = [32,64,128,256]
    array_size = [32, 32]

    # cnn_models = ["inception", "resnet50", "densenet121"]
    # bert_models = ["bert_small", "bert_base", "bert_large"]
    # no_seq = 100

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    cnn_models = list(itertools.product(cnn_models, [1]))

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
    bert_models = list(itertools.product(bert_models, no_seqs))

    e_data = 4.1e-12 #J per byte
    e_compute = 0.4e-12 #J per MAC
    freq = 1e9

    def get_interconn(key):
        return labels[interconn_keys.index(key)]

    cnt = {get_interconn(k): 0 for k in interconn_keys}
    for j in out_jsons:

        cnt[get_interconn(j["interconnect_type"])] += 1
        print(get_interconn(j["interconnect_type"]), " no_array: {}".format(j["no_array"]), " model: {}".format(j["model"]), " no_cycles: {}".format(j["no_cycles"]))


    print(cnt)

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

            for m, no_seq in bert_models:
                try:
                    no_cycles = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                    no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                    if no_cycles > 0:
                        throughputs[interconn][no_array][m] = no_ops / (no_cycles / freq) / 1e12
                except:
                    pass

            #throughputs.append([no_ops / (n / freq) / 1e12 for n in no_cycles])

            for m, no_seq in cnn_models:
                try:
                    no_cycles = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    if no_cycles > 0:
                        throughputs[interconn][no_array][m] = no_ops / (no_cycles / freq) / 1e12
                except:
                    pass
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
    plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/interconn_throughput.png")





    exit()

    out_dir = "experiments/2021-10-10-13-53-10"

    files = os.listdir(out_dir)

    out_jsons = []
    for fname in files:
        try:
            f = open(out_dir+"/"+fname+"/results.json", "r")
            out_jsons.append(json.load(f))
        except:
            print("result.json could not be opened!")
            

    res = {}
    array_size = out_jsons[0]["array_size"]
    freq = 1e9
    batch_size = out_jsons[0]["batch_size"]
    imsize = out_jsons[0]["imsize"]

    no_seqs = [100]
    cnn_models = ["inception", "resnet50", "densenet121"]
    bert_models = ["bert_small", "bert_base", "bert_large"]

    # interconn_keys = [65, 32, 48, 16]
    # labels = ["Butterfly-2", "Benes w/ copy", "Benes", "Crossbar"]
    
    interconn_keys = [129, 32, 16]
    labels = ["Butterfly-2", "Benes w/ copy", "Crossbar"]
        
    
    no_arrays = [32,64,128,256]

    e_data = 4.1e-12 #J per byte
    e_compute = 0.4e-12 #J per MAC

    total_power = {}
    for interconn in interconn_keys:
        total_power[interconn] = {}
        for no_array in no_arrays:
            interconn_power = get_result("interconnect_tdp", {"no_array":no_array, "interconnect_type":interconn}, out_jsons)
            switch_width = array_size[0] + 5 * array_size[1]
            p_data = no_array * switch_width * e_data * freq
            p_compute = no_array * array_size[0] * array_size[1] * e_compute * freq
            total_power[interconn][no_array] = p_data + p_compute + interconn_power



    T_mean_norm = {}
    for interconn in interconn_keys:
        throughputs = []
        for m in bert_models:
            res = []
            for no_seq in no_seqs:
                no_cycles = []
                for no_array in no_arrays:
                    c = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                    no_cycles.append(c)

                no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn, "sentence_len":no_seq}, out_jsons)
                res.append([no_ops / (n / freq) / 1e12 for n in no_cycles])
            throughputs.append(scipy.stats.hmean(res))

        for m in cnn_models:
            no_cycles = []
            for no_array in no_arrays:
                c = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                no_cycles.append(c)
            
            no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
            throughputs.append([no_ops / (n / freq) / 1e12 for n in no_cycles])

        T_hmean = scipy.stats.hmean(throughputs)
        T_mean_norm[interconn] = [T_hmean[i] for i in range(len(no_arrays))]

    for i, interconn in enumerate(interconn_keys):
        tdps = [total_power[interconn][no_arrays[i]] for i in range(len(no_arrays))]
        eff_thru = T_mean_norm[interconn]
        print("interconn: {} eff. throughput: {} TDP: {}".format(interconn, ["{:.5}".format(v) for v in eff_thru], ["{:.5}".format(v) for v in tdps]))
        plt.plot(tdps, eff_thru, 
                    label=labels[i], color=colors[i], linewidth=3, markersize=10, linestyle=lines[i], marker=markers[i])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    #plt.ylim(bottom=0, top=420)
    plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/interconn_throughput.png")