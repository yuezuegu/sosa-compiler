import os
from collections import OrderedDict

os.environ['OPENBLAS_NUM_THREADS'] = '1'
os.environ['OMP_NUM_THREADS'] = '1'
os.environ['MKL_NUM_THREADS'] = '1'
os.environ['TF_XLA_FLAGS'] = '--tf_xla_enable_xla_devices'

import tensorflow as tf

tf.config.threading.set_inter_op_parallelism_threads(1)
tf.config.threading.set_intra_op_parallelism_threads(1)

import numpy as np
np.random.seed(0)

import sys 
sys.path.append('./precompiler')

import graph
from graph import convert_keras_to_graph

from benchmarks import benchmark, get_benchmarks

import argparse

import logging
logging.basicConfig(filename='logger.out', filemode='w', level=logging.INFO)

import json 

def split_mat(x_size, w_size, array_size, tile_first_dim=None):
    no_row, no_col = array_size

    if tile_first_dim is None:
        partition_size = (no_row, no_row, no_col)
    else:
        partition_size = (tile_first_dim, no_row, no_col)


    dim1 = int(x_size[0])
    dim2 = int(w_size[0])
    dim3 = int(w_size[1])

    no_row_tile = int( np.ceil (dim2 / partition_size[1]) )
    no_col_tile = int( np.ceil (dim3 / partition_size[2]) )

    w_tile_dim = {i:{} for i in range(no_row_tile)}
    for j in range(no_col_tile):
        for i in range(no_row_tile):
            tile_dim1 = min((i+1)*partition_size[1], dim2) - i*partition_size[1]
            tile_dim2 = min((j+1)*partition_size[2], dim3) - j*partition_size[2]
            w_tile_dim[i][j] = [tile_dim1,tile_dim2]

    
    no_batch_tile = int( np.ceil (dim1 / partition_size[0]) )
    x_tile_dim = {i:{} for i in range(no_batch_tile)}

    for j in range(no_row_tile):
        for i in range(no_batch_tile):
            tile_dim1 = min((i+1)*partition_size[0], dim1) - i*partition_size[0]
            tile_dim2 = min((j+1)*partition_size[1], dim2) - j*partition_size[1]
            x_tile_dim[i][j] = [tile_dim1, tile_dim2]

    return x_tile_dim, w_tile_dim, no_batch_tile, no_row_tile, no_col_tile


def partition_layer(layer_node, array_size, partition_size):
    if layer_node.layer_type == 'Conv2D' or layer_node.layer_type == 'Dense':
        gemm_info = {}

        input_size = layer_node.layer_attr["input_size"]
        weight_size = layer_node.layer_attr["weight_size"]

        if layer_node.layer_type == 'Conv2D':
            gemm_info["kernel_size"] = layer_node.layer_attr["kernel_size"]

        gemm_info["input_size"] = input_size
        gemm_info["weight_size"] = weight_size

        x_tile_dim, w_tile_dim, no_batch_tile, no_row_tile, no_col_tile = split_mat(input_size, weight_size, array_size, partition_size)
        no_tiles = (no_batch_tile, no_row_tile, no_col_tile)

        gemm_info["x_tile_dim"] = x_tile_dim
        gemm_info["w_tile_dim"] = w_tile_dim
        gemm_info["no_tiles"] = no_tiles
        

        return gemm_info
    else:
        logging.info("No GEMM operation found at layer {} of type {}.".format(layer_node.layer_name, layer_node.layer_type))
        return None

def all_dependencies_scheduled(layer_name, graph, layer_schedules):
    node = graph.get_node(layer_name)

    for s in node.src:
        if s.layer_name not in layer_schedules:
            return False

    return True

def precompile_model(model, array_size, partition_size=None):
    graph = convert_keras_to_graph(model)

    raw_input = 1

    layers = OrderedDict()
    for layer_name in graph.get_layer_names():
        layer_node = graph.get_node(layer_name)
        gemm_op = partition_layer(layer_node, array_size, partition_size)

        dependencies = [s.layer_name for s in layer_node.src]
        
        layers[layer_name] = {"gemm_op": gemm_op, "deps": dependencies, "raw_input":raw_input, "layer_type": layer_node.layer_type}

        if gemm_op is not None: 
            raw_input = 0 #toggle raw_input after the first layer with gemm ops

    return layers

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', type=str, required=False, default='bert_medium')
    parser.add_argument('--extra_models', type=str, nargs='+', required=False, default=[])
    parser.add_argument('--batch_size', type=int, required=False, default=1, help='Batch size')
    parser.add_argument('--sentence_len', type=int, required=False, default=100, help='Sentence length for transformer model')
    parser.add_argument('--imsize', type=int, required=False, default=299)
    parser.add_argument('--array_size', type=int, nargs='+', required=False, default=[32,32], help='Array size')
    parser.add_argument('--out_dir', type=str, required=False, default="experiments/tmp")
    parser.add_argument('--partition_size', type=int, required=False, default=None)
    parser.add_argument('--read_only', type=int, required=False, default=1)
    parser.add_argument('--enable_schedule_duplication', type=int, required=False, default=1)

    args = parser.parse_args()

    batch_size = args.batch_size
    sentence_len = args.sentence_len
    
    array_size = args.array_size

    imsize = args.imsize
    partition_size = args.partition_size
    out_dir = args.out_dir
    extra_models = args.extra_models
    extra_models = [m for m in extra_models if m != "None"]
 
    if len(extra_models) > 0:
        assert args.enable_schedule_duplication == False, "Schedule duplication is supported only for a single BERT model"

    json_out = {"args":args.__dict__}
    for m in [args.model] + extra_models:
        bm = get_benchmarks(m, batch_size, imsize, sentence_len, args.read_only)
        if bm.model_type == "BERT" and args.enable_schedule_duplication:
            keras_model = bm.get_keras_model(no_layers=1)
            no_repeat = bm.no_layers
        else:
            keras_model = bm.get_keras_model() 
            no_repeat = 1

        layers = precompile_model(keras_model, array_size=array_size, partition_size=partition_size)
        json_out[m] = {"order":list(layers.keys()), "layers":layers, "no_repeat":no_repeat, "no_ops":bm.no_ops}
    
    os.makedirs(out_dir, exist_ok=True)
    with open(out_dir+"/precompiled_model.json", "w") as outfile:  
        json.dump(json_out, outfile)

    print("precompiled model is saved at: {}".format(out_dir))

if __name__ == "__main__":
    main()