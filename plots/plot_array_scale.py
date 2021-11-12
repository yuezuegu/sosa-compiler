import os
import json
import matplotlib.pyplot as plt
from matplotlib import rc
import pickle 
import scipy.stats

import sys 
sys.path.append('.')

from helpers import calculate_peak_power, calculate_peak_throughput

from plot_interconnect import get_result

import itertools
import numpy as np

markers = ["o", "P", "*", "v", "s", "p",  "X", "D", "d", "s", "p", "P"]
lines = ['-',':', '-.', '--', '-.', ':','--', '-.', ':','--', '-.', ':']

colors = ["tab:red","teal","steelblue","tab:gray","slategrey","mediumpurple"]

plt.figure(figsize=(7, 5))
rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

e_data = 4.1e-12 #J per byte
e_compute = 0.4e-12 #J per MAC
I_0 = 0.45e-6
freq = 1e9 #Hz
tdp = 250

exp_dir = "experiments/run-2021_11_11-14_00"

files = os.listdir(exp_dir)

out_jsons = []
for fname in files:
    try:
        f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
        out_jsons.append(json.load(f))
    except:
        print(fname + "/sim_result.json could not be opened!")

cnn_models = ["densenet121", "densenet169", "densenet201", "inception", "resnet50",  "resnet101",  "resnet152"]
bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
imsize = [299]
batch_size = 1

baselines = {
    "SOSA (32x32)": list(itertools.product([(32,32)], [32,64,128,256])),
    "128x128": list(itertools.product([(128,128)], [8,16,24,32])),
    "256x256": list(itertools.product([(256,256)], [1, 2, 4, 8, 12])),
    "Monolithic": list(itertools.product([(400,400),(512,512),(800,800),(960,960)], [1]))
    }

def draw_bar(ax, data, x_labels, group_labels, figsize=(16, 7)):
    x = np.arange(len(x_labels))  # the label locations

    no_groups = len(group_labels)

    gap = 0.3
    width = (1. - gap) / no_groups  # the width of the bars

    #['/', '\\','.', 'o', '|', '-', '+', 'x',  'O',  '*']
    hatches = ['-','.','/','o','\\']
    colors = ["lightslategrey","lightsteelblue","powderblue","darkseagreen","tab:red"]
    for ind, y in enumerate(data):
        ax.bar(x - (1 - gap) / 2 + width*ind, y, width-0.05, label=group_labels[ind], hatch=hatches[ind], color=colors[ind])

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax.set_ylabel('Effective Throughput (TeraOps/s)')
    ax.set_xticks(x)
    ax.set_xticklabels(x_labels, rotation=30)

def plot_main_results():

    baselines = { 'Baseline A (512x512)':[(512,512), 1], 'Baseline B (256x256)':[(256,256),4], 'Baseline C (128x128)':[(128,128),16], 'Baseline D (8x8)':[(8,8), 1024], 'SOSA (32x32)':[(32,32), 128]}
    model_name = ["Bert-tiny","Bert-small","Bert-medium","Bert-base","Bert-large","Densenet121", "Densenet169", "Densenet201", "Inception", "Resnet50", "Resnet101","Resnet152","Harm. mean"]

    throughputs = []
    for label in baselines:
        array_size, no_array = baselines[label]
        r, c = array_size

        P_peak = calculate_peak_power(r, c, no_array, freq, e_data, e_compute, I_0)[0]
        T_peak = calculate_peak_throughput(r, c, no_array,freq) / P_peak * tdp

        if r < 16 and c < 16:
            T = []
            for i in range(len(cnn_models+bert_models)):
                T.append(calculate_peak_throughput(r,c,no_array,freq))

            T_pscaled = [t / P_peak * tdp for t in T]
            T_hmean = scipy.stats.hmean(T_pscaled)
            throughputs.append(T_pscaled+[T_hmean])

            print("{}x{}\t No:array:{}\t Peak Power: {}\t Peak Throughput: {}\t Utilization: {}\t Eff. Throughput: {}".format(r, c, no_array, P_peak, T_peak, T_hmean/T_peak*100, T_hmean))
        else:
            T = []

            for m in bert_models:
                T_tmp = []
                for no_seq in no_seqs:
                    args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
                    no_cycles = get_result("no_cycles", args, out_jsons)
                    assert(no_cycles != -1)
                    no_ops = get_result("no_ops", args, out_jsons)

                    # cycles = get_result("no_cycles", {"array_size":[r,c], "no_array":no_array, "model":m, "sentence_len":no_seq}, out_jsons)
                    # t_exec = cycles*(1/freq)
                    # ops = calc_no_ops(benchmarks[m][batch_size][no_seq].layer_dims)

                    T_tmp.append(no_ops/(no_cycles/freq)*1e-12)

                T_hmean = scipy.stats.hmean(T_tmp)
                T.append(T_hmean)

            for m in cnn_models:
                # cycles = get_result("no_cycles", {"array_size":[r,c], "no_array":no_array, "model":m}, out_jsons)
                # t_exec = cycles*(1/freq)
                # ops = calc_no_ops(benchmarks[m][batch_size][imsize].layer_dims)

                args = {"array_size":list(array_size), "no_array":no_array, "model":m}
                no_cycles = get_result("no_cycles", args, out_jsons)
                assert(no_cycles != -1)
                no_ops = get_result("no_ops", args, out_jsons)

                T.append(no_ops/(no_cycles/freq)*1e-12)

            #T = [total_ops[i]/t for i,t in enumerate(t_exec)]
            T_pscaled = [t / P_peak * tdp for t in T]
            T_hmean = scipy.stats.hmean(T_pscaled)
            throughputs.append(T_pscaled+[T_hmean])

            print("{}x{}\t No:array:{}\t Peak Power: {}\t Peak Throughput: {}\t Utilization: {}\t Eff. Throughput: {}".format(r, c, no_array, P_peak, T_peak, T_hmean/T_peak*100, T_hmean))

    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

    fig, ax = plt.subplots(figsize=(16, 5.5))

    print("Results:")
    # for base in throughputs:
    #     print(["{}: {:.2f}".format(model_name[ind], t) for ind, t in enumerate(base)])

    draw_bar(ax, throughputs, model_name, list(baselines.keys()))

    plt.ylim()
    ax = plt.gca()
    ax.set_axisbelow(True)
    plt.grid(True, axis='y')
    plt.legend(loc='lower center', bbox_to_anchor=(0.5, 1.), ncol=3, frameon=False)

    plt.tight_layout()
    plt.show()
    plt.savefig('plots/results.pdf')




def plot_array_scale():
    for ind, label in enumerate(baselines):
        eff_throughputs = []
        peak_powers = []
        for array_size, no_array in baselines[label]:

            res_hmean = []
            for m in bert_models:
                res = []
                for no_seq in no_seqs:
                    args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
                    no_cycles = get_result("no_cycles", args, out_jsons)
                    assert(no_cycles != -1)
                    no_ops = get_result("no_ops", args, out_jsons)

                    res.append(no_ops / (no_cycles / freq) * 1e-12)

                res_hmean.append(scipy.stats.hmean(res))

            for m in cnn_models:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m}
                no_cycles = get_result("no_cycles", args, out_jsons)
                assert(no_cycles != -1)
                no_ops = get_result("no_ops", args, out_jsons)

                res_hmean.append(no_ops / (no_cycles / freq) * 1e-12)

            eff_throughputs.append(scipy.stats.hmean(res_hmean))

            args = {"array_size":list(array_size), "no_array":no_array, "model":m}
            interconn_power = get_result("interconnect_tdp", args, out_jsons)
            switch_width = array_size[0] + 5 * array_size[1]
            p_data = no_array * switch_width * e_data * freq
            p_compute = no_array * array_size[0] * array_size[1] * e_compute * freq
            peak_powers.append(p_data + p_compute + interconn_power)

            print("{}x{} - {}:\t T:{:.4}\t P:{:.4}".format(array_size[0], array_size[1], no_array, eff_throughputs[-1], peak_powers[-1]))

        plt.plot(peak_powers, eff_throughputs, label=label,
                    linewidth=3, linestyle=lines[ind], marker=markers[ind], markersize=10, color=colors[ind])

    plt.ylabel('Effective Throughput (TeraOps/s)')
    plt.xlabel('TDP (Watts)')
    #plt.xlim(right=750)
    #plt.ylim(bottom=0, top=420)

    ax = plt.gca()
    handles, labels = ax.get_legend_handles_labels()

    legend_order = [0,1,2,3]

    ax.legend([handles[o] for o in legend_order], [labels[o] for o in legend_order], fontsize='small',frameon=False,loc='upper left')

    plt.tight_layout()

    plt.savefig("plots/array_scale.png")

if __name__=="__main__":

    plot_main_results()
    #plot_array_scale()