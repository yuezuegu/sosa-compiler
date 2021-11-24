import os
import json
import matplotlib.pyplot as plt
from matplotlib import rc
import pickle 
import scipy.stats

import sys 
sys.path.append('.')

from helpers import calculate_peak_power, calculate_peak_throughput

import itertools
import numpy as np

markers = ["o", "P", "*", "v", "s", "p",  "X", "D", "d", "s", "p", "P"]
lines = ['-',':', '-.', '--', '-.', ':','--', '-.', ':','--', '-.', ':']

#e_data = 4.1e-12 #J per byte
e_data = 2.7e-12 #J per byte (256kB)
#e_data = 1.6e-12 #J per byte (128kB)

e_compute = 0.4e-12 #J per MAC
I_0 = 2.875e-5 #W per byte
#I_0 = 0
freq = 1e9 #Hz
tdp = 400


model_names = ["Bert-medium", "Bert-base", "Bert-large", "Densenet121", "Densenet169", "Densenet201", "Inception", "Resnet50", "Resnet101", "Resnet152", "Harm. mean"]

cnn_models = ["densenet121", "densenet169", "densenet201", "inception", "resnet50",  "resnet101",  "resnet152"]



bert_models = ["bert_small", "bert_base", "bert_large"]
#no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
no_seqs = [100]


imsize = [299]
batch_size = 1

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

def parse_out_jsons(exp_dirs):
    out_jsons = []
    for exp_dir in exp_dirs:
        files = os.listdir(exp_dir)
        
        for fname in files:
            try:
                f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
                out_jsons.append(json.load(f))
            except:
                print(fname + "/sim_result.json could not be opened!")
    return out_jsons


def draw_bar(ax, data, x_labels, group_labels, figsize=(16, 7), colors=None):
    x = np.arange(len(x_labels))  # the label locations

    no_groups = len(group_labels)

    gap = 0.3
    width = (1. - gap) / no_groups  # the width of the bars

    #['/', '\\','.', 'o', '|', '-', '+', 'x',  'O',  '*']
    hatches = ['-','.','/','o','\\','.']
    
    for ind, y in enumerate(data):
        ax.bar(x - (1 - gap) / 2 + width*ind, y, width-0.05, label=group_labels[ind], hatch=hatches[ind], color=colors[ind])

    ax.set_xticks(x)

def hmean_dict(d):
    return scipy.stats.hmean([d[k] for k in d])

def plot_main_results(exp_dirs):
    model_names = ["Bert-medium", "Bert-base", "Bert-large", "Densenet121", "Densenet169", "Densenet201", "Inception", "Resnet50", "Resnet101", "Resnet152", "Harm. mean"]
    cnn_models = ["densenet121", "densenet169", "densenet201", "inception", "resnet50",  "resnet101",  "resnet152"]
    bert_models = ["bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]

    barchart_settings = {
        "16x16": ((16,16),512),
        "64x64": ((64,64),128),
        "128x128": ((128,128),32),
        "256x256": ((256,256),8),
        "Monolithic": ((512,512),1),
        "SOSA (32x32)": ((32,32),256),
        }


    print("Main results: ")

    out_jsons = parse_out_jsons(exp_dirs)

    eff_throughputs = {}
    peak_powers = {}
    for ind, label in enumerate(barchart_settings):
        array_size, no_array = barchart_settings[label]
        eff_throughputs[label] = {}

        for m in bert_models:
            throughput_bert = []
            for no_seq in no_seqs:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
                no_cycles = get_result("no_cycles", args, out_jsons)
                assert(no_cycles != -1)
                no_ops = get_result("no_ops", args, out_jsons)

                throughput_bert.append(no_ops / (no_cycles / freq) * 1e-12)

            eff_throughputs[label][m] = scipy.stats.hmean(throughput_bert)

        for m in cnn_models:
            args = {"array_size":list(array_size), "no_array":no_array, "model":m}
            no_cycles = get_result("no_cycles", args, out_jsons)
            assert(no_cycles != -1)
            no_ops = get_result("no_ops", args, out_jsons)

            eff_throughputs[label][m] = no_ops / (no_cycles / freq) * 1e-12

        eff_throughputs[label]["hmean"] = hmean_dict(eff_throughputs[label])

        peak_powers[label],_,_,_ = calculate_peak_power(array_size[0], array_size[1], no_array, freq, e_data, e_compute, I_0)

        eff_throughput_norm = eff_throughputs[label]["hmean"] / peak_powers[label] * tdp
        peak_throughput = calculate_peak_throughput(array_size[0], array_size[1], no_array, freq)
        peak_throughput_norm = peak_throughput / peak_powers[label] * tdp
        util = eff_throughput_norm / peak_throughput_norm

        print("{}x{} - {}:\t peak power:{:.4}\t eff. thru:{:.4}\t norm. eff. thru:{:.4}\t peak thru:{:.4}\t norm. peak thru:{:.4}\t util:{:.4}".format(array_size[0], array_size[1], no_array, peak_powers[label], eff_throughputs[label]["hmean"], eff_throughput_norm, peak_throughput, peak_throughput_norm, util))

    print("\nBarchart data:")

    barchart_data = []
    for ind, label in enumerate(barchart_settings):
        barchart_data.append([])
        for m in bert_models + cnn_models + ["hmean"]:
            T_normalized = eff_throughputs[label][m] / peak_powers[label] * tdp 
            barchart_data[-1].append(T_normalized)
            
            print(f"{label}:\t model:{m}\t - norm. eff. thru:{T_normalized}")

    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})
    colors = ["teal","steelblue","tab:gray","slategrey","darkseagreen","tab:red"]
    fig, ax = plt.subplots(figsize=(16, 5.5))
    draw_bar(ax, barchart_data, model_names, list(barchart_settings.keys()), colors=colors)

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Effective Throughput (TeraOps/s)')
    ax.set_xticklabels(model_names, rotation=30)

    plt.ylim()
    ax = plt.gca()
    ax.set_axisbelow(True)
    plt.grid(True, axis='y')
    plt.legend(loc='lower center', bbox_to_anchor=(0.5, 1.), ncol=6, frameon=False)

    plt.tight_layout()
    plt.show()
    plt.savefig('plots/results.png')
    plt.savefig('plots/results.pdf')


def plot_array_scale(exp_dirs):
    print("\nArray scale results:")

    cnn_models = ["densenet121", "densenet169", "densenet201", "inception", "resnet50",  "resnet101",  "resnet152"]
    bert_models = ["bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]

    baselines = {
        "SOSA (32x32)": list(itertools.product([(32,32)], [32,64,128,256,512])),
        "16x16": list(itertools.product([(16,16)], [128,256,512,1024])),
        "64x64": list(itertools.product([(64,64)], [16,32,64,128,196])),
        "128x128": list(itertools.product([(128,128)], [1,2,4,8,16,24,32,48,64])),
        "256x256": list(itertools.product([(256,256)], [1, 2, 4, 8, 16])),
        "Monolithic": list(itertools.product([(400,400),(512,512),(800,800),(1024,1024),], [1])),
        }

    out_jsons = parse_out_jsons(exp_dirs)

    eff_throughputs = {}
    peak_powers = {}
    for ind, label in enumerate(baselines):
        eff_throughputs[label] = {}
        peak_powers[label] = {}
        for array_size, no_array in baselines[label]:
            eff_throughputs[label][(array_size, no_array)] = {}

            for m in bert_models:
                throughput_bert = []
                for no_seq in no_seqs:
                    args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
                    no_cycles = get_result("no_cycles", args, out_jsons)
                    no_ops = get_result("no_ops", args, out_jsons)

                    throughput_bert.append(no_ops / (no_cycles / freq) * 1e-12)

                eff_throughputs[label][(array_size, no_array)][m] = scipy.stats.hmean(throughput_bert)

            for m in cnn_models:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m}
                no_cycles = get_result("no_cycles", args, out_jsons)
                no_ops = get_result("no_ops", args, out_jsons)

                eff_throughputs[label][(array_size, no_array)][m] = no_ops / (no_cycles / freq) * 1e-12

            eff_throughputs[label][(array_size, no_array)]["hmean"] = hmean_dict(eff_throughputs[label][(array_size, no_array)])

            peak_powers[label][(array_size, no_array)],_,_,_ = calculate_peak_power(array_size[0], array_size[1], no_array, freq, e_data, e_compute, I_0)

            T_hmean = eff_throughputs[label][(array_size, no_array)]["hmean"]
            P_peak = peak_powers[label][(array_size, no_array)]
            T_normalized = T_hmean / P_peak * tdp 
            print(f"{array_size[0]}x{array_size[1]} - {no_array}:\t T_hmean:{T_hmean}\t P_peak:{P_peak}\t T_norm:{T_normalized}")

    plt.figure(figsize=(7, 5))
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})
    colors = ["tab:red", "teal","steelblue","tab:gray","slategrey","darkseagreen"]

    for ind, label in enumerate(baselines):
        eff_vals = [hmean_dict(eff_throughputs[label][(array_size, no_array)]) for (array_size, no_array) in eff_throughputs[label]]

        plt.plot(peak_powers[label].values(), eff_vals, label=label,
                    linewidth=3, linestyle=lines[ind], marker=markers[ind], markersize=10, color=colors[ind])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    plt.xlim(left=0)
    plt.ylim(bottom=0, top=300)

    ax = plt.gca()
    ax.legend(fontsize='small', ncol=2, frameon=False, loc='upper left')

    plt.tight_layout()

    plt.savefig("plots/array_scale.png")
    plt.savefig("plots/array_scale.pdf")


def plot_interconnect(exp_dirs):
    print("\nInterconnect results:")

    cnn_models = ["densenet121", "densenet169", "densenet201", "inception", "resnet50",  "resnet101",  "resnet152"]
    bert_models = ["bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]

    out_jsons = parse_out_jsons(exp_dirs)

    plt.figure(figsize=(7, 5))
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

    colors = ["pink","tab:red","lightcoral","mistyrose","lightsteelblue","lightskyblue","gray","steelblue",]
    markers = ["P", "o", "d", "v", "s", "p",  "X", "D", "d", "s", "p", "P"]
    lines = ['--', '-', '-.', ':', '-.', ':','--', '-.', ':','--', '-.', ':']

    interconn_keys = [128, 129, 130, 131, 16, 32]
    ict_types = {128:"banyan_exp_0", 129:"banyan_exp_1", 130:"banyan_exp_2", 131:"banyan_exp_3", 16:"crossbar", 32:"benes_copy"}
    labels = ["Butterfly-1", "Butterfly-2", "Butterfly-4", "Butterfly-8", "Crossbar", "Benes"]

    no_arrays = [32, 64, 128, 256]
    array_size = [32, 32]

    total_power = {}
    throughputs = {}
    for cnt, interconn in enumerate(interconn_keys):
        throughputs[interconn] = {}
        total_power[interconn] = {}
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
                        else:
                            print("no of cycles is less than zero")
                    except:
                        print("Result couldn't be found")
                thru_hmean = scipy.stats.hmean(list(throughputs[interconn][no_array][m].values()))
                #print(f"{interconn} - {no_array} - {m}: {thru_hmean}")
                throughputs[interconn][no_array][m] = thru_hmean

            for m in cnn_models:
                try:
                    no_cycles = get_result("no_cycles", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    no_ops = get_result("no_ops", {"no_array":no_array, "model":m, "interconnect_type":interconn}, out_jsons)
                    if no_cycles > 0:
                        thru = no_ops / (no_cycles / freq) / 1e12
                        #print(f"{interconn} - {no_array} - {m}: {thru}")
                        throughputs[interconn][no_array][m] = thru
                    else:
                        print("no of cycles is less than zero")
                except:
                    print("Result couldn't be found")

            throughputs[interconn][no_array] = scipy.stats.hmean(list(throughputs[interconn][no_array].values()))
            total_power[interconn][no_array],_,_,_ = calculate_peak_power(array_size[0], array_size[1], no_array, freq, e_data, e_compute, I_0, ict_type=ict_types[interconn])
            print(f"ICT:{ict_types[interconn]}\t N:{no_array}\t eff. thru:{throughputs[interconn][no_array]}\t power:{total_power[interconn][no_array]}")

        plt.plot(total_power[interconn].values(), throughputs[interconn].values(), 
                    label=labels[cnt], color=colors[cnt], linewidth=3, markersize=10, linestyle=lines[cnt], marker=markers[cnt])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    plt.ylim(bottom=0, top=300)
    #plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)
    plt.tight_layout()
    plt.savefig("plots/interconn_throughput.png")
    plt.savefig("plots/interconn_throughput.pdf")

def plot_partition_size(exp_dirs):
    print("\nPartition size: ")
    out_jsons = parse_out_jsons(exp_dirs)

    plt.figure(figsize=(7, 5))
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]

    bert_models = ["bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]

    array_size = [32, 32]
    no_array = 256

    PARTITION_SIZES = [8, 16, 32, 48, 64, 128, 256, 512]
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
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "partition_size":b}
                no_cycles = get_result("no_cycles", args, out_jsons)
                no_ops = get_result("no_ops", args, out_jsons)
                if no_cycles > 0:
                    throughputs[b][m] = no_ops / (no_cycles / freq) / 1e12
            except:
                pass

        throughputs[b] = scipy.stats.hmean(list(throughputs[b].values()))
        print(f"P:{b} - eff. thru:{throughputs[b]}")

    plt.plot(PARTITION_SIZES[:-2], list(throughputs.values())[:-2], label='Fixed partition', linewidth=3, linestyle=lines[0], marker=markers[0], markersize=10, color="steelblue")
    plt.axhline(y = throughputs[PARTITION_SIZES[-1]], label='No partition', color = 'tab:gray', linewidth=2, linestyle = '--')

    plt.ylabel("Effective Throughput (TeraOps/s)")
    plt.xlabel("Partition size of X's first dimension")
    plt.ylim(bottom=0)
    #plt.xlim(right=750)
    plt.legend(fontsize='small',frameon=False)

    plt.tight_layout()
    plt.savefig("plots/partition_size.png")
    plt.savefig("plots/partition_size.pdf")

def plot_multibatch(exp_dirs):
    print("\nMulti-batch: ")

    out_jsons = parse_out_jsons(exp_dirs)

    batch_sizes = [1,2,4,8]
    no_array = 256

    models = ["resnet152", "bert_medium", "resnet152"]
    #models = ["densenet121", "bert_medium", "densenet121"]
    extra_models = ["None", "None","bert_medium"]

    throughputs = []
    for batch_size in batch_sizes:
        T = []

        for ind, m in enumerate(models):
            extra_model = extra_models[ind]

            args = {"model":m, "extra_models": [extra_model], "batch_size":batch_size, "no_array":no_array}
            no_cycles = get_result("no_cycles", args, out_jsons)
            assert(no_cycles != -1)
            no_ops = get_result("no_ops", args, out_jsons)

            throughput = no_ops/(no_cycles/freq)*1e-12
            T.append(throughput)

        throughputs.append(T)

    cnn_batch1 = throughputs[0][0]
    bert_batch1 = throughputs[0][1]

    mean_batch1 = scipy.stats.hmean([cnn_batch1, bert_batch1])
    multi_batch1 = throughputs[0][2]

    print(f"cnn_batch1: {cnn_batch1}\t bert_batch1: {bert_batch1}")
    print("Expected hmean throughput of sequential CNN + BERT: {}, measured throughput they run in parallel {}, speedup thanks to multitenancy: {}".format(mean_batch1, multi_batch1, multi_batch1/mean_batch1))

    model_names = ["Resnet", "BERT", "Resnet+BERT"]
    baselines = ["1","2","4","8"]

    fig, ax = plt.subplots(figsize=(7, 5))

    colors = ["lightslategrey","lightsteelblue","powderblue","darkseagreen"]
    draw_bar(ax, throughputs, model_names, baselines, colors = colors)

    ax.set_ylabel('Effective Throughput (TeraOps/s)')
    ax.set_xticklabels(model_names, rotation=0)

    ax.set_axisbelow(True)
    plt.grid(True, axis='y')
    plt.legend(loc='lower center', title='Batch size:', bbox_to_anchor=(0.5, 1.), ncol=5, frameon=False)

    plt.tight_layout()
    plt.show()
    plt.savefig('plots/multibatch.png')
    plt.savefig('plots/multibatch.pdf')

def plot_bank_size(exp_dirs):
    out_jsons = parse_out_jsons(exp_dirs)

    plt.figure(figsize=(7, 5))
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

    array_size = (32,32)
    no_array = 256
    batch_size = 8

    cnn_models = ["resnet152"]
    bert_models = []

    bank_sizes = [65536, 131072, 262144, 524288, 1048576]

    eff_throughputs = []
    peak_powers = []
    bw_usage = []
    for ind, bank_size in enumerate(bank_sizes):

        bw_usage.append(0)
        res_hmean = []
        for m in bert_models:
            res = []
            for no_seq in no_seqs:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq, "bank_size":bank_size, "batch_size":batch_size}
                no_cycles = get_result("no_cycles", args, out_jsons)
                assert(no_cycles != -1)
                no_ops = get_result("no_ops", args, out_jsons)
                bw_usage[-1] += get_result("total_bw_usage", args, out_jsons)
                #print("{}kB:\t m:{}\t BW:{:.4}".format(bank_size/1024, m, get_result("total_bw_usage", args, out_jsons)))

                res.append(no_ops / (no_cycles / freq) * 1e-12)

            res_hmean.append(scipy.stats.hmean(res))

        for m in cnn_models:
            args = {"array_size":list(array_size), "no_array":no_array, "model":m, "bank_size":bank_size, "batch_size":batch_size}
            no_cycles = get_result("no_cycles", args, out_jsons)
            assert(no_cycles != -1)
            no_ops = get_result("no_ops", args, out_jsons)
            bw_usage[-1] += get_result("total_bw_usage", args, out_jsons)
            #print("{}kB:\t m:{}\t BW:{:.4}".format(bank_size/1024, m, get_result("total_bw_usage", args, out_jsons)))

            res_hmean.append(no_ops / (no_cycles / freq) * 1e-12)

        eff_throughputs.append(scipy.stats.hmean(res_hmean))

        print("{}kB:\t T:{:.4}\t BW:{:.4}".format(bank_size/1024, eff_throughputs[-1], bw_usage[-1]))

    fig, ax1 = plt.subplots()

    plt.xlabel("SRAM Bank size (kB)")
    plt.xticks([b/1024 for b in bank_sizes], rotation=45)
    plt.xlim(left=0)

    plot1 = ax1.plot([b/1024 for b in bank_sizes], eff_throughputs/eff_throughputs[-1], label="Eff. Throughput", linewidth=3, marker=markers[1], linestyle=lines[0], markersize=10, color="steelblue")
    ax1.set_ylabel("Normalized Eff. Throughput")
    ax1.set_ylim(bottom=0)


    ax2 = ax1.twinx()

    plot2 = ax2.plot([b/1024 for b in bank_sizes], [bw/1024/1024 for bw in bw_usage], label="DRAM BW Usage", linewidth=3, marker=markers[0], linestyle=lines[1], markersize=10, color="teal")
    ax2.set_ylabel("DRAM Bandwidth Usage (MB)")
    ax2.set_ylim(bottom=0)


    lns = plot1+plot2
    labs = [l.get_label() for l in lns]
    plt.legend(lns, labs, loc='center right', fontsize='small',frameon=False)

    #plt.legend()
    plt.tight_layout()
    plt.savefig("plots/memory.png")
    plt.savefig("plots/memory.pdf")

def plot_memory(exp_dirs):
    out_jsons = parse_out_jsons(exp_dirs)

    for m in cnn_models+bert_models:
        args = {"model":m}
        total_no_gemm_ops = get_result("total_no_gemm_ops", args, out_jsons)  
        no_layers = get_result("no_layers", args, out_jsons)        
        print("{}: {}".format(m, total_no_gemm_ops/no_layers))

if __name__=="__main__":
    interconnect_dirs = ["experiments/run-2021_11_17-17_59"]
    #partition_size_dirs = ["experiments/run-2021_11_22-12_07_58"]
    partition_size_dirs = ["experiments/run-2021_11_23-10_37_46"]
    array_scale_dirs = ["experiments/run-2021_11_17-20_24", "experiments/run-2021_11_15-13_45", "experiments/run-2021_11_23-10_34_21", "experiments/run-2021_11_23-17_06_52"]
    #array_scale_dirs = ["experiments/run-2021_11_20-10_47_21"]

    #multibatch_dirs = ["experiments/run-2021_11_17-21_12"]
    #multibatch_dirs = ["experiments/run-2021_11_22-12_10_22"]
    #multibatch_dirs = ["experiments/run-2021_11_23-10_38_49"]
    multibatch_dirs = ["experiments/run-2021_11_23-21_37_33"]


    bank_size_dirs = ["experiments/run-2021_11_17-21_13"]
    #bank_size_dirs = ["experiments/run-2021_11_23-10_38_49"]
    memory_dirs = ["experiments/run-2021_11_21-13_07_55"]

    plot_memory(memory_dirs)
    plot_interconnect(interconnect_dirs)
    plot_partition_size(partition_size_dirs)
    plot_main_results(array_scale_dirs)
    plot_array_scale(array_scale_dirs)
    plot_multibatch(multibatch_dirs)
    #plot_bank_size(bank_size_dirs)