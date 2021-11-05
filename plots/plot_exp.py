from dataclasses import dataclass
from common import from_dict, named_zip, filter_key
from defs import IctType
import json
import os
from typing import Dict, List, Tuple
from functools import reduce
import pickle
import scipy.stats
import scipy
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.font_manager
from matplotlib import rc
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from matplotlib.transforms import Bbox
import textwrap

# see please:
# https://stackoverflow.com/questions/44970010/axes-class-set-explicitly-size-width-height-of-axes-in-given-units

def init_matplotlib():
    # Global font parameters
    rc("font", **{ "family": "sans-serif", "sans-serif": [ "Helvetica" ], "size": 16 })

    # Save figure parameters
    # rc("savefig", **{ "pad_inches": 0.5, "bbox": "tight", "dpi": 200 })

# Defines the plot styles
plot_styles = named_zip(
    color=[
        "pink",
        "tab:red",
        "lightcoral",
        "mistyrose",
        "lightsteelblue",
        "lightskyblue",
        "gray",
        "steelblue",
        "tab:cyan",
        "tab:blue",
        "tab:orange",
        "tab:olive"],
    marker=["P", "o", "d", "v", "s", "p",  "X", "D", "d", "s", "p", "P"],
    linestyle=['--', '-', '-.', ':', '-.', ':','--', '-.', ':','--', '-.', ':'],
    linewidth=[3] * 12,
    markersize=[10] * 12
)


def text_wrap(l: List[str], w=10) -> List[str]:
    return [ "\n".join(textwrap.wrap(str(x), w)) for x in l ]

def new_figure(
    fig_w = 10,
    fig_h = 8,
    left_margin = 1,
    right_margin = 1,
    top_margin = 1,
    bottom_margin = 1
) -> Tuple[Figure, Axes]:
    """
    Creates a figure and with a pre-defined margin.
    """
    fig = plt.figure(figsize=(fig_w, fig_h))
    ax = fig.add_axes((
        left_margin / fig_w,
        bottom_margin / fig_h,
        (fig_w - (left_margin + right_margin)) / fig_w,
        (fig_h - (top_margin + bottom_margin)) / fig_h
    ))
    return fig, ax


EXP_DIR = "experiments/run-2021_11_02-16_20_50"

@from_dict
@dataclass(frozen=True, order=True)
class ExpCfg:
    """
    Experiment configuration.
    """
    array_size: Tuple[int, int]
    batch_size: int
    imsize: int
    interconnect_type: IctType
    model: str
    no_array: int
    sentence_len: int


@from_dict
@dataclass(frozen=True, order=True)
class ExpRes:
    """
    Experiment results.
    """
    interconnect_tdp: float
    no_cycles: int
    no_main_rounds: int
    no_ops: int
    no_post_rounds: int
    x_tiles_bw_usage: float
    w_tiles_bw_usage: float
    p_tiles_bw_usage: float
    total_bw_usage: float
    total_no_ops: float
    total_sram_read_bytes: int
    total_sram_write_bytes: int
    no_tiles: int

    def rounds(self):
        return self.no_main_rounds + self.no_post_rounds


def calculate_no_tiles(fpath: str) -> int:
    with open(fpath, "r") as f:
        d = json.load(f)
        r = 0
        for layer_name, layer_data in d["layers"].items():
            gemm_op = layer_data["gemm_op"]
            if gemm_op is None:
                continue
            a, b, c = gemm_op["no_tiles"]
            r = r + a * b * c
            # print(a, b, c)
        # print(r)
        r = r * d["no_repeat"]
        return r


def load_experiments(dir_path: str, rebuild = False) -> Dict[ExpCfg, ExpRes]:
    """
    Loads the experiment results from the given path.
    """

    # performs actual loading
    def load():
        res = {}
        files = os.listdir(dir_path)
        for fname in files:
            with open(f"{ dir_path }/{ fname }/sim_results.json", "r") as f:
                d = json.load(f)
                key = ExpCfg.from_dict(d)
                if res.get(key) is not None:
                    raise RuntimeError("Repeated key: ", d)
                d["no_tiles"] = calculate_no_tiles(f"{ dir_path }/{ fname }/precompiled_model.json")
                res[key] = ExpRes.from_dict(d)
        return res
    
    cache_path = f"{ dir_path }/cache.pickle"
    if rebuild or not os.path.exists(cache_path):
        print("Rebuilding cache.")
        data = load()
        with open(cache_path, "wb") as f:
            pickle.dump(data, f)
        return data
    else:
        print("Loading cache.")
        with open(cache_path, "rb") as f:
            return pickle.load(f)


def calculate_perc_active(no_array: int, results: List[ExpRes]) -> float:
    return scipy.stats.gmean(
        [ x.no_tiles / x.no_main_rounds / no_array * 100 for x in results ]
    )

def calculate_cyc_op(l: List[ExpRes]) -> float:
    return np.mean(
        # TODO how to get no_cycles only for the matrix multiplications?
        # or, do we need it?
        [ x.no_cycles / x.rounds() for x in l ]
    )

# Which graphs do we want to show?
#
#    * Number of rounds vs number of arrays --> Contention
#    * Cycles per round vs number of arrays --> Latency
#
# for each type of interconnect.
#

def main():
    print("Loading experiment data...")
    experiments = load_experiments(EXP_DIR)
    print("Done.")

    init_matplotlib()

    def task1():
        def plot_active_arrays_percentage(model: str = None):
            plt.figure(figsize=(7, 5))

            fig, ax = plt.subplots(1, 1)

            if model is not None:
                ax.set_title(f"Percentage of Active Arrays for {model}\nfor each Interconnect Type")
            else:
                ax.set_title(f"Percentage of Active Arrays\nfor each Interconnect Type")

            l_no_array = [32, 64, 128, 256]
            l_ict_type = [
                IctType.banyan_exp_0,
                IctType.banyan_exp_1,
                IctType.banyan_exp_2,
                IctType.banyan_exp_3,
                IctType.crossbar,
                IctType.benes_copy
            ]
            l_ict_labels = text_wrap(l_ict_type)
            def filter_func(k: ExpCfg):
                return (
                    k.interconnect_type == ict and
                    k.no_array == no_array and
                    (
                        model is None or
                        model is not None and k.model == model
                    )
                )
            for style, ict in zip(plot_styles, l_ict_type):
                l_percentages = []
                for no_array in l_no_array:
                    x = filter_key(experiments, filter_func)
                    percentages = [
                        xx.no_tiles / xx.no_main_rounds / no_array * 100
                        for xx in x.values() ]
                    l_percentages.append(scipy.stats.gmean(percentages))
                ax.plot(l_no_array, l_percentages, label=str(ict), **style)
            
            ax.set_xscale("log", base=2)
            ax.set_xticks(l_no_array)
            ax.set_ylabel("Percentage of Active Arrays")
            ax.set_xlabel("Number of Arrays")
            ax.legend()
            # ax.loglog()
            if model is not None:
                plt.savefig(f"plots/perc_active_{model}.png")
                plt.savefig(f"plots/perc_active_{model}.svg")
            else:
                plt.savefig(f"plots/perc_active.png")
                plt.savefig(f"plots/perc_active.svg")
        
        models = ["inception", "resnet50",  "resnet101",  "resnet152", "densenet121", "densenet169",
                "densenet201", "bert_tiny", "bert_small", "bert_medium", "bert_base", "bert_large"]
        for model in models:
            plot_active_arrays_percentage(model=model)
        plot_active_arrays_percentage(None)

    def task2():
        # interconnect types we are interested in
        l_ict_type = [
            IctType.banyan_exp_0,
            IctType.banyan_exp_1,
            IctType.banyan_exp_2,
            IctType.banyan_exp_3,
            IctType.crossbar,
            IctType.benes_copy
        ]

        no_array = 128

        # figure parameters
        fig_params = {
            "fig_w": 8,
            "fig_h": 8,
            "left_margin": 1,
            "right_margin": 1,
            "top_margin": 0.5,
            "bottom_margin": 2
        }

        # filter function to select the experiments
        def filter(ict_type: IctType):
            def f(k: ExpCfg):
                return (
                    k.interconnect_type == ict_type and
                    k.no_array == no_array
                )
            return f

        def plot_perc_busy():
            fig, ax = new_figure(**fig_params)

            x_pos = np.arange(len(l_ict_type))
            y_pos = np.zeros(len(l_ict_type))

            for idx, ict in enumerate(l_ict_type):
                results = filter_key(experiments, filter(ict))
                perc_active = calculate_perc_active(no_array, results.values())
                y_pos[idx] = perc_active

            print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
            ax.grid()
            ax.set_axisbelow(True)
            ax.bar(x_pos, y_pos, align="center", width=0.6, label="N = 128")
            ax.set_xticks(x_pos)
            ax.set_xticklabels(text_wrap(l_ict_type, 15), rotation=30)
            ax.set_ylim([0, 100])
            ax.set_ylabel("Percentage of Active Arrays")
            ax.set_xlabel("Interconnect Type")
            ax.legend(loc="upper left")

            fig.savefig("plots/plot_perc_busy.svg")
            fig.savefig("plots/plot_perc_busy.png", dpi=100)

        def plot_cycles_ops():
            fig, ax = new_figure(**fig_params)

            x_pos = np.arange(len(l_ict_type))
            y_pos = np.zeros(len(l_ict_type))

            for idx, ict in enumerate(l_ict_type):
                results = filter_key(experiments, filter(ict))
                perc_active = calculate_cyc_op(results.values())
                y_pos[idx] = perc_active

            print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
            ax.grid()
            ax.set_axisbelow(True)
            ax.bar(x_pos, y_pos, align="center", width=0.6, label="N = 128")
            ax.set_xticks(x_pos)
            ax.set_xticklabels(text_wrap(l_ict_type, 15), rotation=30)
            ax.set_ylabel("Cycles per Operation")
            ax.set_xlabel("Interconnect Type")
            ax.legend(loc="upper left")

            fig.savefig("plots/plot_perc_cycles_ops.svg")
            fig.savefig("plots/plot_perc_cycles_ops.png", dpi=100)

        def plot_ict_power():
            fig, ax = new_figure(**fig_params)

            x_pos = np.arange(len(l_ict_type))
            y_pos = np.zeros(len(l_ict_type))

            for idx, ict in enumerate(l_ict_type):
                results = filter_key(experiments, filter(ict))
                for k, v in results.items():
                    y_pos[idx] = v.interconnect_tdp
                    break

            print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
            ax.grid()
            ax.set_axisbelow(True)
            ax.bar(x_pos, y_pos, align="center", width=0.6, label="N = 128")
            ax.set_xticks(x_pos)
            ax.set_xticklabels(text_wrap(l_ict_type, 15), rotation=30)
            ax.set_ylabel("Power [Watt]")
            ax.set_xlabel("Interconnect Type")
            ax.legend(loc="upper left")

            fig.savefig("plots/plot_watt_byte.svg")
            fig.savefig("plots/plot_watt_byte.png", dpi=100)
        
        plot_perc_busy()
        plot_cycles_ops()
        plot_ict_power()

    # task1()
    task2()


if __name__ == "__main__":
    main()
