import os
import json
import matplotlib.pyplot as plt
from matplotlib import rc
import pickle 
import numpy as np
import sys 
import scipy.stats
sys.path.append('.')

colors = ["lightcoral","tab:red","pink","steelblue","lightskyblue","plum"]
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
    out_dir = "experiments/2021-10-06-15-41-35"

    files = os.listdir(out_dir)

    out_jsons = []
    for fname in files:
        with open(out_dir+"/"+fname+"/results.json", "r") as f:
            out_jsons.append(json.load(f))

    

    res = {}
    array_size = out_jsons[0]["array_size"]
    freq = 1e9
    batch_size = out_jsons[0]["batch_size"]
    imsize = out_jsons[0]["imsize"]

    no_seqs = [20, 60, 100]
    cnn_models = ["inception", "resnet50", "densenet121"]
    bert_models = ["bert_small", "bert_base", "bert_large"]

    #interconns = ["butterfly_0", "butterfly_1", "butterfly_2", "benes_with_copy", "benes_vanilla", "crossbar"]
    interconns = ["butterfly_1", "benes_with_copy", "benes_vanilla", "crossbar"]
    labels = ["Butterfly-2", "Benes w/ copy", "Benes", "Crossbar"]
    no_arrays = [32,64,128,256,512]

    interconn_power = {}
    total_power = {}
    for interconn in interconns:
        interconn_power[interconn] = {}
        total_power[interconn] = {}
        for no_array in no_arrays:
            switch_width = array_size[0] + 5 * array_size[1]

            if interconn == "crossbar":
                interconn_power[interconn][no_array] = Crossbar(no_array).calc_power(switch_width)
            elif interconn == "butterfly_0":
                interconn_power[interconn][no_array] = Butterfly(no_array).calc_power(switch_width) 
            elif interconn == "butterfly_1":
                interconn_power[interconn][no_array] = BanyanWithExpansion(no_array, exp = 1).calc_power(switch_width) 
            elif interconn == "butterfly_2":
                interconn_power[interconn][no_array] = BanyanWithExpansion(no_array, exp = 2).calc_power(switch_width)             
            elif interconn == "butterfly_3":
                interconn_power[interconn][no_array] = BanyanWithExpansion(no_array, exp = 3).calc_power(switch_width) 
            elif interconn == "benes_with_copy":
                interconn_power[interconn][no_array] = BenesWithCopy(no_array).calc_power(switch_width) 
            elif interconn == "benes_vanilla":
                interconn_power[interconn][no_array] = Benes(no_array).calc_power(switch_width)  
            else:
                raise NotImplementedError

            p_data = no_array * switch_width * e_data * freq
            p_compute = no_array * array_size[0] * array_size[1] * e_compute * freq

            total_power[interconn][no_array] = p_compute + p_data + interconn_power[interconn][no_array]

    with open('benchmarks.pickle', 'rb') as pickle_file:
        benchmarks = pickle.load(pickle_file)

    T_mean_norm = {}
    for interconn in interconns:
        throughputs = []
        for m in bert_models:
            res = []
            for no_seq in no_seqs:
                no_cycles = []
                for no_array in no_arrays:
                    c = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect":interconn, "sentence_len":no_seq}, out_jsons)
                    no_cycles.append(c)

                no_ops = calc_no_ops(benchmarks[m][batch_size][no_seq].layer_dims)
                res.append([no_ops / (n / freq) for n in no_cycles])
            throughputs.append(scipy.stats.hmean(res))

        for m in cnn_models:
            no_cycles = []
            for no_array in no_arrays:
                c = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect":interconn}, out_jsons)
                no_cycles.append(c)
            
            no_ops = calc_no_ops(benchmarks[m][batch_size][imsize].layer_dims)
            throughputs.append([no_ops / (n / freq) for n in no_cycles])

        T_hmean = scipy.stats.hmean(throughputs)
        #T_mean_norm[interconn] = [T_hmean[i]/total_power[interconn][no_arrays[i]]*no_arrays[i] for i in range(len(no_arrays))]
        T_mean_norm[interconn] = [T_hmean[i] for i in range(len(no_arrays))]

    for i, interconn in enumerate(interconns):
        tdps = [total_power[interconn][no_arrays[i]] for i in range(len(no_arrays))]
        eff_thru = T_mean_norm[interconn]
        print("interconn: {} eff. throughput: {} TDP: {}".format(interconn, ["{:.5}".format(v) for v in eff_thru], ["{:.5}".format(v) for v in tdps]))
        plt.plot(tdps, eff_thru, 
                    label=labels[i], color=colors[i], linewidth=3, markersize=10, linestyle=lines[i], marker=markers[i])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    plt.ylim(bottom=0, top=420)
    plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/interconn_throughput.png")

    exit()

    plt.figure()

    total_power = {}

    no_arrays = [1,2,4,8,16,32,64,128,256]
    for interconn in ["benes_copy", "benes_vanilla", "butterfly", "crossbar"]:

        power_perc = []
        total_power[interconn] = []
        for no_array in no_arrays:
            p_data = no_array * (array_size[0] + 5*array_size[1]) * e_data * freq
            p_compute = no_array * array_size[0] * array_size[1] *e_compute * freq

            if interconn == "benes_copy":
                p_interconn = 0.1e-3 * no_array * np.log2(no_array) * (array_size[0] + 5*array_size[1])
            elif interconn == "benes_vanilla":
                p_interconn = 0.1e-3/2 * no_array * np.log2(no_array) * (array_size[0] + 5*array_size[1])
            elif interconn == "butterfly":
                p_interconn = 0.1e-3/4 * no_array * np.log2(no_array) * (array_size[0] + 5*array_size[1])
            elif interconn == "crossbar":
                p_interconn = 0.1e-3/4 * no_array * no_array * (array_size[0] + 5*array_size[1])
            else:
                raise NotImplementedError

            total_power[interconn].append(p_data + p_compute + p_interconn)
            power_perc.append(100*p_interconn/(p_data + p_compute + p_interconn))

        plt.plot(no_arrays, [total_power[interconn][i]/no_arrays[i] for i in range(len(no_arrays))])

    plt.ylim(bottom=0, top=3)
    plt.legend(["Benes w/ copy network", "Vanilla Benes", "Butterfly","Crossbar"])
    plt.xlabel("Number of systolic pods")
    plt.ylabel("Total power consumption (W) per pod")
    plt.savefig("plots/interconn_power.png")


    exit()

    plt.figure()

    eff_thru_per_watt = [throughputs["benes"][i] / total_power["benes"][i] * no_arrays[i] for i in range(len(no_arrays))]
    plt.plot(no_arrays, eff_thru_per_watt)

    eff_thru_per_watt = [throughputs["butterfly"][i] / total_power["butterfly"][i] * no_arrays[i] for i in range(len(no_arrays))]
    plt.plot(no_arrays, eff_thru_per_watt)

    plt.savefig("plots/interconn_eff.png")