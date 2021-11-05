import os
import time
from datetime import datetime
import subprocess
import itertools 
import numpy as np

def is_proc_ended(proc):
    retcode = proc.poll()
    if retcode is not None: # Process finished.
        print("Process {} ended with code {}".format(proc.pid, retcode))
        if retcode != 0:
            print("FAILED: Return code is not 0")
        return True
    else:
        return False

def wait_for_proc_limit(running_procs):
    while True:
        for proc in running_procs:
            if (is_proc_ended(proc)):
                running_procs.remove(proc)
                
        if len(running_procs) < 32: #Block if there is more than x number of running threads
            return running_procs

        time.sleep(.1)

def wait_all_finish(running_procs):
    while True:
        for proc in running_procs:
            if (is_proc_ended(proc)):
                running_procs.remove(proc)
                
        if len(running_procs) == 0:
            return running_procs

        time.sleep(.1)

def date_minute():
    return datetime.now().strftime("%Y_%m_%d-%H_%M")

def date_second():
    return datetime.now().strftime("%Y_%m_%d-%H_%M_%S")

def date_millisecond():
    return datetime.now().strftime("%Y_%m_%d-%H_%M_%S_%f")[:-3]



def partition_size_experiment(exp_dir):
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)

    MEMORY_BW = 1200
    BANK_SIZE = 1 << 19

    PARTITION_SIZES = [8, 16, 32, 48, 64, 128, 256, 512, 1024, 100000000]

    CNN_MODELS = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    CNN_MODELS = list(itertools.product(CNN_MODELS, [1]))

    BERT_MODELS = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    NO_SEQS = [100]
    BERT_MODELS = list(itertools.product(BERT_MODELS, NO_SEQS))

    NO_ARRAY = 128
    INTERCONN = "banyan_exp_1"

    out_dirs = []
    running_procs = []

    for PARTITION_SIZE in PARTITION_SIZES:
        for MODEL,SENTENCE_LEN in BERT_MODELS + CNN_MODELS:
            OUT_DIR = exp_dir + "/" + date_millisecond
            out_dirs.append(OUT_DIR)
            os.mkdir(OUT_DIR)

            cmd = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --partition_size {PARTITION_SIZE} \
                --out_dir {OUT_DIR}'
            print(cmd)
            subprocess.run(cmd, shell=True)

            cmd = f"./build-Release/compiler_st \
                -r {ARRAY_SIZE[0]} \
                -c {ARRAY_SIZE[1]} \
                -N {NO_ARRAY} \
                -M {MEMORY_BW} \
                -S {BANK_SIZE} \
                -I {INTERCONN} \
                -d {OUT_DIR}"
            p = subprocess.Popen(cmd, shell=True)
            print("process: {} ".format(p.pid), cmd)

            running_procs.append(p)

            time.sleep(.1)

            wait_for_proc_limit(running_procs)

    wait_all_finish(running_procs)
    
if __name__=="__main__":
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_minute())
    partition_size_experiment()

    elapsed = time.time() - start
    os.system("echo Script is done in {} hours.".format(elapsed/60/60))