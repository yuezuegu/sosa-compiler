import os
import time
from datetime import datetime
import subprocess

def wait_for_proc_limit(running_procs):
    while len(running_procs) > 1000:
        for proc in running_procs:
            retcode = proc.poll()
            if retcode is not None: # Process finished.
                print("Process {} ended with code {}".format(proc.pid, retcode))
                if retcode != 0:
                    print("FAILED: Return code is not 0")
                running_procs.remove(proc)
                break
            else: # No process is done, wait a bit and check again.
                time.sleep(.1)
                continue
    return running_procs

if __name__=="__main__":
    start = time.time()

    exp_dir = "experiments/run-{}".format(datetime.now().strftime("%Y_%m_%d-%H_%M_%S"))
    os.system("mkdir -p {}".format(exp_dir))

    BATCH_SIZE = 1
    IMSIZE = 299
    ARRAY_SIZE = (32, 32)
    SENTENCE_LEN = 100
    MEMORY_BW = 1200
    BANK_SIZE = 1 << 19

    cnn_models = ["inception", "resnet50", "densenet121"]
    bert_models = ["bert_small", "bert_base", "bert_large"]

    no_arrays = [32,64,128,256,512]
    
    out_dirs = []
    running_procs = []

    for INTERCONN in ["banyan_exp_0", "banyan_exp_1", "banyan_exp_2", "banyan_exp_3", "benes_vanilla", "benes_copy", "crossbar"]:
        for N in no_arrays:
            for MODEL in bert_models + cnn_models:
                
                OUT_DIR = exp_dir + "/" + datetime.today().strftime('%Y-%m-%d-%H%M%S')
                out_dirs.append(OUT_DIR)
                os.mkdir(OUT_DIR)

                cmd = f'python precompiler/precompile.py \
                    --model {MODEL} \
                    --batch_size {BATCH_SIZE} \
                    --sentence_len {SENTENCE_LEN} \
                    --imsize {IMSIZE} \
                    --out_dir {OUT_DIR}'
                print(cmd)
                subprocess.run(cmd, shell=True)

                cmd = f"./build-Release/compiler_st \
                    -r {ARRAY_SIZE[0]} \
                    -c {ARRAY_SIZE[1]} \
                    -N {N} \
                    -M {MEMORY_BW} \
                    -S {BANK_SIZE} \
                    -I {INTERCONN} \
                    -d {OUT_DIR}"
                p = subprocess.Popen(cmd, shell=True)
                print("process: {} ".format(p.pid), cmd)

                running_procs.append(p)

                time.sleep(2)

                wait_for_proc_limit(running_procs)
        
    elapsed = time.time() - start
    os.system("echo Script is done in {} hours. Results are here: {}".format(elapsed/60/60, exp_dir))