
import os 
from datetime import datetime
import hashlib

out_dir = "experiments/run-{}".format(datetime.now().strftime("%Y_%m_%d-%H_%M_%S"))
os.system("mkdir -p {}".format(out_dir))

args = {"model": "bert_tiny", "array_size": [32,32]}
arg_str = "_".join(["{}".format(a[1]) for a in list(args.values())])
arg_hash = hashlib.md5(arg_str.encode('utf-8')).hexdigest()

out_dir = out_dir + "/" + str(arg_hash)

os.system("python precompiler/precompile.py --model {} --array_size {} {} --out_dir {} &> log.out".format(
    args["model"], 
    str(args["array_size"][0]), 
    str(args["array_size"][0]), 
    out_dir))