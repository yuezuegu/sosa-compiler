import json
from os import name
import numpy as np
import matplotlib.pyplot as plt
from matplotlib import rc
from matplotlib import cm

from scipy.stats import hmean

freq = 1e9

rc('font',**{'family':'sans-serif','sans-serif':['Helvetica'],'size':18})

with open("atpps_250.json") as json_file:
    out_data = json.load(json_file)
    r_range = out_data["r_range"]
    c_range = out_data["c_range"]
    atpps = out_data["atpps"]

seq_list = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
imsize_list = [224, 256, 299]

ds = []
for model_name in ["bert_medium","bert_base","bert_large"]:
    ds_tmp = []
    for no_seq in seq_list:
        ds_tmp.append(np.array(atpps[model_name][str(no_seq)]))
    ds.append(hmean(ds_tmp))

ds_bert = hmean(ds)

ds = []
for model_name in ["inception","resnet50","resnet101","resnet152","densenet121","densenet169","densenet201"]:
    ds_tmp = []
    for imsize in imsize_list:
        ds_tmp.append(np.array(atpps[model_name][str(imsize)]))
    ds.append(hmean(ds_tmp))

ds_cnn = hmean(ds)

ds_mix = hmean([ds_cnn, ds_bert])

ds_bert = ds_bert / 1e12 * freq #convert ops/cycle/W to Teraops/s/W
ds_cnn = ds_cnn / 1e12 * freq #convert ops/cycle/W to Teraops/s/W
ds_mix = ds_mix / 1e12 * freq #convert ops/cycle/W to Teraops/s/W

baselines = [(128,128), (256,256), (512,512), (8,8)]

names = ["Bert", "CNN", "Mix"]
for ind, ds in enumerate([ds_bert, ds_cnn, ds_mix]):
    i,j=np.unravel_index(np.argmax(ds), ds.shape)
    r_peak = r_range[i]
    c_peak = c_range[j]
    print(names[ind])
    print("Peak at {}x{}".format(r_peak, c_peak))

    T = {}
    for r,c in baselines+[(r_peak,c_peak)]+[(32,32), (68,32), (20,128), (20,32)]:
        i = r_range.index(r) #np.where(r_range == r)[0][0]
        j = c_range.index(c) #np.where(r_range == c)[0][0]
        T[(r,c)] = ds[i,j]
        print("{}x{}: {}".format(r,c,T[(r,c)]))

    plt.figure(figsize=(6.5, 5))
    plt.pcolormesh(r_range, c_range, ds, cmap=cm.get_cmap('coolwarm'), shading='auto')

    cb = plt.colorbar()

    for k in baselines+[(r_peak,c_peak)]:
        plt.plot(k[1], k[0], 'w.')
        cb.ax.plot(0.5, T[k], 'w.')

    plt.ylabel("# of rows")
    plt.xlabel("# of colums")
    plt.xticks([100,200,300,400,500])
    plt.tight_layout()
    plt.savefig("plots/design_space_"+names[ind]+".png")