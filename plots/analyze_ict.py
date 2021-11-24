"""
This file generates following metrics for each interconnect type:

    * Percentage of active pods
    * Interconnect power consumption relative to all system
    * Cycles a tile operation takes

"""

from common import filter_key
import dataclasses
from typing import Dict, List
import scipy.stats
import scipy
import numpy as np
import math
import matplotlib.pyplot as plt
from result_utils import *
from plot_helpers import *

EXP_DIR = "experiments/run-2021_11_02-16_20_50"

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

    # we do not want to take bert_tiny and bert_small models into account
    for k in list(experiments.keys()):
        if k.model in ["bert_tiny", "bert_small"]:
            del experiments[k]

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
        
        # to store data for latex
        ltx = [ [ f"\multrowR{{ { str(x) } }}", None, None, None ] for x in l_ict_type ]

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
                ltx[idx][1] = f"{perc_active:.2f}"

            # print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
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
                cyc_op = calculate_cyc_op(results.values())
                y_pos[idx] = cyc_op
                ltx[idx][2] = f"{cyc_op:.2f}"

            # print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
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
            # this one finds relative to the overall power consumption
            fig, ax = new_figure(**fig_params)

            x_pos = np.arange(len(l_ict_type))
            y_pos = np.zeros(len(l_ict_type))

            e_data = 4.1e-12 # J per byte
            e_compute = 0.4e-12 # J per MAC
            freq = 1e9

            for idx, ict in enumerate(l_ict_type):
                results = filter_key(experiments, filter(ict))
                for k, v in results.items():
                    sw_width = k.array_size[0] + 5 * k.array_size[1]
                    p_compute = no_array * k.array_size[0] * k.array_size[1] * e_compute * freq
                    p_data = k.no_array * sw_width * e_data * freq
                    p = v.interconnect_tdp / (p_data + p_compute + v.interconnect_tdp) * 100
                    y_pos[idx] = p
                    ltx[idx][3] = f"{p:.2f}"

            # print("\t".join([ f"{x:.2f}" for x in y_pos ]))
            
            ax.grid()
            ax.set_axisbelow(True)
            ax.bar(x_pos, y_pos, align="center", width=0.6, label="N = 128")
            ax.set_xticks(x_pos)
            ax.set_xticklabels(text_wrap(l_ict_type, 15), rotation=30)
            ax.set_ylabel("Interconnect Power / Total Power [Percentage]")
            ax.set_xlabel("Interconnect Type")
            ax.legend(loc="upper left")

            fig.savefig("plots/plot_ict_power_total_power.svg")
            fig.savefig("plots/plot_ict_power_total_power.png", dpi=100)
        
        def plot_ict_power_w_per_byte():
            calc = {  } # for calculating the ICT power
            # Usage: calc[ict_type](n) where n = log N
            # Taken from the C++ code
            calc[IctType.banyan_exp_0] = lambda n: 2.875e-5 * (math.pow(2, n)) * n
            calc[IctType.banyan_exp_1] = lambda n: calc[IctType.banyan_exp_0](n + 1)
            calc[IctType.banyan_exp_2] = lambda n: calc[IctType.banyan_exp_0](n + 2)
            calc[IctType.banyan_exp_3] = lambda n: calc[IctType.banyan_exp_0](n + 3)
            calc[IctType.benes_copy] = lambda n: 2.875e-5 * 4 * (math.pow(2, n)) * n
            calc[IctType.crossbar] = lambda n: 2.875e-5 * (math.pow(2, n)) * (math.pow(2, n))

            for idx, ict in enumerate(l_ict_type):
                n = 8 # N = 256
                p = calc[ict](n) / 256 * 1e3 # mW/byte
                ltx[idx][3] = f"{p:.2f}"

        plot_perc_busy()
        plot_cycles_ops()
        plot_ict_power_w_per_byte()

        print("\\\\\\hline\n\n".join([ " &\n".join(a) for a in ltx ]))

    def task3():
        cfgs: Dict[ExpCfg, Dict[IctType, ExpRes]] = {}
        for cfg, res in experiments.items():
            cfg_p = dataclasses.replace(cfg, interconnect_type=None)
            if cfg_p not in cfgs:
                cfgs[cfg_p] = {}
            cfgs[cfg_p][cfg.interconnect_type] = res
        
        for cfg, t in cfgs.items():
            if t[IctType.banyan_exp_2].no_main_rounds < t[IctType.crossbar].no_main_rounds:
                print(cfg)
                for ict in sorted(t.keys()):
                    print(f"{ repr(ict) } --> { t[ict] }")
                print()


    # task1()
    task2()
    # task3()


if __name__ == "__main__":
    main()
