from enum import Enum
from typing import Callable

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
