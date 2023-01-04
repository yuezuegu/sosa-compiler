import pickle
import numpy as np
import matplotlib.pyplot as plt 
from matplotlib import rc

from run_design_space import no_folds, e_data, e_compute, I_0
from helpers import calculate_peak_power, calculate_peak_throughput

from precompiler.benchmarks import benchmark, get_benchmarks


with open('benchmarks.pickle', 'rb') as pickle_file:
    benchmarks = pickle.load(pickle_file)

util = {}
power = {}
for cnn_model in ["inception"]:

    bm = get_benchmarks(cnn_model, batch_size=16, image_size=224, seq_len=None, read_only=1)

    util[cnn_model] = {}
    for no_rows in [8, 16, 32, 64, 128, 256]:
    #for no_rows in range(4, 256, 4):
        N = 1
        freq = 1e9

        r_range = np.array([no_rows])
        c_range = np.array([no_rows])

        total_time = np.zeros((r_range.shape[0],c_range.shape[0]))
        for layer_name in bm.layer_dims:
            X = bm.layer_dims[layer_name][0]
            W = bm.layer_dims[layer_name][1]
            m = X[0]
            n = X[1]
            k = W[1]

            w_stall = np.broadcast_to(r_range.reshape(-1,1), (r_range.shape[0],c_range.shape[0]))
            folds = no_folds(m,n,k,r_range,c_range)
            total_time += np.multiply(np.ceil( np.divide(folds, N) ), w_stall)

        T_peak = calculate_peak_throughput(r_range,c_range,N,freq)
        ops_per_cycle = bm.no_ops / total_time
        ops_per_sec = ops_per_cycle * freq

        u = 100 * ops_per_sec / 1e12 / T_peak
        util[cnn_model][(no_rows, no_rows)] = u.item()

        p_total, p_data, p_compute, p_interconnect = calculate_peak_power(r_range, c_range, N, freq, e_data, e_compute, I_0)
        power[(no_rows, no_rows)] = {"total": (2*no_rows*no_rows)*freq/1e12/p_total[0,0], "data": (2*no_rows*no_rows)*freq/1e12/p_data[0,0], "compute": (2*no_rows*no_rows)*freq/1e12/p_compute[0,0]}

    print(util[cnn_model])

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':22})

plt.subplots(figsize=(12, 7))

ax1 = plt.gca()
lns1 = ax1.plot([k[0]*k[1] for k in power], [power[k]["total"] for k in power], '*--', linewidth=4, markersize=10, color="tab:red", label="Power eff.")
# plt.plot([k[0]*k[1] for k in power], [power[k]["data"] for k in power], 'd-.', label='Data access', linewidth=3, markersize=10)
# plt.plot([k[0]*k[1] for k in power], [power[k]["compute"] for k in power], 'o-', label='Total', linewidth=3, markersize=10)

plt.ylabel("Power Efficiency (TeraOps/s/Watt)")
plt.xlabel('Array size')
plt.ylim(bottom=0)

ax2 = ax1.twinx()
plt.sca(ax2)

lns2 = ax2.plot([k[0]*k[1] for k in util["inception"]], [util["inception"][k] for k in util["inception"]], 'o-', linewidth=4, markersize=10, color="tab:blue", label="Util.")

plt.xscale('log')

plt.ylabel('Utilization (%)')

# plt.xlim(left=64)
plt.ylim(bottom=0)
plt.xticks([8*8, 16*16, 32*32, 64*64, 128*128, 256*256],['8x8', '16x16', '32x32', '64x64', '128x128','256x256'])

lns = lns1+lns2
labs = [l.get_label() for l in lns]
plt.legend(lns, labs, loc='center left', frameon=False)



plt.tight_layout()
plt.savefig('power_tradeoff.png')
plt.savefig('power_tradeoff.pdf')