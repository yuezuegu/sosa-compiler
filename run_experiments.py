import os
import time
from datetime import datetime
import subprocess
import itertools 
import numpy as np

NO_PROCS = 32
running_procs = []

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

def memory_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)
    NO_ARRAY = 128

    BANK_SIZE = 262144

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    cnn_models = list(itertools.product(cnn_models, [1]))

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [100]
    bert_models = list(itertools.product(bert_models, no_seqs))

    INTERCONN = "banyan_exp_1"

    #for MEMORY_BW in [400, 800, 1200, 1600, 2000]:
    for MEMORY_BW in [1200]:
        for MODEL,SENTENCE_LEN in cnn_models + bert_models:    
                OUT_DIR = exp_dir + "/" + date_millisecond()

                os.mkdir(OUT_DIR)

                cmd1 = f'python precompiler/precompile.py \
                    --model {MODEL} \
                    --batch_size {BATCH_SIZE} \
                    --sentence_len {SENTENCE_LEN} \
                    --imsize {IMSIZE} \
                    --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
                    --out_dir {OUT_DIR}' \
                    
                cmd2 = f"./build-Release/compiler_st \
                    -r {ARRAY_SIZE[0]} \
                    -c {ARRAY_SIZE[1]} \
                    -N {NO_ARRAY} \
                    -M {MEMORY_BW} \
                    -S {BANK_SIZE} \
                    -I {INTERCONN} \
                    -d {OUT_DIR}"
                
                cmd = cmd1 + " && " + cmd2
                p = subprocess.Popen(cmd, shell=True)
                print("process: {} ".format(p.pid), cmd)

                running_procs.append(p)

                time.sleep(.1)
                wait_for_proc_limit(running_procs)

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)
            


def interconnect_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)

    MEMORY_BW = 1200
    BANK_SIZE = 262144

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    cnn_models = list(itertools.product(cnn_models, [1]))

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
    bert_models = list(itertools.product(bert_models, no_seqs))

    #no_arrays = [32,64,128,256,512]
    no_arrays = [32,64,128]

    #for INTERCONN in ["banyan_exp_0", "banyan_exp_1", "banyan_exp_2", "banyan_exp_3", "benes_copy", "crossbar"]:
    for INTERCONN in ["bus"]:
        for no_array in no_arrays:
            for MODEL,SENTENCE_LEN in bert_models + cnn_models:
                OUT_DIR = exp_dir + "/" + date_millisecond()
                os.mkdir(OUT_DIR)

                cmd1 = f'python precompiler/precompile.py \
                    --model {MODEL} \
                    --batch_size {BATCH_SIZE} \
                    --sentence_len {SENTENCE_LEN} \
                    --imsize {IMSIZE} \
                    --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
                    --out_dir {OUT_DIR}'

                cmd2 = f"./build-Release/compiler_st \
                    -r {ARRAY_SIZE[0]} \
                    -c {ARRAY_SIZE[1]} \
                    -N {no_array} \
                    -M {MEMORY_BW} \
                    -S {BANK_SIZE} \
                    -I {INTERCONN} \
                    -d {OUT_DIR}"

                cmd = cmd1 + " && " + cmd2
                p = subprocess.Popen(cmd, shell=True)
                print("process: {} ".format(p.pid), cmd)

                running_procs.append(p)

                time.sleep(.1)
                wait_for_proc_limit(running_procs)

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)

def bank_size_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))
    IMSIZE = 299

    INTERCONN = "banyan_exp_1"

    ARRAY_SIZE = (32, 32)
    NO_ARRAY = 256

    cnn_models = ["resnet152"]

    MEMORY_BW = 1200

    for BATCH_SIZE in [1, 2, 4, 8]:
        for BANK_SIZE in [32768, 65536, 131072, 262144, 524288, 1048576]:
            for MODEL in cnn_models:
                OUT_DIR = exp_dir + "/" + date_millisecond()

                os.mkdir(OUT_DIR)

                cmd1 = f'python precompiler/precompile.py \
                    --model {MODEL} \
                    --batch_size {BATCH_SIZE} \
                    --imsize {IMSIZE} \
                    --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
                    --out_dir {OUT_DIR}' \
                    
                cmd2 = f"./build-Release/compiler_st \
                    -r {ARRAY_SIZE[0]} \
                    -c {ARRAY_SIZE[1]} \
                    -N {NO_ARRAY} \
                    -M {MEMORY_BW} \
                    -S {BANK_SIZE} \
                    -I {INTERCONN} \
                    -d {OUT_DIR}"
                
                cmd = cmd1 + " && " + cmd2
                p = subprocess.Popen(cmd, shell=True)
                print("process: {} ".format(p.pid), cmd)

                running_procs.append(p)

                time.sleep(.1)
                wait_for_proc_limit(running_procs)

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)

def multi_batch_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))

    IMSIZE = 299

    MEMORY_BW = 1200
    BANK_SIZE = 262144    

    INTERCONN = "banyan_exp_1"

    ARRAY_SIZE = (32, 32)
    NO_ARRAY = 256

    #MODELS = ["densenet121", "bert_medium", "densenet121"]
    MODELS = ["resnet152", "bert_medium", "resnet152"]
    EXTRA_MODELS = ["None", "None", "bert_medium"]

    SENTENCE_LEN = 100

    for BATCH_SIZE in [1,2,4,8]:
        for ind, MODEL in enumerate(MODELS):
            OUT_DIR = exp_dir + "/" + date_millisecond()
            os.mkdir(OUT_DIR)

            cmd1 = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
                --extra_models {EXTRA_MODELS[ind]} \
                --enable_schedule_duplication 0 \
                --out_dir {OUT_DIR}' \
                
            cmd2 = f"./build-Release/compiler_st \
                -r {ARRAY_SIZE[0]} \
                -c {ARRAY_SIZE[1]} \
                -N {NO_ARRAY} \
                -M {MEMORY_BW} \
                -S {BANK_SIZE} \
                -I {INTERCONN} \
                -d {OUT_DIR}"
            
            cmd = cmd1 + " && " + cmd2
            p = subprocess.Popen(cmd, shell=True)
            print("process: {} ".format(p.pid), cmd)

            running_procs.append(p)

            time.sleep(.1)
            wait_for_proc_limit(running_procs)

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)

def array_scale_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 4
    IMSIZE = 299

    MEMORY_BW = 1200
    BANK_SIZE = 262144

    cnn_models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    cnn_models = list(itertools.product(cnn_models, [1]))

    bert_models = ["bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
    no_seqs = [10, 20, 40, 60, 80, 100, 200, 300, 400, 500]
    bert_models = list(itertools.product(bert_models, no_seqs))

    INTERCONN = "banyan_exp_1"

    array_size_list = []
    
    array_size_list += [(20,32,N) for N in [128,256]]
    #array_size_list += [(32,32,N) for N in [8,16,32,64,128,256,512]]

    #array_size_list += [(16,16,N) for N in [128,256,512,1024]]
    array_size_list += [(32,32,N) for N in [128,256]]
    array_size_list += [(64,64,N) for N in [32,64,128]]
    
    #array_size_list += [(128,128,N) for N in [1, 2, 4, 8, 16, 24, 32, 48, 64]]
    #array_size_list += [(128,128,N) for N in [2,4,8,16,24,32,48,64]]
    #array_size_list += [(256,256,N) for N in [1, 2, 4, 8, 12, 16]]
    #array_size_list += [(400,400,1), (512,512,1), (800,800,1), (960,960,1), (1024,1024,1)]
    #array_size_list += [(1024,1024,1)]

    for r, c, N in array_size_list:
        for MODEL,SENTENCE_LEN in bert_models + cnn_models:
            OUT_DIR = exp_dir + "/" + date_millisecond()
            os.mkdir(OUT_DIR)

            bank_size_norm = BANK_SIZE*int(np.ceil(128/N))

            cmd1 = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --array_size {r} {c} \
                --out_dir {OUT_DIR}'

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

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)


def partition_size_experiment():
    start = time.time()

    exp_dir = "experiments/run-{}".format(date_second())
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)

    MEMORY_BW = 1200
    BANK_SIZE = 262144

    PARTITION_SIZES = [8, 16, 32, 48, 64, 128, 256, 512, 1024]

    CNN_MODELS = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169", "densenet201"]
    CNN_MODELS = list(itertools.product(CNN_MODELS, [1]))

    BERT_MODELS = ["bert_medium", "bert_base", "bert_large"]
    NO_SEQS = [100]
    BERT_MODELS = list(itertools.product(BERT_MODELS, NO_SEQS))

    NO_ARRAY = 256
    INTERCONN = "banyan_exp_1"

    for PARTITION_SIZE in PARTITION_SIZES:
        for MODEL,SENTENCE_LEN in BERT_MODELS + CNN_MODELS:
            OUT_DIR = exp_dir + "/" + date_millisecond()
            os.mkdir(OUT_DIR)

            cmd1 = f'python precompiler/precompile.py \
                --model {MODEL} \
                --batch_size {BATCH_SIZE} \
                --sentence_len {SENTENCE_LEN} \
                --imsize {IMSIZE} \
                --array_size {ARRAY_SIZE[0]} {ARRAY_SIZE[1]} \
                --partition_size {PARTITION_SIZE} \
                --out_dir {OUT_DIR}'

            cmd2 = f"./build-Release/compiler_st \
                -r {ARRAY_SIZE[0]} \
                -c {ARRAY_SIZE[1]} \
                -N {NO_ARRAY} \
                -M {MEMORY_BW} \
                -S {BANK_SIZE} \
                -I {INTERCONN} \
                -d {OUT_DIR}"

            cmd = cmd1 + " && " + cmd2
            p = subprocess.Popen(cmd, shell=True)
            print("process: {} ".format(p.pid), cmd)

            running_procs.append(p)

            time.sleep(.1)

            wait_for_proc_limit(running_procs)

    elapsed = time.time() - start
    print(f"All processes for {__name__} have been started after {elapsed/60/60} hours, results are in {exp_dir}")
    time.sleep(1)

if __name__=="__main__":
    start = time.time()

    #memory_experiment()
    interconnect_experiment()
    #partition_size_experiment()
    #array_scale_experiment()
    #bank_size_experiment()
    #multi_batch_experiment()
    
    wait_all_finish(running_procs)

    elapsed = time.time() - start
    print("All experiments are completed in {} hours.".format(elapsed/60/60))