from dataclasses import dataclass
from typing import Tuple, Dict, Callable
from common import from_dict
from enum import IntEnum
import json
import pickle
import os

class IctType(IntEnum):
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
        """
        Creates a string from the interconnect type.
        """
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
    model: str
    no_array: int
    sentence_len: int
    interconnect_type: IctType


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
