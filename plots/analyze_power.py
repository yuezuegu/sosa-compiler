"""
This file analyzes the power consumption for SOSA.
"""

from result_utils import *
from typing import Iterator
import scipy
import scipy.stats

EXP_DIR = "experiments/run-2021_11_21-14_19_29"

def main():
    print("Loading experiment data...")
    experiments = load_experiments(EXP_DIR)
    print("Done.")

    # print_experiments(experiments)

    # constants
    sram_energy_per_byte = 2.7e-12 # J / byte
    freq = 1e9 # 1/s

    def task1() -> float:
        def sram_power() -> Iterator[float]:
            for k, v in experiments.items():
                total_energy = \
                    (v.total_sram_read_bytes + v.total_sram_write_bytes) * sram_energy_per_byte
                time = v.no_cycles / freq
                yield total_energy / time
        
        r = scipy.stats.gmean(list(sram_power()))
        print(f"SRAM power = { r }")
        return r

    def task2():
        pass
    
    task1()

if __name__ == "__main__":
    main()
