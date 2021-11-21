import textwrap
from matplotlib.figure import Figure
from matplotlib.axes import Axes
import matplotlib.pyplot as plt
from matplotlib import rc
from typing import List, Tuple
from common import named_zip

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
