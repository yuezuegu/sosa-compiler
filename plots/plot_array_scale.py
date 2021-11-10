import os
import json
import matplotlib.pyplot as plt
from matplotlib import rc
import pickle 
import scipy.stats
import sys 
sys.path.append('.')

from plot_interconnect import get_result

markers = ["P", "*", "o", "v", "s", "p",  "X", "D", "d", "s", "p", "P"]
lines = [':', '-.', '-','--', '-.', ':','--', '-.', ':','--', '-.', ':']

colors = ["teal","steelblue","tab:red","tab:gray","slategrey","mediumpurple"]


plt.figure(figsize=(7, 5))
rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':16})

e_data = 4.1e-12 #J per byte
e_compute = 0.4e-12 #J per MAC
I_0 = 0.45e-6
freq = 1e9 #Hz

exp_dir = "experiments/run-2021_11_10-17_32"

files = os.listdir(exp_dir)

out_jsons = []
for fname in files:
    try:
        f = open(exp_dir+"/"+fname+"/sim_results.json", "r")
        out_jsons.append(json.load(f))
    except:
        print(fname + "/sim_result.json could not be opened!")

cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
#cnn_models = []
bert_models = ["bert_medium", "bert_base", "bert_large"]
no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
imsize = [299]
batch_size = 1

labels = ["256x256", "128x128", "SOSA (32x32)", "Monolithic"]
array_size_list = [ (32,32), (128,128)]
no_array_list = {
                    (128,128):[16],
                    (32,32):[128]
                }


for ind, array_size in enumerate(array_size_list):
    no_arrays = no_array_list[array_size]

    out = []
    for no_array in no_arrays:
        eff_thru_harm = []
        for m in bert_models:
            eff_thru = []
            for no_seq in no_seqs:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
                no_cycles = get_result("no_cycles", args, out_jsons)
                assert(no_cycles != -1)
                no_ops = get_result("no_ops", args, out_jsons)
                throughput = no_ops / (no_cycles / freq) * 1e-12

                eff_thru.append(throughput)

            eff_thru_harm.append(scipy.stats.hmean(eff_thru))

        for m in cnn_models:
            eff_thru = []
            for im in imsize:
                args = {"array_size":list(array_size), "no_array":no_array, "model":m, "imsize":im}
                no_cycles = get_result("no_cycles", args, out_jsons)
                no_ops = get_result("no_ops", args, out_jsons)
                throughput = no_ops / (no_cycles / freq) * 1e-12

                eff_thru.append(throughput)

            eff_thru_harm.append(scipy.stats.hmean(eff_thru))

        out.append(scipy.stats.hmean(eff_thru_harm))

    peak_powers = {}
    for N in no_arrays:
        args = {"array_size":list(array_size), "no_array":N, "model":m}
        interconn_power = get_result("interconnect_tdp", args, out_jsons)
        switch_width = array_size[0] + 5 * array_size[1]
        p_data = N * switch_width * e_data * freq
        p_compute = N * array_size[0] * array_size[1] * e_compute * freq
        peak_powers[N] = p_data + p_compute + interconn_power

    #peak_powers = [calculate_peak_power(array_size[0], array_size[1], N, freq, e_data, e_compute, I_0)[0] for N in no_arrays]

    for i, N in enumerate(no_arrays):
        print("{}x{} - {}:\t T:{:.4}\t P:{:.4}".format(array_size[0], array_size[1], N, out[i], peak_powers[N]))

    plt.plot(peak_powers.values(), out, label=labels[ind],
                linewidth=3, linestyle=lines[ind], marker=markers[ind], markersize=10, color=colors[ind])

# array_size = [8,8]
# ind = len(array_size_list)
# out = []
# peak_powers = []
# for N in [200, 400, 800, 1200, 1600, 2000]:
#     T = calculate_peak_throughput(array_size[0], array_size[1],N,freq)
#     out.append(T)

#     peak_powers.append(calculate_peak_power(array_size[0], array_size[1], N, freq, e_data, e_compute, I_0)[0])

# plt.plot(peak_powers, out, label=labels[ind],
#             linewidth=3, linestyle=lines[ind], marker=markers[ind], markersize=10, color=colors[ind])


# ind += 1 
# out = []
# peak_powers = []
# for array_size in [(400,400), (512,512), (800,800), (960,960), (1024,1024)]:
#     no_array = 1

#     eff_thru_harm = []

#     for m in bert_models:
#         eff_thru = []
#         for no_seq in no_seqs:
#             args = {"array_size":list(array_size), "no_array":no_array, "model":m, "sentence_len":no_seq}
#             no_cycles = get_result("no_cycles", args, out_jsons)

#             eff_thru.append(no_ops[m][no_seq] / (no_cycles / freq))

#         eff_thru_harm.append(scipy.stats.hmean(eff_thru))

#     for m in cnn_models:
#         eff_thru = []
#         for im in imsize:
#             args = {"array_size":list(array_size), "no_array":no_array, "model":m, "imsize":im}
#             no_cycles = get_result("no_cycles", args, out_jsons)

#             eff_thru.append(no_ops[m][im] / (no_cycles / freq))

#         eff_thru_harm.append(scipy.stats.hmean(eff_thru))

#     out.append(scipy.stats.hmean(eff_thru_harm))

#     peak_powers.append(calculate_peak_power(array_size[0], array_size[1], 1, freq, e_data, e_compute, I_0)[0])

# plt.plot(peak_powers, out, label=labels[ind],
#             linewidth=3, linestyle=lines[ind], marker=markers[ind], markersize=10, color=colors[ind])


plt.ylabel('Effective Throughput (TeraOps/s)')
plt.xlabel('TDP (Watts)')
#plt.xlim(right=750)
#plt.ylim(bottom=0, top=420)

ax = plt.gca()
handles, labels = ax.get_legend_handles_labels()

legend_order = [2,1,0,3]

#ax.legend([handles[o] for o in legend_order], [labels[o] for o in legend_order], fontsize='small',frameon=False)

plt.tight_layout()

plt.savefig("plots/array_scale.png")