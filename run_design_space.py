
import pickle
import argparse
import json 

import numpy as np

import time 

import matplotlib.pyplot as plt

from precompiler.benchmarks import benchmark, get_benchmarks

e_data = 4.1e-12 #J per byte
e_compute = 0.4e-12 #J per MAC
I_0 = 2.875e-5 #W per byte

freq = 1e9 #Hz

tdp = 250 #Watts

r_range = np.arange(4, 512+1, 4)
c_range = r_range

def no_folds(m,n,k,r,c):
    return np.multiply.outer(np.ceil(m/r) * np.ceil(n/r), np.ceil(k/c))

def calc_atpp(bm):
    layer_dims = bm.layer_dims
    no_layers = len(layer_dims)
    assert no_layers>0, "Empty layer list"
    
    N = find_no_systolic_arrays(freq, e_data, e_compute, I_0, tdp)

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

    p_peak, _, _, _ = calculate_peak_power(N, freq, e_data, e_compute, I_0)
    atpp = bm.no_ops / total_time / p_peak #ops/cycle/W
    return atpp

def calculate_peak_power(N, freq, e_data, e_compute, I_0):
    sram_read = np.add.outer(r_range, 5 * c_range)
    p_data = sram_read * N * e_data * freq
    p_compute = np.multiply.outer(r_range, c_range) * N * e_compute * freq
    p_interconnect = I_0 * N * np.log2(N) * sram_read

    p_total = p_data + p_compute + p_interconnect
    return p_total, p_data, p_compute, p_interconnect

def find_no_systolic_arrays(freq, e_data, e_compute, I_0, tdp):
    no_arrays = np.ones((r_range.shape[0], c_range.shape[0]))

    N = 2
    while True:
        p_peak, _, _, _ = calculate_peak_power(N, freq, e_data, e_compute, I_0)
        
        cond = p_peak <= tdp
        if(not np.any(cond)):
            return no_arrays
        
        no_arrays = np.where(cond, N, no_arrays) 

        N += 1
        #N *= 2

if __name__ == "__main__":
    with open('benchmarks.pickle', 'rb') as pickle_file:
        benchmarks = pickle.load(pickle_file)

    batch_size = 1
    
    atpps = {}
    for model_name in ["bert_medium","bert_base","bert_large"]:
        atpps[model_name] = {}

        for no_seq in [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]:
            start = time.time()

            bm = get_benchmarks(model_name, batch_size, None, no_seq)

            atpp_np = calc_atpp(bm)
            atpps[model_name][no_seq] = atpp_np.tolist()

            elapsed = time.time() - start
            print("Model: {} no_seq: {} \t elapsed: {}".format(model_name, no_seq ,elapsed))

    for model_name in ["inception","resnet50","resnet101","resnet152","densenet121","densenet169","densenet201"]:
        atpps[model_name] = {}

        for imsize in [224, 256, 299]:
            start = time.time()

            bm = get_benchmarks(model_name, batch_size, imsize, None)

            atpp_np = calc_atpp(bm)
            atpps[model_name][imsize] = atpp_np.tolist()

            elapsed = time.time() - start
            print("Model: {} \t elapsed: {}".format(model_name ,elapsed))

    with open("atpps_"+str(tdp)+".json", 'w') as outfile:
        json.dump({"r_range": r_range.tolist(), "c_range": c_range.tolist(), "atpps": atpps},outfile)
