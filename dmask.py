
import os
import json 
import matplotlib.pyplot as plt

fnames = ['2021-10-12-104028', '2021-10-12-122401', '2021-10-12-140856', '2021-10-12-155410', '2021-10-12-173933', '2021-10-12-192243', '2021-10-12-211637', '2021-10-12-230908', '2021-10-13-010216', '2021-10-13-025804']

acc = {"flops": {0.01: 85.236, 0.1: 85.897, 1:85.337, 10:81.931, 100:79.067}, "systolic": {0.01: 86.098, 0.1:86.478, 1: 85.517, 10:82.031, 100:80.168}}
no_cycles = {"flops": {}, "systolic": {}}
for f in fnames:
    os.system("./main -r 128 -c 128 -N 1 --work_dir dmask/{} &> /dev/null".format(f))

    with open("dmask/{}/results.json".format(f)) as jf:
        
        res = json.load(jf)

        hw_type = res["hw_type"]
        lat_coef = res["lat_coef"]
        no_cycles[hw_type][lat_coef] = res["no_cycles"]

print(no_cycles)

plt.plot(no_cycles["flops"].values(), acc["flops"].values())
plt.plot(no_cycles["systolic"].values(), acc["systolic"].values())
plt.xlim(left=0)
plt.savefig("res.png")