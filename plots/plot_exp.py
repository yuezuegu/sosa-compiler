from dataclasses import dataclass
from common import from_dict, named_zip
from enum import Enum
import json
import os
from typing import Callable, Dict, Tuple, List, Any
from functools import reduce

import matplotlib.pyplot as plt
from matplotlib import rc

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

EXP_DIR = "experiments/run-2021_11_02-16_20_50"

class IctType(Enum):
    """
    Defines the interconnect type.
    """
    crossbar = 1 << 4
    benes_copy = 2 << 4
    benes_vanilla = 4 << 4
    banyan = 8 << 4
    banyan_exp_0 = banyan | 0
    banyan_exp_1 = banyan | 1
    banyan_exp_2 = banyan | 2
    banyan_exp_3 = banyan | 3
    banyan_exp_4 = banyan | 4

# TODO is there a better way to assign aliases to enum names?
def _icttype_str() -> Callable[[], Callable[[IctType], str]]:
    d = {
        IctType.crossbar: "Crossbar",
        IctType.benes_copy: "Benes with Copy Network",
        IctType.benes_vanilla: "Benes",
        IctType.banyan: "Butterfly-1",
        IctType.banyan_exp_0: "Butterfly-1",
        IctType.banyan_exp_1: "Butterfly-2",
        IctType.banyan_exp_2: "Butterfly-4",
        IctType.banyan_exp_3: "Butterfly-8",
        IctType.banyan_exp_4: "Butterfly-16",
    }
    def f(self: IctType) -> str:
        return d[self]
    return f
IctType.__str__ = _icttype_str()

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
        return r

def load_experiments(dir_path: str) -> Dict[ExpCfg, ExpRes]:
    """
    Loads the experiment results from the given path.
    """
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


def filter_key(d: Dict, pred) -> Dict:
    """
    Filters the given function with a predicate.
    """
    return { k: v for (k, v) in d.items() if pred(k) }

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
                l_total_rounds = []
                for no_array in l_no_array:
                    print(ict, no_array)
                    x = filter_key(experiments, filter_func)
                    print(list(x.keys()))
                    no_rounds = [ xx.rounds() for xx in x.values() ]
                    no_tiles = [ xx.no_tiles for xx in x.values() ]
                    l_total_rounds.append(sum(no_tiles) / sum(no_rounds) / no_array * 100)
                    avg1 = sum(no_tiles) / sum(no_rounds) / no_array * 100
                    avg2 = sum([no_tiles[i] / no_rounds[i] / no_array * 100 for i in range(len(no_tiles))]) / len(no_tiles)
                    print(avg1, " - ", avg2)
                ax.plot(l_no_array, l_total_rounds, label=str(ict), **style)
            
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
    
    task1()


if __name__ == "__main__":
    main()
