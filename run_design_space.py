
# import sys 
# sys.path.append('.')

import pickle
import argparse
import json
from matplotlib.markers import MarkerStyle 

import numpy as np

import time 

import matplotlib.pyplot as plt
from matplotlib import rc
from matplotlib import cm

from scipy.stats import hmean
from precompiler.benchmarks import benchmark, get_benchmarks

from helpers import calculate_peak_power

#e_data = 4.1e-12 #J per byte (512kB)
e_data = 2.7e-12 #J per byte (256kB)
#e_data = 1.6e-12 #J per byte (128kB)
#e_data = 1.1e-12 #J per byte (64kB)

e_compute = 0.4e-12 #J per MAC
I_0 = 2.875e-5 #W per byte
#I_0 = 0

freq = 1e9 #Hz

#tdp = 250 #Watts
tdp = 400

def no_folds(m,n,k,r,c):
    return np.multiply.outer(np.ceil(m/r) * np.ceil(n/r), np.ceil(k/c))

def calc_atpp(bm, r_range, c_range):
    layer_dims = bm.layer_dims
    no_layers = len(layer_dims)
    assert no_layers>0, "Empty layer list"
    
    N = find_no_systolic_arrays(r_range, c_range, freq, e_data, e_compute, I_0, tdp)

    total_time = np.zeros((r_range.shape[0],c_range.shape[0]))
    for layer_name in layer_dims:
        X = layer_dims[layer_name][0]
        W = layer_dims[layer_name][1]
        m = X[0]
        n = X[1]
        k = W[1]

        w_stall = np.broadcast_to(r_range.reshape(-1,1), (r_range.shape[0],c_range.shape[0]))
        folds = no_folds(m,n,k,r_range,c_range)
        total_time += np.multiply(np.ceil( np.divide(folds, N) ), w_stall)

    p_peak, _, _, _ = calculate_peak_power(r_range, c_range, N, freq, e_data, e_compute, I_0)
    atpp = bm.no_ops / total_time / p_peak #ops/cycle/W
    return atpp



def find_no_systolic_arrays(r_range, c_range, freq, e_data, e_compute, I_0, tdp):
    no_arrays = np.ones((r_range.shape[0], c_range.shape[0]))

    N = 2
    while True:
        p_peak, _, _, _ = calculate_peak_power(r_range, c_range, N, freq, e_data, e_compute, I_0)
        
        cond = p_peak <= tdp
        if(not np.any(cond)):
            return no_arrays
        
        no_arrays = np.where(cond, N, no_arrays) 

        N += 1
        #N *= 2


bert_models = ["bert_medium","bert_base","bert_large"]
cnn_models = ["inception","resnet50","resnet101","resnet152","densenet121","densenet169","densenet201"]
seq_list = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
imsize_list = [224,256,299]
batch_size = 1

r_range = np.arange(1, 256+1, 1)
c_range = r_range

def plot_design_space(atpps):
    rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':18})

    ds = []
    for model_name in bert_models:
        ds_tmp = []
        for no_seq in seq_list:
            ds_tmp.append(np.array(atpps[model_name][str(no_seq)]))
        ds.append(hmean(ds_tmp))

    ds_bert = hmean(ds)

    ds = []
    for model_name in cnn_models:
        ds_tmp = []
        for imsize in imsize_list:
            ds_tmp.append(np.array(atpps[model_name][str(imsize)]))
        ds.append(hmean(ds_tmp))

    ds_cnn = hmean(ds)

    ds_mix = hmean([ds_cnn, ds_bert])

    ds_bert = ds_bert / 1e12 * freq #convert ops/cycle/W to Teraops/s/W
    ds_cnn = ds_cnn / 1e12 * freq #convert ops/cycle/W to Teraops/s/W
    ds_mix = ds_mix / 1e12 * freq #convert ops/cycle/W to Teraops/s/W

    baselines = [(128,128), (256,256), (32,32), (8,8)]

    names = ["Bert", "CNN", "Mix"]

    for ind, ds in enumerate([ds_bert, ds_cnn, ds_mix]):
        i,j=np.unravel_index(np.argmax(ds), ds.shape)
        r_peak = r_range[i]
        c_peak = c_range[j]
        print("Peak at {}x{}".format(r_peak, c_peak))

        T = {}
        for r,c in baselines+[(r_peak,c_peak)]+[(16,16), (32,32), (64,64), ]:
            i = np.where(r_range == r)[0][0]
            j = np.where(r_range == c)[0][0]
            T[(r,c)] = ds[i,j]
            print("{}x{}: {}".format(r,c,T[(r,c)]))

        plt.figure(figsize=(6.5, 5))
        plt.pcolormesh(r_range, c_range, ds, cmap=cm.get_cmap('coolwarm'), shading='auto')

        cb = plt.colorbar()

        points = []
        markers = ['D','o','s','p','*',]
        labels = ["128x128","256x256","32x32","8x8"]
        for i, k in enumerate(baselines):
            p = plt.scatter(k[1], k[0], s=100, marker=markers[i], color='white', edgecolors='black', label=labels[i])
            points.append(p)
            cb.ax.plot(0.7, T[k], marker=markers[i], markersize=8, color='white', markeredgecolor='black')

        plt.legend(handles=[points[i] for i in [3,2,0,1]], fontsize='small',frameon=False, loc='lower center', bbox_to_anchor=(0.5, 1.), ncol=2)

        plt.ylabel("# of rows")
        plt.xlabel("# of colums")
        plt.xticks([64,128,192,256])
        plt.yticks([64,128,192,256])
        plt.tight_layout()
        plt.savefig("plots/design_space"+names[ind]+".png")
        plt.savefig("plots/design_space"+names[ind]+".pdf")

def run_design_space():    
    with open('benchmarks.pickle', 'rb') as pickle_file:
        benchmarks = pickle.load(pickle_file)

    atpps = {}
    for model_name in bert_models:
        atpps[model_name] = {}

        for no_seq in seq_list:
            start = time.time()

            bm = get_benchmarks(model_name, batch_size, None, no_seq, read_only=1)

            atpp_np = calc_atpp(bm, r_range, c_range)
            atpps[model_name][no_seq] = atpp_np.tolist()

            elapsed = time.time() - start
            print("Model: {} no_seq: {} \t elapsed: {}".format(model_name, no_seq ,elapsed))

    for model_name in cnn_models:
        atpps[model_name] = {}

        for imsize in imsize_list:
            start = time.time()

            bm = get_benchmarks(model_name, batch_size, imsize, None, read_only=1)

            atpp_np = calc_atpp(bm, r_range, c_range)
            atpps[model_name][imsize] = atpp_np.tolist()

            elapsed = time.time() - start
            print("Model: {} \t elapsed: {}".format(model_name ,elapsed))

    with open("experiments/atpps.json", "w") as json_file:
        json.dump(atpps, json_file)

if __name__ == "__main__":
    run_design_space()

    with open("experiments/atpps.json", "r") as json_file:
        atpps=json.load(json_file)

    plot_design_space(atpps)