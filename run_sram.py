

import subprocess
import os
from datetime import datetime

MODEL = "bert_medium"
SENTENCE_LEN = 100

NO_ROWS = 32
NO_COLS = 32
NO_ARRAYS = 128
MEMORY_BW = 10

INTERCONN = "banyan_exp_1"

out_dirs = []
for BANK_SIZE in [1 << i for i in range(10, 15)]:
    OUT_DIR = "experiments/" + datetime.today().strftime('%Y-%m-%d-%H%M%S')
    out_dirs.append(OUT_DIR)
    os.mkdir(OUT_DIR)

    cmd = f'python precompiler/precompile.py \
        --model {MODEL} \
        --sentence_len {SENTENCE_LEN} \
        --out_dir {OUT_DIR}'
    print(cmd)
    subprocess.call(cmd, shell=True)

    cmd = f"./build/compiler_st \
        -r {NO_ROWS} \
        -c {NO_COLS} \
        -N {NO_ARRAYS} \
        -M {MEMORY_BW} \
        -S {BANK_SIZE} \
        -I {INTERCONN} \
        -d {OUT_DIR} &"
    print(cmd)
    subprocess.run(cmd, shell=True)

print("Experiments saved at: {}".format(out_dirs))