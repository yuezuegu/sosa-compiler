"""
This file analyzes the power consumption for SOSA.
"""

from result_utils import *

EXP_DIR = "experiments/run-2021_11_21-14_19_29"

def main():
    print("Loading experiment data...")
    experiments = load_experiments(EXP_DIR)
    print("Done.")

    # print_experiments(experiments)



if __name__ == "__main__":
    main()
