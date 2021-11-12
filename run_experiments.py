import os
import time
from datetime import datetime
import subprocess
import itertools 
import numpy as np

NO_PROCS = 64

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
                
        if len(running_procs) < NO_PROCS: #Block if there is more than x number of running threads
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


def array_scale_experiment(exp_dir):
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299

    MEMORY_BW = 1200
    BANK_SIZE = 524288

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    cnn_models = list(itertools.product(cnn_models, [1]))

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
    bert_models = list(itertools.product(bert_models, no_seqs))

    INTERCONN = "banyan_exp_1"

    out_dirs = []
    running_procs = []

    array_size_list = []
    array_size_list += [(16,16,N) for N in [64,128,256,512]]
    # array_size_list += [(32,32,N) for N in [16,32,64,128,256,512]]
    # array_size_list += [(128,128,N) for N in [1, 2, 4, 8, 16, 24, 32, 48, 64]]
    # array_size_list += [(256,256,N) for N in [1, 2, 4, 8, 12, 16]]
    # array_size_list += [(400,400,1), (512,512,1), (800,800,1), (960,960,1), (1024,1024,1)]

    for r, c, N in array_size_list:
        for MODEL,SENTENCE_LEN in bert_models + cnn_models:
            
            OUT_DIR = exp_dir + "/" + date_millisecond()
            out_dirs.append(OUT_DIR)
            os.mkdir(OUT_DIR)

            bank_size_norm = BANK_SIZE*int(np.ceil(128/N))

            cmd1 = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --array_size {r} {c} \
                --out_dir {OUT_DIR}'

            #print(cmd)
            #subprocess.run(cmd, shell=True)

            cmd2 = f"./build-Release/compiler_st \
                -r {r} \
                -c {c} \
                -N {N} \
                -M {MEMORY_BW} \
                -S {bank_size_norm} \
                -I {INTERCONN} \
                -d {OUT_DIR}"
            
            cmd = cmd1 + " && " + cmd2
            p = subprocess.Popen(cmd, shell=True)
            print("process: {} ".format(p.pid), cmd)

            running_procs.append(p)

            time.sleep(.1)

            wait_for_proc_limit(running_procs)

    wait_all_finish(running_procs)

    elapsed = time.time() - start
    os.system("echo Script is done in {} hours. Results are here: {}".format(elapsed/60/60, exp_dir))    



def partition_size_experiment(exp_dir):
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)

    MEMORY_BW = 1200
    BANK_SIZE = 1 << 19

    PARTITION_SIZES = [8, 16, 32, 48, 64, 128, 256, 512, 1024, 10000]

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
            OUT_DIR = exp_dir + "/" + date_millisecond()
            out_dirs.append(OUT_DIR)
            os.mkdir(OUT_DIR)

            cmd = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
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

    #partition_size_experiment()

    array_scale_experiment(exp_dir)

    elapsed = time.time() - start
    os.system("echo Script is done in {} hours.".format(elapsed/60/60))